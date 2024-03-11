// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2021 Felix Fietkau <nbd@nbd.name> */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/bitfield.h>
#include <linux/dma-mapping.h>
#include <linux/skbuff.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/mfd/syscon.h>
#include <linux/debugfs.h>
#include <linux/iopoll.h>
#include <linux/soc/mediatek/mtk_wed.h>
#include <net/rtnetlink.h>

#include "mtk_eth_soc.h"
#include "mtk_eth_reset.h"
#include "mtk_wed_regs.h"
#include "mtk_wed.h"
#include "mtk_ppe.h"
#include "mtk_wed_mcu.h"
#include "mtk_wed_wo.h"

struct wo_cmd_ring {
	u32 q_base;
	u32 cnt;
	u32 unit;
};
static struct mtk_wed_hw *hw_list[3];
static DEFINE_MUTEX(hw_lock);

static void
wed_m32(struct mtk_wed_device *dev, u32 reg, u32 mask, u32 val)
{
	regmap_update_bits(dev->hw->regs, reg, mask | val, val);
}

static void
wed_set(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	return wed_m32(dev, reg, 0, mask);
}

static void
wed_clr(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	return wed_m32(dev, reg, mask, 0);
}

static void
wdma_m32(struct mtk_wed_device *dev, u32 reg, u32 mask, u32 val)
{
	wdma_w32(dev, reg, (wdma_r32(dev, reg) & ~mask) | val);
}

static void
wdma_set(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	wdma_m32(dev, reg, 0, mask);
}

static void
wdma_clr(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	wdma_m32(dev, reg, mask, 0);
}

static u32
mtk_wdma_read_reset(struct mtk_wed_device *dev)
{
	return wdma_r32(dev, MTK_WDMA_GLO_CFG);
}

static u32
mtk_wed_check_busy(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	if (wed_r32(dev, reg) & mask)
		return true;

	return false;
}

static int
mtk_wed_poll_busy(struct mtk_wed_device *dev, u32 reg, u32 mask)
{
	int sleep = 1000;
	int timeout = 100 * sleep;
	u32 val;

	return read_poll_timeout(mtk_wed_check_busy, val, !val, sleep,
				 timeout, false, dev, reg, mask);
}

static int
mtk_wdma_rx_reset(struct mtk_wed_device *dev)
{
	u32 status;
	u32 mask = MTK_WDMA_GLO_CFG_RX_DMA_BUSY;
	int busy, i;
	u32 value;

	wdma_clr(dev, MTK_WDMA_GLO_CFG, MTK_WDMA_GLO_CFG_RX_DMA_EN);
	busy = readx_poll_timeout(mtk_wdma_read_reset, dev, status,
				  !(status & mask), 0, 10000);

	if (dev->hw->version == 3) {
		wdma_clr(dev, MTK_WDMA_PREF_TX_CFG, MTK_WDMA_PREF_TX_CFG_PREF_EN);
		wdma_clr(dev, MTK_WDMA_PREF_RX_CFG, MTK_WDMA_PREF_RX_CFG_PREF_EN);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_PREF_TX_CFG_PREF_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_PREF_TX_CFG);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_PREF_RX_CFG_PREF_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_PREF_RX_CFG);

		wdma_clr(dev, MTK_WDMA_WRBK_TX_CFG, MTK_WDMA_WRBK_TX_CFG_WRBK_EN);
		wdma_clr(dev, MTK_WDMA_WRBK_RX_CFG, MTK_WDMA_WRBK_RX_CFG_WRBK_EN);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_WRBK_TX_CFG_WRBK_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_WRBK_TX_CFG);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_WRBK_RX_CFG_WRBK_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_WRBK_RX_CFG);

		/* Prefetch FIFO */
		wdma_w32(dev, MTK_WDMA_PREF_RX_FIFO_CFG,
			 MTK_WDMA_PREF_RX_FIFO_CFG_RING0_CLEAR |
			 MTK_WDMA_PREF_RX_FIFO_CFG_RING1_CLEAR);
		wdma_clr(dev, MTK_WDMA_PREF_RX_FIFO_CFG,
			 MTK_WDMA_PREF_RX_FIFO_CFG_RING0_CLEAR |
			 MTK_WDMA_PREF_RX_FIFO_CFG_RING1_CLEAR);

		/* Core FIFO */
		value = (MTK_WDMA_XDMA_RX_FIFO_CFG_RX_PAR_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_CMD_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_DMAD_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_ARR_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_LEN_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_WID_FIFO_CLEAR |
			 MTK_WDMA_XDMA_RX_FIFO_CFG_RX_BID_FIFO_CLEAR);

		wdma_w32(dev, MTK_WDMA_XDMA_RX_FIFO_CFG, value);
		wdma_clr(dev, MTK_WDMA_XDMA_RX_FIFO_CFG, value);

		/* Writeback FIFO */
		wdma_w32(dev, MTK_WDMA_WRBK_RX_FIFO_CFG(0), MTK_WDMA_WRBK_RX_FIFO_CFG_RING_CLEAR);
		wdma_w32(dev, MTK_WDMA_WRBK_RX_FIFO_CFG(1), MTK_WDMA_WRBK_RX_FIFO_CFG_RING_CLEAR);

		wdma_clr(dev, MTK_WDMA_WRBK_RX_FIFO_CFG(0), MTK_WDMA_WRBK_RX_FIFO_CFG_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_WRBK_RX_FIFO_CFG(1), MTK_WDMA_WRBK_RX_FIFO_CFG_RING_CLEAR);

		/* Prefetch ring status */
		wdma_w32(dev, MTK_WDMA_PREF_SIDX_CFG, MTK_WDMA_PREF_SIDX_CFG_RX_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_PREF_SIDX_CFG, MTK_WDMA_PREF_SIDX_CFG_RX_RING_CLEAR);
		/* Writeback ring status */
		wdma_w32(dev, MTK_WDMA_WRBK_SIDX_CFG, MTK_WDMA_WRBK_SIDX_CFG_RX_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_WRBK_SIDX_CFG, MTK_WDMA_WRBK_SIDX_CFG_RX_RING_CLEAR);
	}
	wdma_w32(dev, MTK_WDMA_RESET_IDX, MTK_WDMA_RESET_IDX_RX);
	wdma_w32(dev, MTK_WDMA_RESET_IDX, 0);

	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		if (!dev->tx_wdma[i].desc) {
			wdma_w32(dev, MTK_WDMA_RING_RX(i) +
				 MTK_WED_RING_OFS_CPU_IDX, 0);
	}

	return busy;
}

static void
mtk_wdma_tx_reset(struct mtk_wed_device *dev)
{
	u32 status;
	u32 mask = MTK_WDMA_GLO_CFG_TX_DMA_BUSY;
	int busy, i;
	u32 value;

	wdma_clr(dev, MTK_WDMA_GLO_CFG, MTK_WDMA_GLO_CFG_TX_DMA_EN);
	if (readx_poll_timeout(mtk_wdma_read_reset, dev, status,
			       !(status & mask), 0, 10000))
		WARN_ON_ONCE(1);

	if (dev->hw->version == 3) {
		wdma_clr(dev, MTK_WDMA_PREF_TX_CFG, MTK_WDMA_PREF_TX_CFG_PREF_EN);
		wdma_clr(dev, MTK_WDMA_PREF_RX_CFG, MTK_WDMA_PREF_RX_CFG_PREF_EN);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_PREF_TX_CFG_PREF_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_PREF_TX_CFG);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_PREF_RX_CFG_PREF_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_PREF_RX_CFG);

		wdma_clr(dev, MTK_WDMA_WRBK_TX_CFG, MTK_WDMA_WRBK_TX_CFG_WRBK_EN);
		wdma_clr(dev, MTK_WDMA_WRBK_RX_CFG, MTK_WDMA_WRBK_RX_CFG_WRBK_EN);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_WRBK_TX_CFG_WRBK_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_WRBK_TX_CFG);
		busy = read_poll_timeout(wdma_r32, status,
					 !(status & MTK_WDMA_WRBK_RX_CFG_WRBK_BUSY), 0, 10000,
					 false, dev, MTK_WDMA_WRBK_RX_CFG);

		/* Prefetch FIFO */
		wdma_w32(dev, MTK_WDMA_PREF_TX_FIFO_CFG,
			 MTK_WDMA_PREF_TX_FIFO_CFG_RING0_CLEAR |
			 MTK_WDMA_PREF_TX_FIFO_CFG_RING1_CLEAR);
		wdma_clr(dev, MTK_WDMA_PREF_TX_FIFO_CFG,
			 MTK_WDMA_PREF_TX_FIFO_CFG_RING0_CLEAR |
			 MTK_WDMA_PREF_TX_FIFO_CFG_RING1_CLEAR);
		/* Core FIFO */
		value = (MTK_WDMA_XDMA_TX_FIFO_CFG_TX_PAR_FIFO_CLEAR |
			 MTK_WDMA_XDMA_TX_FIFO_CFG_TX_CMD_FIFO_CLEAR |
			 MTK_WDMA_XDMA_TX_FIFO_CFG_TX_DMAD_FIFO_CLEAR |
			 MTK_WDMA_XDMA_TX_FIFO_CFG_TX_ARR_FIFO_CLEAR);

		wdma_w32(dev, MTK_WDMA_XDMA_TX_FIFO_CFG, value);
		wdma_clr(dev, MTK_WDMA_XDMA_TX_FIFO_CFG, value);
		/* Writeback FIFO */
		wdma_w32(dev, MTK_WDMA_WRBK_TX_FIFO_CFG(0), MTK_WDMA_WRBK_TX_FIFO_CFG_RING_CLEAR);
		wdma_w32(dev, MTK_WDMA_WRBK_TX_FIFO_CFG(1), MTK_WDMA_WRBK_TX_FIFO_CFG_RING_CLEAR);

		wdma_clr(dev, MTK_WDMA_WRBK_TX_FIFO_CFG(0), MTK_WDMA_WRBK_TX_FIFO_CFG_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_WRBK_TX_FIFO_CFG(1), MTK_WDMA_WRBK_TX_FIFO_CFG_RING_CLEAR);

		/* Prefetch ring status */
		wdma_w32(dev, MTK_WDMA_PREF_SIDX_CFG, MTK_WDMA_PREF_SIDX_CFG_TX_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_PREF_SIDX_CFG, MTK_WDMA_PREF_SIDX_CFG_TX_RING_CLEAR);
		/* Writeback ring status */
		wdma_w32(dev, MTK_WDMA_WRBK_SIDX_CFG, MTK_WDMA_WRBK_SIDX_CFG_TX_RING_CLEAR);
		wdma_clr(dev, MTK_WDMA_WRBK_SIDX_CFG, MTK_WDMA_WRBK_SIDX_CFG_TX_RING_CLEAR);
	}
	wdma_w32(dev, MTK_WDMA_RESET_IDX, MTK_WDMA_RESET_IDX_TX);
	wdma_w32(dev, MTK_WDMA_RESET_IDX, 0);
	for (i = 0; i < ARRAY_SIZE(dev->rx_wdma); i++)
		wdma_w32(dev, MTK_WDMA_RING_TX(i) +
			 MTK_WED_RING_OFS_CPU_IDX, 0);
}

static u32
mtk_wed_read_reset(struct mtk_wed_device *dev)
{
	return wed_r32(dev, MTK_WED_RESET);
}

static void
mtk_wed_reset(struct mtk_wed_device *dev, u32 mask)
{
	u32 status;

	wed_w32(dev, MTK_WED_RESET, mask);
	if (readx_poll_timeout(mtk_wed_read_reset, dev, status,
			       !(status & mask), 0, 1000))
		WARN_ON_ONCE(1);
}

static void
mtk_wed_wo_reset(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	u8 state = WO_STATE_DISABLE;
	u8 state_done = WOIF_DISABLE_DONE;
	void __iomem *reg;
	u32 value;
	unsigned long timeout = jiffies + WOCPU_TIMEOUT;

	mtk_wdma_tx_reset(dev);

	mtk_wed_reset(dev, MTK_WED_RESET_WED);

	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), false);

	do {
		value = wed_r32(dev, MTK_WED_SCR0 + 4 * WED_DUMMY_CR_WO_STATUS);
	} while (value != state_done && !time_after(jiffies, timeout));

	reg = ioremap(WOCPU_MCUSYS_RESET_ADDR, 4);
	value = readl((void *)reg);
	switch(dev->hw->index) {
	case 0:
		value |= WOCPU_WO0_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		value &= ~WOCPU_WO0_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		break;
	case 1:
		value |= WOCPU_WO1_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		value &= ~WOCPU_WO1_MCUSYS_RESET_MASK;
		writel(value, (void *)reg);
		break;
	default:
		dev_err(dev->hw->dev, "wrong mtk_wed%d\n",
			dev->hw->index);

		break;
	}

	iounmap((void *)reg);
}

void mtk_wed_fe_reset(void)
{
	int i;

	mutex_lock(&hw_lock);

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];
		struct mtk_wed_device *dev = hw->wed_dev;
		int err;

		if (!dev || !dev->wlan.reset)
			continue;

		pr_info("%s: receive fe reset start event, trigger SER\n", __func__);

		/* reset callback blocks until WLAN reset is completed */
		err = dev->wlan.reset(dev);
		if (err)
			dev_err(dev->dev, "wlan reset failed: %d\n", err);
	}

	mutex_unlock(&hw_lock);
}

void mtk_wed_fe_reset_complete(void)
{
	int i;

	mutex_lock(&hw_lock);

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];
		struct mtk_wed_device *dev = hw->wed_dev;

		if (!dev || !dev->wlan.reset_complete)
			continue;

		pr_info("%s: receive fe reset done event, continue SER\n", __func__);
		dev->wlan.reset_complete(dev);
	}

	mutex_unlock(&hw_lock);
}

static struct mtk_wed_hw *
mtk_wed_assign(struct mtk_wed_device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw = hw_list[i];

		if (!hw || hw->wed_dev)
			continue;

		hw->wed_dev = dev;
		hw->pci_base = MTK_WED_PCIE_BASE;

		return hw;
	}

	return NULL;
}

static int
mtk_wed_pao_buffer_alloc(struct mtk_wed_device *dev)
{
	struct mtk_wed_pao *pao;
	int i, j;

	pao = kzalloc(sizeof(struct mtk_wed_pao), GFP_KERNEL);
	if (!pao)
		return -ENOMEM;

	dev->hw->wed_pao = pao;

	for (i = 0; i < 32; i++) {
		/* each segment is 64K*/
		pao->hif_txd[i] = (char *)__get_free_pages(GFP_ATOMIC |
							   GFP_DMA32 |
							   __GFP_ZERO, 4);
		if (!pao->hif_txd[i])
			goto err;

		pao->hif_txd_phys[i] = dma_map_single(dev->hw->dev,
						      pao->hif_txd[i],
						      16 * PAGE_SIZE,
						      DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(dev->hw->dev,
					       pao->hif_txd_phys[i])))
			goto err;
	}

	return 0;

err:
	for (j = 0; j < i; j++)
		dma_unmap_single(dev->hw->dev, pao->hif_txd_phys[j],
			     16 * PAGE_SIZE, DMA_TO_DEVICE);

	return -ENOMEM;
}

static int
mtk_wed_pao_free_buffer(struct mtk_wed_device *dev)
{
	struct mtk_wed_pao *pao = dev->hw->wed_pao;
	int i;

	for (i = 0; i < 32; i++) {
		dma_unmap_single(dev->hw->dev, pao->hif_txd_phys[i],
				 16 * PAGE_SIZE, DMA_TO_DEVICE);
		free_pages((unsigned long)pao->hif_txd[i], 4);
	}

	return 0;
}

static int
mtk_wed_tx_buffer_alloc(struct mtk_wed_device *dev)
{
	struct mtk_wdma_desc *desc;
	void *desc_ptr;
	dma_addr_t desc_phys;
	struct dma_page_info *page_list;
	u32 last_seg = MTK_WDMA_DESC_CTRL_LAST_SEG1;
	int token = dev->wlan.token_start;
	int ring_size, pkt_nums, n_pages, page_idx;
	int i, ret = 0;

	if (dev->ver == MTK_WED_V1) {
		ring_size = dev->wlan.nbuf & ~(MTK_WED_BUF_PER_PAGE - 1);
		pkt_nums = ring_size;
		dev->tx_buf_ring.desc_size = sizeof(struct mtk_wdma_desc);
	} else if (dev->hw->version == 2) {
		ring_size = MTK_WED_VLD_GROUP_SIZE * MTK_WED_PER_GROUP_PKT +
			    MTK_WED_WDMA_RING_SIZE * 2;
		last_seg = MTK_WDMA_DESC_CTRL_LAST_SEG0;
		dev->tx_buf_ring.desc_size = sizeof(struct mtk_wdma_desc);
	} else if (dev->hw->version == 3) {
		ring_size = MTK_WED_TX_BM_DMA_SIZE;
		pkt_nums = MTK_WED_TX_BM_PKT_CNT;
		dev->tx_buf_ring.desc_size = sizeof(struct mtk_rxbm_desc);
	}

	n_pages = ring_size / MTK_WED_BUF_PER_PAGE;

	page_list = kcalloc(n_pages, sizeof(*page_list), GFP_KERNEL);
	if (!page_list)
		return -ENOMEM;

	dev->tx_buf_ring.size = ring_size;
	dev->tx_buf_ring.pages = page_list;
	dev->tx_buf_ring.pkt_nums = pkt_nums;

	desc_ptr = dma_alloc_coherent(dev->hw->dev,
				      ring_size * dev->tx_buf_ring.desc_size,
				      &desc_phys, GFP_KERNEL);
	if (!desc_ptr)
		return -ENOMEM;

	dev->tx_buf_ring.desc = desc_ptr;
	dev->tx_buf_ring.desc_phys = desc_phys;

	for (i = 0, page_idx = 0; i < pkt_nums; i += MTK_WED_BUF_PER_PAGE) {
		dma_addr_t page_phys, buf_phys;
		struct page *page;
		void *buf;
		int s;

		page = __dev_alloc_pages(GFP_KERNEL | GFP_DMA32, 0);
		if (!page)
			return -ENOMEM;

		page_phys = dma_map_page(dev->hw->dev, page, 0, PAGE_SIZE,
					 DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev->hw->dev, page_phys)) {
			__free_page(page);
			return -ENOMEM;
		}

		page_list[page_idx].addr = page;
		page_list[page_idx].addr_phys = page_phys;
		page_idx++;

		dma_sync_single_for_cpu(dev->hw->dev, page_phys, PAGE_SIZE,
					DMA_BIDIRECTIONAL);

		buf = page_to_virt(page);
		buf_phys = page_phys;

		for (s = 0; s < MTK_WED_BUF_PER_PAGE; s++) {
			desc = desc_ptr;
			desc->buf0 = buf_phys;
			if (dev->hw->version < 3) {
				u32 txd_size;

				txd_size = dev->wlan.init_buf(buf, buf_phys, token++);
				desc->buf1 = buf_phys + txd_size;
				desc->ctrl = FIELD_PREP(MTK_WDMA_DESC_CTRL_LEN0,
							txd_size) |
					     FIELD_PREP(MTK_WDMA_DESC_CTRL_LEN1,
							MTK_WED_BUF_SIZE - txd_size) |
							last_seg;
				desc->info = 0;
			} else {
				desc->ctrl = token << 16;
			}
			desc_ptr += dev->tx_buf_ring.desc_size;

			buf += MTK_WED_BUF_SIZE;
			buf_phys += MTK_WED_BUF_SIZE;
		}

		dma_sync_single_for_device(dev->hw->dev, page_phys, PAGE_SIZE,
					   DMA_BIDIRECTIONAL);
	}

	if (dev->hw->version == 3)
		ret = mtk_wed_pao_buffer_alloc(dev);

	return ret;
}

static void
mtk_wed_free_tx_buffer(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc = dev->tx_buf_ring.desc;
	struct dma_page_info *page_list = dev->tx_buf_ring.pages;
	int ring_size, page_idx, pkt_nums;
	int i;

	if (!page_list)
		return;

	if (!desc)
		goto free_pagelist;

	pkt_nums = ring_size = dev->tx_buf_ring.size;
	if (dev->hw->version == 3) {
		mtk_wed_pao_free_buffer(dev);
		pkt_nums = dev->tx_buf_ring.pkt_nums;
	}

	for (i = 0, page_idx = 0; i < pkt_nums; i += MTK_WED_BUF_PER_PAGE) {
		void *page = page_list[page_idx].addr;

		if (!page)
			break;

		dma_unmap_page(dev->hw->dev, page_list[page_idx].addr_phys,
			       PAGE_SIZE, DMA_BIDIRECTIONAL);
		__free_page(page);
		page_idx++;
	}

	dma_free_coherent(dev->hw->dev, ring_size * dev->tx_buf_ring.desc_size,
			  dev->tx_buf_ring.desc, dev->tx_buf_ring.desc_phys);

free_pagelist:
	kfree(page_list);
}

static int
mtk_wed_rx_buffer_alloc(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc;
	dma_addr_t desc_phys;
	int ring_size;

	ring_size = dev->wlan.rx_nbuf;
	dev->rx_buf_ring.size = ring_size;
	desc = dma_alloc_coherent(dev->hw->dev, ring_size * sizeof(*desc),
				  &desc_phys, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	dev->rx_buf_ring.desc = desc;
	dev->rx_buf_ring.desc_phys = desc_phys;

	dev->wlan.init_rx_buf(dev, dev->wlan.rx_npkt);
	return 0;
}

static void
mtk_wed_free_rx_buffer(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc = dev->rx_buf_ring.desc;
	int ring_size = dev->rx_buf_ring.size;

	if (!desc)
		return;

	dev->wlan.release_rx_buf(dev);

	dma_free_coherent(dev->hw->dev, ring_size * sizeof(*desc),
			  desc, dev->rx_buf_ring.desc_phys);
}

/* TODO */
static int
mtk_wed_rx_page_buffer_alloc(struct mtk_wed_device *dev)
{
	int ring_size = dev->wlan.rx_nbuf, buf_num = MTK_WED_RX_PG_BM_CNT;
	struct mtk_rxbm_desc *desc;
	dma_addr_t desc_phys;
	struct dma_page_info *page_list;
	int n_pages, page_idx;
	int i;

	n_pages = buf_num / MTK_WED_RX_PAGE_BUF_PER_PAGE;

	page_list = kcalloc(n_pages, sizeof(*page_list), GFP_KERNEL);
	if (!page_list)
		return -ENOMEM;

	dev->rx_page_buf_ring.size = ring_size & ~(MTK_WED_BUF_PER_PAGE - 1);
	dev->rx_page_buf_ring.pages = page_list;
	dev->rx_page_buf_ring.pkt_nums = buf_num;

	desc = dma_alloc_coherent(dev->hw->dev, ring_size * sizeof(*desc),
	                         &desc_phys, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	dev->rx_page_buf_ring.desc = desc;
	dev->rx_page_buf_ring.desc_phys = desc_phys;

	for (i = 0, page_idx = 0; i < buf_num; i += MTK_WED_RX_PAGE_BUF_PER_PAGE) {
		dma_addr_t page_phys, buf_phys;
		struct page *page;
		void *buf;
		int s;

		page = __dev_alloc_pages(GFP_KERNEL | GFP_DMA32, 0);
		if (!page)
			return -ENOMEM;

		page_phys = dma_map_page(dev->hw->dev, page, 0, PAGE_SIZE,
		                        DMA_BIDIRECTIONAL);
		if (dma_mapping_error(dev->hw->dev, page_phys)) {
			__free_page(page);
			return -ENOMEM;
		}

		page_list[page_idx].addr= page;
		page_list[page_idx].addr_phys= page_phys;
		page_idx++;

		dma_sync_single_for_cpu(dev->hw->dev, page_phys, PAGE_SIZE,
		                       DMA_BIDIRECTIONAL);

		buf = page_to_virt(page);
		buf_phys = page_phys;

		for (s = 0; s < MTK_WED_RX_PAGE_BUF_PER_PAGE; s++) {

			desc->buf0 = cpu_to_le32(buf_phys);
			desc++;

			buf += MTK_WED_PAGE_BUF_SIZE;
			buf_phys += MTK_WED_PAGE_BUF_SIZE;
		}

		dma_sync_single_for_device(dev->hw->dev, page_phys, PAGE_SIZE,
					   DMA_BIDIRECTIONAL);
	}

	return 0;
}

static void
mtk_wed_rx_page_free_buffer(struct mtk_wed_device *dev)
{
	struct mtk_rxbm_desc *desc = dev->rx_page_buf_ring.desc;
	struct dma_page_info *page_list = dev->rx_page_buf_ring.pages;
	int ring_size, page_idx;
	int i;

	if (!page_list)
		return;

	if (!desc)
		goto free_pagelist;

	ring_size = dev->rx_page_buf_ring.pkt_nums;

	for (i = 0, page_idx = 0; i < ring_size; i += MTK_WED_RX_PAGE_BUF_PER_PAGE) {
		void *page = page_list[page_idx].addr;

		if (!page)
			break;

		dma_unmap_page(dev->hw->dev, page_list[page_idx].addr_phys,
                              PAGE_SIZE, DMA_BIDIRECTIONAL);
		__free_page(page);
		page_idx++;
       }

	dma_free_coherent(dev->hw->dev, dev->rx_page_buf_ring.size * sizeof(*desc),
                         desc, dev->rx_page_buf_ring.desc_phys);

free_pagelist:
       kfree(page_list);
}

static void
mtk_wed_free_ring(struct mtk_wed_device *dev, struct mtk_wed_ring *ring, int scale)
{
	if (!ring->desc)
		return;

	dma_free_coherent(dev->hw->dev, ring->size * sizeof(*ring->desc) * scale,
			  ring->desc, ring->desc_phys);
}

static void
mtk_wed_free_tx_rings(struct mtk_wed_device *dev)
{
	int i, scale = dev->hw->version > 1 ? 2 : 1;

	for (i = 0; i < ARRAY_SIZE(dev->tx_ring); i++)
		if ((dev->tx_ring[i].flags & MTK_WED_RING_CONFIGURED))
			mtk_wed_free_ring(dev, &dev->tx_ring[i], 1);

	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		if ((dev->tx_wdma[i].flags & MTK_WED_RING_CONFIGURED))
			mtk_wed_free_ring(dev, &dev->tx_wdma[i], scale);
}

static void
mtk_wed_free_rx_rings(struct mtk_wed_device *dev)
{
	int i, scale = dev->hw->version > 1 ? 2 : 1;

	for (i = 0; i < ARRAY_SIZE(dev->rx_ring); i++)
		if ((dev->rx_ring[i].flags & MTK_WED_RING_CONFIGURED))
			mtk_wed_free_ring(dev, &dev->rx_ring[i], 1);

	for (i = 0; i < ARRAY_SIZE(dev->rx_wdma); i++)
		if ((dev->rx_wdma[i].flags & MTK_WED_RING_CONFIGURED))
			mtk_wed_free_ring(dev, &dev->rx_wdma[i], scale);

	mtk_wed_free_rx_buffer(dev);
	mtk_wed_free_ring(dev, &dev->rro.rro_ring, 1);

	if (dev->wlan.hwrro)
		mtk_wed_rx_page_free_buffer(dev);
}

static void
mtk_wed_set_int(struct mtk_wed_device *dev, u32 irq_mask)
{
	u32 wdma_mask;

	wdma_mask = FIELD_PREP(MTK_WDMA_INT_MASK_RX_DONE, GENMASK(1, 0));
	if (mtk_wed_get_rx_capa(dev))
		wdma_mask |= FIELD_PREP(MTK_WDMA_INT_MASK_TX_DONE,
					GENMASK(1, 0));
	/* wed control cr set */
	wed_set(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WPDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WED_TX_BM_EN |
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	if (dev->hw->version == 1) {
		wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER,
			MTK_WED_PCIE_INT_TRIGGER_STATUS);

		wed_w32(dev, MTK_WED_WPDMA_INT_TRIGGER,
			MTK_WED_WPDMA_INT_TRIGGER_RX_DONE |
			MTK_WED_WPDMA_INT_TRIGGER_TX_DONE);

		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_SUBRT_ADV);
	} else {
		if (dev->hw->version == 3)
			wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_TX_TKID_ALI_EN);

		wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_TX,
			MTK_WED_WPDMA_INT_CTRL_TX0_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX0_DONE_CLR |
			MTK_WED_WPDMA_INT_CTRL_TX1_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX1_DONE_CLR |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX0_DONE_TRIG,
				   dev->wlan.tx_tbit[0]) |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX1_DONE_TRIG,
				   dev->wlan.tx_tbit[1]));

		wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_TX_FREE,
			MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_EN |
			MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_CLR |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_TX_FREE_DONE_TRIG,
				    dev->wlan.txfree_tbit));

		if (mtk_wed_get_rx_capa(dev))
			wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_RX,
				MTK_WED_WPDMA_INT_CTRL_RX0_EN |
				MTK_WED_WPDMA_INT_CTRL_RX0_CLR |
				MTK_WED_WPDMA_INT_CTRL_RX1_EN |
				MTK_WED_WPDMA_INT_CTRL_RX1_CLR |
				FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RX0_DONE_TRIG,
					   dev->wlan.rx_tbit[0]) |
				FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RX1_DONE_TRIG,
					   dev->wlan.rx_tbit[1]));
	}

	wed_w32(dev, MTK_WED_WDMA_INT_TRIGGER, wdma_mask);
	if (dev->hw->version == 1) {
		wed_clr(dev, MTK_WED_WDMA_INT_CTRL, wdma_mask);
	} else {
		wed_w32(dev, MTK_WED_WDMA_INT_CLR, wdma_mask);
		wed_set(dev, MTK_WED_WDMA_INT_CTRL,
			FIELD_PREP(MTK_WED_WDMA_INT_POLL_SRC_SEL,
				   dev->wdma_idx));
	}

	wdma_w32(dev, MTK_WDMA_INT_MASK, wdma_mask);
	wdma_w32(dev, MTK_WDMA_INT_GRP2, wdma_mask);
	wed_w32(dev, MTK_WED_WPDMA_INT_MASK, irq_mask);
	wed_w32(dev, MTK_WED_INT_MASK, irq_mask);
}

static void
mtk_wed_set_ext_int(struct mtk_wed_device *dev, bool en)
{
	u32 mask = MTK_WED_EXT_INT_STATUS_ERROR_MASK;

	switch (dev->hw->version) {
	case 1:
		mask |= MTK_WED_EXT_INT_STATUS_TX_DRV_R_RESP_ERR;
		break;
	case 2 :
		mask |= MTK_WED_EXT_INT_STATUS_RX_FBUF_LO_TH2 |
			MTK_WED_EXT_INT_STATUS_RX_FBUF_HI_TH2 |
			MTK_WED_EXT_INT_STATUS_RX_DRV_COHERENT |
			MTK_WED_EXT_INT_STATUS_TX_DMA_W_RESP_ERR;
		break;
	case 3:
		mask = MTK_WED_EXT_INT_STATUS_RX_DRV_COHERENT;
		break;
	}

	if (!dev->hw->num_flows)
		mask &= ~MTK_WED_EXT_INT_STATUS_TKID_WO_PYLD;

	wed_w32(dev, MTK_WED_EXT_INT_MASK, en ? mask : 0);
	wed_r32(dev, MTK_WED_EXT_INT_MASK);
}

static void
mtk_wed_pao_init(struct mtk_wed_device *dev)
{
	struct mtk_wed_pao *pao = dev->hw->wed_pao;
	int i;

	for (i = 0; i < 32; i++)
		wed_w32(dev, MTK_WED_PAO_HIFTXD_BASE_L(i),
			pao->hif_txd_phys[i]);

	/* init all sta parameter */
	wed_w32(dev, MTK_WED_PAO_STA_INFO_INIT, MTK_WED_PAO_STA_RMVL |
		MTK_WED_PAO_STA_WTBL_HDRT_MODE |
		FIELD_PREP(MTK_WED_PAO_STA_MAX_AMSDU_LEN,
			   dev->wlan.max_amsdu_len >> 8) |
		FIELD_PREP(MTK_WED_PAO_STA_MAX_AMSDU_NUM,
			   dev->wlan.max_amsdu_nums));

	wed_w32(dev, MTK_WED_PAO_STA_INFO, MTK_WED_PAO_STA_INFO_DO_INIT);

	if (mtk_wed_poll_busy(dev, MTK_WED_PAO_STA_INFO,
			      MTK_WED_PAO_STA_INFO_DO_INIT)) {
		dev_err(dev->hw->dev, "mtk_wed%d: pao init failed!\n",
			dev->hw->index);
		return;
	}

	/* init pao txd src */
	wed_set(dev, MTK_WED_PAO_HIFTXD_CFG,
		FIELD_PREP(MTK_WED_PAO_HIFTXD_SRC, dev->hw->index));

	/* init qmem */
	wed_set(dev, MTK_WED_PAO_PSE, MTK_WED_PAO_PSE_RESET);
	if (mtk_wed_poll_busy(dev, MTK_WED_PAO_MON_QMEM_STS1, BIT(29))) {
		pr_info("%s: init pao qmem fail\n", __func__);
		return;
	}

	/* eagle E1 PCIE1 tx ring 22 flow control issue */
	if (dev->wlan.chip_id == 0x7991) {
		wed_clr(dev, MTK_WED_PAO_AMSDU_FIFO,
			MTK_WED_PAO_AMSDU_IS_PRIOR0_RING);
	}

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_TX_PAO_EN);

	return;
}

static int
mtk_wed_hwrro_init(struct mtk_wed_device *dev)
{
	if (!mtk_wed_get_rx_capa(dev))
		return 0;

	wed_set(dev, MTK_WED_RRO_PG_BM_RX_DMAM,
		FIELD_PREP(MTK_WED_RRO_PG_BM_RX_SDL0, 128));

	wed_w32(dev, MTK_WED_RRO_PG_BM_BASE,
		dev->rx_page_buf_ring.desc_phys);

	wed_w32(dev, MTK_WED_RRO_PG_BM_INIT_PTR,
		MTK_WED_RRO_PG_BM_INIT_SW_TAIL_IDX |
		FIELD_PREP(MTK_WED_RRO_PG_BM_SW_TAIL_IDX,
			   MTK_WED_RX_PG_BM_CNT));

	/* enable rx_page_bm to fetch dmad */
	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_PG_BM_EN);

	return 0;
}

static int
mtk_wed_check_wfdma_rx_fill(struct mtk_wed_device *dev,
			   struct mtk_wed_ring *ring)
{
	int timeout = 3;
	u32 cur_idx;

	do {
		cur_idx = readl(ring->wpdma + MTK_WED_RING_OFS_CPU_IDX);
		if (cur_idx == MTK_WED_RX_RING_SIZE - 1)
			break;

		usleep_range(100000, 200000);
		timeout--;
	} while (timeout > 0);

	return timeout;
}


static void
mtk_wed_set_512_support(struct mtk_wed_device *dev, bool en)
{
	if (en) {
		wed_w32(dev, MTK_WED_TXDP_CTRL, MTK_WED_TXDP_DW9_OVERWR);
		wed_w32(dev, MTK_WED_TXP_DW1,
			FIELD_PREP(MTK_WED_WPDMA_WRITE_TXP, 0x0103));
	} else {
		wed_w32(dev, MTK_WED_TXP_DW1,
			FIELD_PREP(MTK_WED_WPDMA_WRITE_TXP, 0x0100));
		wed_clr(dev, MTK_WED_TXDP_CTRL, MTK_WED_TXDP_DW9_OVERWR);
	}
}

static void
mtk_wed_dma_enable(struct mtk_wed_device *dev)
{
#define MTK_WFMDA_RX_DMA_EN 	BIT(2)

	if (dev->hw->version == 1)
		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_SUBRT_ADV);

	wed_set(dev, MTK_WED_GLO_CFG,
		MTK_WED_GLO_CFG_TX_DMA_EN |
		MTK_WED_GLO_CFG_RX_DMA_EN);

	wed_set(dev, MTK_WED_WDMA_RX_PREF_CFG,
		FIELD_PREP(MTK_WED_WDMA_RX_PREF_BURST_SIZE, 0x10) |
		FIELD_PREP(MTK_WED_WDMA_RX_PREF_LOW_THRES, 0x8));
	wed_clr(dev, MTK_WED_WDMA_RX_PREF_CFG,
		MTK_WED_WDMA_RX_PREF_DDONE2_EN);

	wed_set(dev, MTK_WED_WDMA_RX_PREF_CFG, MTK_WED_WDMA_RX_PREF_EN);

	wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DDONE2_WR);
	wed_set(dev, MTK_WED_WDMA_GLO_CFG,
		MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);

	wdma_set(dev, MTK_WDMA_GLO_CFG,
		 MTK_WDMA_GLO_CFG_TX_DMA_EN /*|
		 MTK_WDMA_GLO_CFG_RX_INFO1_PRERES |
		 MTK_WDMA_GLO_CFG_RX_INFO2_PRERES*/);

	if (dev->hw->version == 1) {
		wdma_set(dev, MTK_WDMA_GLO_CFG,
			 MTK_WDMA_GLO_CFG_RX_INFO3_PRERES);
	} else {
		int idx = 0;

		if (mtk_wed_get_rx_capa(dev))
			wed_set(dev, MTK_WED_WDMA_GLO_CFG,
				MTK_WED_WDMA_GLO_CFG_TX_DRV_EN |
				MTK_WED_WDMA_GLO_CFG_TX_DDONE_CHK);

		wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_PKT_PROC |
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_CRX_SYNC);

		if (dev->hw->version == 3) {
			wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
				MTK_WED_WPDMA_GLO_CFG_TX_DDONE_CHK_LAST);
			wed_set(dev, MTK_WED_WPDMA_GLO_CFG,
				MTK_WED_WPDMA_GLO_CFG_TX_DDONE_CHK |
				MTK_WED_WPDMA_GLO_CFG_RX_DRV_EVENT_PKT_FMT_CHK |
				MTK_WED_WPDMA_GLO_CFG_RX_DRV_UNS_VER_FORCE_4);

			wdma_set(dev, MTK_WDMA_PREF_RX_CFG, MTK_WDMA_PREF_RX_CFG_PREF_EN);
			wdma_set(dev, MTK_WDMA_WRBK_RX_CFG, MTK_WDMA_WRBK_RX_CFG_WRBK_EN);
			if (mtk_wed_get_rx_capa(dev)) {
				wed_set(dev, MTK_WED_WPDMA_RX_D_PREF_CFG,
					MTK_WED_WPDMA_RX_D_PREF_EN |
					FIELD_PREP(MTK_WED_WPDMA_RX_D_PREF_BURST_SIZE, 0x10) |
					FIELD_PREP(MTK_WED_WPDMA_RX_D_PREF_LOW_THRES, 0x8));

				wed_set(dev, MTK_WED_RRO_RX_D_CFG(2), MTK_WED_RRO_RX_D_DRV_EN);

				wdma_set(dev, MTK_WDMA_PREF_TX_CFG, MTK_WDMA_PREF_TX_CFG_PREF_EN);

				wdma_set(dev, MTK_WDMA_WRBK_TX_CFG, MTK_WDMA_WRBK_TX_CFG_WRBK_EN);
			}
		}

		wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_TX_TKID_KEEP |
			MTK_WED_WPDMA_GLO_CFG_TX_DMAD_DW3_PREV);

		if (!mtk_wed_get_rx_capa(dev))
			return;

		wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG, MTK_WED_WPDMA_RX_D_RXD_READ_LEN);
		wed_set(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RX_DRV_EN |
			FIELD_PREP(MTK_WED_WPDMA_RX_D_RXD_READ_LEN, 0x18) |
			FIELD_PREP(MTK_WED_WPDMA_RX_D_INIT_PHASE_RXEN_SEL,
				   0x2));

		for (idx = 0; idx < dev->hw->ring_num; idx++) {
			struct mtk_wed_ring *ring = &dev->rx_ring[idx];

			if(!(ring->flags & MTK_WED_RING_CONFIGURED))
				continue;

			if(mtk_wed_check_wfdma_rx_fill(dev, ring)) {
				unsigned int val;

				val = wifi_r32(dev, dev->wlan.wpdma_rx_glo -
					       dev->wlan.phy_base);
				val |= MTK_WFMDA_RX_DMA_EN;

				wifi_w32(dev, dev->wlan.wpdma_rx_glo -
					 dev->wlan.phy_base, val);

				dev_err(dev->hw->dev, "mtk_wed%d: rx(%d) dma enable successful!\n",
						dev->hw->index, idx);
			} else {
				dev_err(dev->hw->dev, "mtk_wed%d: rx(%d) dma enable failed!\n",
					dev->hw->index, idx);
			}
		}
	}
}

static void
mtk_wed_dma_disable(struct mtk_wed_device *dev)
{
	wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN);

	wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
		MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);

	wed_clr(dev, MTK_WED_GLO_CFG,
		MTK_WED_GLO_CFG_TX_DMA_EN |
		MTK_WED_GLO_CFG_RX_DMA_EN);

	wdma_clr(dev, MTK_WDMA_GLO_CFG,
		 MTK_WDMA_GLO_CFG_TX_DMA_EN |
		 MTK_WDMA_GLO_CFG_RX_INFO1_PRERES |
		 MTK_WDMA_GLO_CFG_RX_INFO2_PRERES);

	if (dev->ver == MTK_WED_V1) {
		regmap_write(dev->hw->mirror, dev->hw->index * 4, 0);
		wdma_clr(dev, MTK_WDMA_GLO_CFG,
			 MTK_WDMA_GLO_CFG_RX_INFO3_PRERES);
	} else {
		wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_PKT_PROC |
			MTK_WED_WPDMA_GLO_CFG_RX_DRV_R0_CRX_SYNC);
		wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RX_DRV_EN);
		wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_TX_DDONE_CHK);

		if (dev->hw->version == 3 && mtk_wed_get_rx_capa(dev)) {
			wdma_clr(dev, MTK_WDMA_PREF_TX_CFG,
				 MTK_WDMA_PREF_TX_CFG_PREF_EN);
			wdma_clr(dev, MTK_WDMA_PREF_RX_CFG,
				 MTK_WDMA_PREF_RX_CFG_PREF_EN);
		}
	}
}

static void
mtk_wed_stop(struct mtk_wed_device *dev)
{
	if (mtk_wed_get_rx_capa(dev)) {
		wed_w32(dev, MTK_WED_EXT_INT_MASK1, 0);
		wed_w32(dev, MTK_WED_EXT_INT_MASK2, 0);
	}
	mtk_wed_set_ext_int(dev, false);

	wed_w32(dev, MTK_WED_WPDMA_INT_TRIGGER, 0);
	wed_w32(dev, MTK_WED_WDMA_INT_TRIGGER, 0);
	wdma_w32(dev, MTK_WDMA_INT_MASK, 0);
	wdma_w32(dev, MTK_WDMA_INT_GRP2, 0);
	wed_w32(dev, MTK_WED_WPDMA_INT_MASK, 0);
}

static void
mtk_wed_deinit(struct mtk_wed_device *dev)
{
	mtk_wed_stop(dev);
	mtk_wed_dma_disable(dev);

	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WPDMA_INT_AGENT_EN |
		MTK_WED_CTRL_WED_TX_BM_EN |
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	if (dev->hw->version == 1)
		return;

	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_RX_ROUTE_QM_EN |
		MTK_WED_CTRL_WED_RX_BM_EN |
		MTK_WED_CTRL_RX_RRO_QM_EN);

	if (dev->hw->version == 3) {
		wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_TX_PAO_EN);
		wed_clr(dev, MTK_WED_RESET, MTK_WED_RESET_TX_PAO);
		wed_clr(dev, MTK_WED_PCIE_INT_CTRL,
			MTK_WED_PCIE_INT_CTRL_MSK_EN_POLA |
			MTK_WED_PCIE_INT_CTRL_MSK_IRQ_FILTER);
	}
}

static void
mtk_wed_detach(struct mtk_wed_device *dev)
{
	struct device_node *wlan_node;
	struct mtk_wed_hw *hw = dev->hw;

	mutex_lock(&hw_lock);

	mtk_wed_deinit(dev);

	mtk_wdma_rx_reset(dev);

	mtk_wed_reset(dev, MTK_WED_RESET_WED);

	mtk_wdma_tx_reset(dev);

	mtk_wed_free_tx_buffer(dev);
	mtk_wed_free_tx_rings(dev);
	if (mtk_wed_get_rx_capa(dev)) {
		mtk_wed_wo_reset(dev);
		mtk_wed_free_rx_rings(dev);
		mtk_wed_wo_exit(hw);
	}

	if (dev->wlan.bus_type == MTK_WED_BUS_PCIE) {
		wlan_node = dev->wlan.pci_dev->dev.of_node;
		if (of_dma_is_coherent(wlan_node))
			regmap_update_bits(hw->hifsys, HIFSYS_DMA_AG_MAP,
					   BIT(hw->index), BIT(hw->index));
	}

	if ((!hw_list[!hw->index] || !hw_list[!hw->index]->wed_dev) &&
	    hw->eth->dma_dev != hw->eth->dev)
		mtk_eth_set_dma_device(hw->eth, hw->eth->dev);

	memset(dev, 0, sizeof(*dev));
	module_put(THIS_MODULE);

	hw->wed_dev = NULL;
	mutex_unlock(&hw_lock);
}

static void
mtk_wed_bus_init(struct mtk_wed_device *dev)
{
	switch (dev->wlan.bus_type) {
	case MTK_WED_BUS_PCIE: {
		struct device_node *np = dev->hw->eth->dev->of_node;
		struct regmap *regs;

		if (dev->hw->version == 2) {
			regs = syscon_regmap_lookup_by_phandle(np,
							       "mediatek,wed-pcie");
			if (IS_ERR(regs))
				break;

			regmap_update_bits(regs, 0, BIT(0), BIT(0));
		}

		if (dev->wlan.msi) {
		     wed_w32(dev, MTK_WED_PCIE_CFG_INTM, dev->hw->pci_base| 0xc08);
		     wed_w32(dev, MTK_WED_PCIE_CFG_BASE, dev->hw->pci_base | 0xc04);
		     wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER, BIT(8));
		} else {
		     wed_w32(dev, MTK_WED_PCIE_CFG_INTM, dev->hw->pci_base | 0x180);
		     wed_w32(dev, MTK_WED_PCIE_CFG_BASE, dev->hw->pci_base | 0x184);
		     wed_w32(dev, MTK_WED_PCIE_INT_TRIGGER, BIT(24));
		}

		wed_w32(dev, MTK_WED_PCIE_INT_CTRL,
			FIELD_PREP(MTK_WED_PCIE_INT_CTRL_POLL_EN, 2));

		/* pcie interrupt control: pola/source selection */
		wed_set(dev, MTK_WED_PCIE_INT_CTRL,
			MTK_WED_PCIE_INT_CTRL_MSK_EN_POLA |
			MTK_WED_PCIE_INT_CTRL_MSK_IRQ_FILTER |
			FIELD_PREP(MTK_WED_PCIE_INT_CTRL_SRC_SEL, dev->hw->index));

		break;
	}
	case MTK_WED_BUS_AXI:
		wed_set(dev, MTK_WED_WPDMA_INT_CTRL,
			MTK_WED_WPDMA_INT_CTRL_SIG_SRC |
			FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_SRC_SEL, 0));
		break;
	default:
		break;
	}

	return;
}

static void
mtk_wed_set_wpdma(struct mtk_wed_device *dev)
{
	if (dev->hw->version == 1) {
		wed_w32(dev, MTK_WED_WPDMA_CFG_BASE,  dev->wlan.wpdma_phys);
	} else {
		mtk_wed_bus_init(dev);

		wed_w32(dev, MTK_WED_WPDMA_CFG_BASE,  dev->wlan.wpdma_int);
		wed_w32(dev, MTK_WED_WPDMA_CFG_INT_MASK,  dev->wlan.wpdma_mask);
		wed_w32(dev, MTK_WED_WPDMA_CFG_TX, dev->wlan.wpdma_tx);
		wed_w32(dev, MTK_WED_WPDMA_CFG_TX_FREE,  dev->wlan.wpdma_txfree);

		if (mtk_wed_get_rx_capa(dev)) {
			int i;

			wed_w32(dev, MTK_WED_WPDMA_RX_GLO_CFG,  dev->wlan.wpdma_rx_glo);
			wed_w32(dev, MTK_WED_WPDMA_RX_RING0,  dev->wlan.wpdma_rx[0]);
			if (dev->wlan.wpdma_rx[1])
				wed_w32(dev, MTK_WED_WPDMA_RX_RING1,  dev->wlan.wpdma_rx[1]);

			if (dev->wlan.hwrro) {
				wed_w32(dev, MTK_WED_RRO_RX_D_CFG(0), dev->wlan.wpdma_rx_rro[0]);
				wed_w32(dev, MTK_WED_RRO_RX_D_CFG(1), dev->wlan.wpdma_rx_rro[1]);
				for (i = 0; i < MTK_WED_RX_PAGE_QUEUES; i++) {
					wed_w32(dev, MTK_WED_RRO_MSDU_PG_RING_CFG(i),
						dev->wlan.wpdma_rx_pg + i * 0x10);
			       }
			}
		}
	}
}

static void
mtk_wed_hw_init_early(struct mtk_wed_device *dev)
{
	u32 mask, set;

	mtk_wed_deinit(dev);
	mtk_wed_reset(dev, MTK_WED_RESET_WED);

	mtk_wed_set_wpdma(dev);

	if (dev->hw->version == 3) {
		mask = MTK_WED_WDMA_GLO_CFG_BT_SIZE;
		set = FIELD_PREP(MTK_WED_WDMA_GLO_CFG_BT_SIZE, 2);
	} else {
		mask = MTK_WED_WDMA_GLO_CFG_BT_SIZE |
		       MTK_WED_WDMA_GLO_CFG_DYNAMIC_DMAD_RECYCLE |
		       MTK_WED_WDMA_GLO_CFG_RX_DIS_FSM_AUTO_IDLE;
		set = FIELD_PREP(MTK_WED_WDMA_GLO_CFG_BT_SIZE, 2) |
		      MTK_WED_WDMA_GLO_CFG_DYNAMIC_SKIP_DMAD_PREP |
		      MTK_WED_WDMA_GLO_CFG_IDLE_DMAD_SUPPLY;
	}

	wed_m32(dev, MTK_WED_WDMA_GLO_CFG, mask, set);

	if (dev->hw->version == 1) {
		u32 offset;

		offset = dev->hw->index ? 0x04000400 : 0;
		wed_w32(dev, MTK_WED_WDMA_OFFSET0, 0x2a042a20 + offset);
		wed_w32(dev, MTK_WED_WDMA_OFFSET1, 0x29002800 + offset);

		wed_w32(dev, MTK_WED_PCIE_CFG_BASE, MTK_PCIE_BASE(dev->hw->index));
	} else {
		wed_w32(dev, MTK_WED_WDMA_CFG_BASE, dev->hw->wdma_phy);
		wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_ETH_DMAD_FMT);
		wed_w32(dev, MTK_WED_WDMA_OFFSET0,
			FIELD_PREP(MTK_WED_WDMA_OFST0_GLO_INTS,
				   MTK_WDMA_INT_STATUS) |
			FIELD_PREP(MTK_WED_WDMA_OFST0_GLO_CFG,
				   MTK_WDMA_GLO_CFG));

		wed_w32(dev, MTK_WED_WDMA_OFFSET1,
			FIELD_PREP(MTK_WED_WDMA_OFST1_TX_CTRL,
				   MTK_WDMA_RING_TX(0)) |
			FIELD_PREP(MTK_WED_WDMA_OFST1_RX_CTRL,
				   MTK_WDMA_RING_RX(0)));
	}
}

static void
mtk_wed_rx_bm_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RX_BM_RX_DMAD,
		FIELD_PREP(MTK_WED_RX_BM_RX_DMAD_SDL0,  dev->wlan.rx_size));

	wed_w32(dev, MTK_WED_RX_BM_BASE, dev->rx_buf_ring.desc_phys);

	wed_w32(dev, MTK_WED_RX_BM_INIT_PTR, MTK_WED_RX_BM_INIT_SW_TAIL |
		FIELD_PREP(MTK_WED_RX_BM_SW_TAIL, dev->wlan.rx_npkt));

	wed_w32(dev, MTK_WED_RX_BM_DYN_ALLOC_TH,
		FIELD_PREP(MTK_WED_RX_BM_DYN_ALLOC_TH_H, 0xffff));

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_BM_EN);
}

static void
mtk_wed_rro_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RROQM_MIOD_CFG,
		FIELD_PREP(MTK_WED_RROQM_MIOD_MID_DW, 0x70 >> 2) |
		FIELD_PREP(MTK_WED_RROQM_MIOD_MOD_DW, 0x10 >> 2) |
		FIELD_PREP(MTK_WED_RROQM_MIOD_ENTRY_DW,
			   MTK_WED_MIOD_ENTRY_CNT >> 2));

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL0, dev->rro.miod_desc_phys);

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL1,
		FIELD_PREP(MTK_WED_RROQM_MIOD_CNT, MTK_WED_MIOD_CNT));

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL0, dev->rro.fdbk_desc_phys);

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL1,
		FIELD_PREP(MTK_WED_RROQM_FDBK_CNT, MTK_WED_FB_CMD_CNT));

	wed_w32(dev, MTK_WED_RROQM_FDBK_CTRL2, 0);

	wed_w32(dev, MTK_WED_RROQ_BASE_L, dev->rro.rro_ring.desc_phys);

	wed_set(dev, MTK_WED_RROQM_RST_IDX,
		MTK_WED_RROQM_RST_IDX_MIOD |
		MTK_WED_RROQM_RST_IDX_FDBK);

	wed_w32(dev, MTK_WED_RROQM_RST_IDX, 0);

	wed_w32(dev, MTK_WED_RROQM_MIOD_CTRL2, MTK_WED_MIOD_CNT -1);

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_RRO_QM_EN);
}

static void
mtk_wed_route_qm_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_RESET, MTK_WED_RESET_RX_ROUTE_QM);

	do {
		udelay(100);

		if (!(wed_r32(dev, MTK_WED_RESET) & MTK_WED_RESET_RX_ROUTE_QM))
			break;
	} while (1);

	/* configure RX_ROUTE_QM */
	if (dev->hw->version == 2) {
		wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_Q_RST);
		wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_TXDMAD_FPORT);
		wed_set(dev, MTK_WED_RTQM_GLO_CFG,
			FIELD_PREP(MTK_WED_RTQM_TXDMAD_FPORT, 0x3 + dev->hw->index));
		wed_clr(dev, MTK_WED_RTQM_GLO_CFG, MTK_WED_RTQM_Q_RST);
	} else {
		wed_set(dev, MTK_WED_RTQM_ENQ_CFG0,
			FIELD_PREP(MTK_WED_RTQM_ENQ_CFG_TXDMAD_FPORT, 0x3 + dev->hw->index));
	}

	/* enable RX_ROUTE_QM */
	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_ROUTE_QM_EN);
}

static void
mtk_wed_tx_hw_init(struct mtk_wed_device *dev)
{
	int size = dev->wlan.nbuf;
	int rev_size = MTK_WED_TX_RING_SIZE / 2;
	int thr_lo = 1, thr_hi = 1;

	if (dev->hw->version == 1) {
		wed_w32(dev, MTK_WED_TX_BM_CTRL,
			MTK_WED_TX_BM_CTRL_PAUSE |
			FIELD_PREP(MTK_WED_TX_BM_CTRL_VLD_GRP_NUM, size / 128) |
			FIELD_PREP(MTK_WED_TX_BM_CTRL_RSV_GRP_NUM, rev_size / 128));
	} else {
		size = MTK_WED_WDMA_RING_SIZE * ARRAY_SIZE(dev->tx_wdma) +
		       dev->tx_buf_ring.size;
		rev_size = size;
		thr_lo = 0;
		thr_hi = MTK_WED_TX_BM_DYN_THR_HI;

		wed_w32(dev, MTK_WED_TX_TKID_CTRL,
			MTK_WED_TX_TKID_CTRL_PAUSE |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_VLD_GRP_NUM,
				   size / 128) |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_RSV_GRP_NUM,
				   size / 128));

		/* return SKBID + SDP back to bm */
		if (dev->ver == 3) {
			wed_set(dev, MTK_WED_TX_TKID_CTRL,
				MTK_WED_TX_TKID_CTRL_FREE_FORMAT);
			 size = dev->wlan.nbuf;
			 rev_size = size;
		} else {
			wed_w32(dev, MTK_WED_TX_TKID_DYN_THR,
				FIELD_PREP(MTK_WED_TX_TKID_DYN_THR_LO, 0) |
				MTK_WED_TX_TKID_DYN_THR_HI);
		}
	}

	mtk_wed_reset(dev, MTK_WED_RESET_TX_BM);

	wed_w32(dev, MTK_WED_TX_BM_BASE, dev->tx_buf_ring.desc_phys);

	wed_w32(dev, MTK_WED_TX_BM_TKID,
		FIELD_PREP(MTK_WED_TX_BM_TKID_START,
			   dev->wlan.token_start) |
		FIELD_PREP(MTK_WED_TX_BM_TKID_END,
			   dev->wlan.token_start + dev->wlan.nbuf - 1));

	wed_w32(dev, MTK_WED_TX_BM_BUF_LEN, MTK_WED_PKT_SIZE);

	if (dev->hw->version < 3)
		wed_w32(dev, MTK_WED_TX_BM_DYN_THR,
			FIELD_PREP(MTK_WED_TX_BM_DYN_THR_LO, thr_lo) |
			FIELD_PREP(MTK_WED_TX_BM_DYN_THR_LO, thr_hi));
	else {
		/* change to new bm */
		wed_w32(dev, MTK_WED_TX_BM_INIT_PTR, dev->tx_buf_ring.pkt_nums |
			MTK_WED_TX_BM_INIT_SW_TAIL_IDX);
		wed_clr(dev, MTK_WED_TX_BM_CTRL, MTK_WED_TX_BM_CTRL_LEGACY_EN);
	}

	if (dev->hw->version != 1) {
		wed_w32(dev, MTK_WED_TX_TKID_CTRL,
			MTK_WED_TX_TKID_CTRL_PAUSE |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_VLD_GRP_NUM,
				   size / 128) |
			FIELD_PREP(MTK_WED_TX_TKID_CTRL_RSV_GRP_NUM,
				   size / 128));

		/* return SKBID + SDP back to bm */
		if (dev->ver == 3)
			wed_set(dev, MTK_WED_TX_TKID_CTRL,
				MTK_WED_TX_TKID_CTRL_FREE_FORMAT);
		else
			wed_w32(dev, MTK_WED_TX_TKID_DYN_THR,
				FIELD_PREP(MTK_WED_TX_TKID_DYN_THR_LO, 0) |
				MTK_WED_TX_TKID_DYN_THR_HI);
	}
	wed_w32(dev, MTK_WED_TX_BM_TKID,
		FIELD_PREP(MTK_WED_TX_BM_TKID_START,
			   dev->wlan.token_start) |
		FIELD_PREP(MTK_WED_TX_BM_TKID_END,
			   dev->wlan.token_start + dev->wlan.nbuf - 1));

	wed_w32(dev, MTK_WED_TX_BM_INIT_PTR, dev->tx_buf_ring.pkt_nums |
		MTK_WED_TX_BM_INIT_SW_TAIL_IDX);
	wed_clr(dev, MTK_WED_TX_BM_CTRL, MTK_WED_TX_BM_CTRL_PAUSE);
	if (dev->hw->version != 1)
		wed_clr(dev, MTK_WED_TX_TKID_CTRL, MTK_WED_TX_TKID_CTRL_PAUSE);
}

static void
mtk_wed_rx_hw_init(struct mtk_wed_device *dev)
{
	wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX,
		MTK_WED_WPDMA_RX_D_RST_CRX_IDX |
		MTK_WED_WPDMA_RX_D_RST_DRV_IDX);

	wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX, 0);

	/* reset prefetch index of ring */
	wed_set(dev, MTK_WED_WPDMA_RX_D_PREF_RX0_SIDX,
		MTK_WED_WPDMA_RX_D_PREF_SIDX_IDX_CLR);
	wed_clr(dev, MTK_WED_WPDMA_RX_D_PREF_RX0_SIDX,
		MTK_WED_WPDMA_RX_D_PREF_SIDX_IDX_CLR);

	wed_set(dev, MTK_WED_WPDMA_RX_D_PREF_RX1_SIDX,
		MTK_WED_WPDMA_RX_D_PREF_SIDX_IDX_CLR);
	wed_clr(dev, MTK_WED_WPDMA_RX_D_PREF_RX1_SIDX,
		MTK_WED_WPDMA_RX_D_PREF_SIDX_IDX_CLR);

	/* reset prefetch FIFO of ring */
	wed_set(dev, MTK_WED_WPDMA_RX_D_PREF_FIFO_CFG,
		MTK_WED_WPDMA_RX_D_PREF_FIFO_CFG_R0_CLR |
		MTK_WED_WPDMA_RX_D_PREF_FIFO_CFG_R1_CLR);
	wed_w32(dev, MTK_WED_WPDMA_RX_D_PREF_FIFO_CFG, 0);

	mtk_wed_rx_bm_hw_init(dev);
	if (dev->wlan.hwrro)
		mtk_wed_hwrro_init(dev);
	mtk_wed_rro_hw_init(dev);
	mtk_wed_route_qm_hw_init(dev);
}

static void
mtk_wed_hw_init(struct mtk_wed_device *dev)
{
	if (dev->init_done)
		return;

	dev->init_done = true;
	mtk_wed_set_ext_int(dev, false);
	mtk_wed_tx_hw_init(dev);
	if (mtk_wed_get_rx_capa(dev))
		mtk_wed_rx_hw_init(dev);
}

static void
mtk_wed_ring_reset(struct mtk_wdma_desc *desc, int size, int scale, bool tx)
{
	__le32 ctrl;
	int i;

	if (tx)
		ctrl = cpu_to_le32(MTK_WDMA_DESC_CTRL_DMA_DONE);
	else
		ctrl = cpu_to_le32(MTK_WFDMA_DESC_CTRL_TO_HOST);

	for (i = 0; i < size; i++) {
		desc->buf0 = 0;
		desc->ctrl = ctrl;
		desc->buf1 = 0;
		desc->info = 0;
		desc += scale;
	}
}

static void
mtk_wed_rx_reset(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	u8 state = WO_STATE_SER_RESET;
	bool busy = false;
	int i;

	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), true);

	if (dev->wlan.hwrro) {
		wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_IND_CMD_EN);
		mtk_wed_poll_busy(dev, MTK_WED_RRO_RX_HW_STS,
				  MTK_WED_RX_IND_CMD_BUSY);
		mtk_wed_reset(dev, MTK_WED_RESET_RRO_RX_TO_PG);
	}
	wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG, MTK_WED_WPDMA_RX_D_RX_DRV_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
				 MTK_WED_WPDMA_RX_D_RX_DRV_BUSY);
	if (dev->hw->version == 3)
		busy = mtk_wed_poll_busy(dev, MTK_WED_WPDMA_RX_D_PREF_CFG,
					 MTK_WED_WPDMA_RX_D_PREF_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_RX_D_DRV);
	} else {
		if (dev->hw->version == 3) {
			/*1.a. Disable Prefetch HW*/
			wed_clr(dev, MTK_WED_WPDMA_RX_D_PREF_CFG, MTK_WED_WPDMA_RX_D_PREF_EN);
			mtk_wed_poll_busy(dev, MTK_WED_WPDMA_RX_D_PREF_CFG,
					  MTK_WED_WPDMA_RX_D_PREF_BUSY);
			wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX,
				MTK_WED_WPDMA_RX_D_RST_DRV_IDX_ALL);
		}
		wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX,
			MTK_WED_WPDMA_RX_D_RST_CRX_IDX |
			MTK_WED_WPDMA_RX_D_RST_DRV_IDX);

		wed_set(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RST_INIT_COMPLETE |
			MTK_WED_WPDMA_RX_D_FSM_RETURN_IDLE);
		wed_clr(dev, MTK_WED_WPDMA_RX_D_GLO_CFG,
			MTK_WED_WPDMA_RX_D_RST_INIT_COMPLETE |
			MTK_WED_WPDMA_RX_D_FSM_RETURN_IDLE);

		wed_w32(dev, MTK_WED_WPDMA_RX_D_RST_IDX, 0);
	}

	/* reset rro qm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_RRO_QM_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				 MTK_WED_CTRL_RX_RRO_QM_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_RX_RRO_QM);
	} else {
		wed_set(dev, MTK_WED_RROQM_RST_IDX,
			MTK_WED_RROQM_RST_IDX_MIOD |
			MTK_WED_RROQM_RST_IDX_FDBK);
		wed_w32(dev, MTK_WED_RROQM_RST_IDX, 0);
	}

	if (dev->wlan.hwrro) {
		/* Disable RRO MSDU Page Drv */
		wed_clr(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG, MTK_WED_RRO_MSDU_PG_DRV_EN);

		/* Disable RRO Data Drv */
		wed_clr(dev, MTK_WED_RRO_RX_D_CFG(2), MTK_WED_RRO_RX_D_DRV_EN);

		/* RRO MSDU Page Drv Reset */
		wed_w32(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG, MTK_WED_RRO_MSDU_PG_DRV_CLR);
		mtk_wed_poll_busy(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG,
				  MTK_WED_RRO_MSDU_PG_DRV_CLR);

		/* RRO Data Drv Reset */
		wed_w32(dev, MTK_WED_RRO_RX_D_CFG(2), MTK_WED_RRO_RX_D_DRV_CLR);
		mtk_wed_poll_busy(dev, MTK_WED_RRO_RX_D_CFG(2),
				  MTK_WED_RRO_RX_D_DRV_CLR);
	}

	/* reset route qm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_RX_ROUTE_QM_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				 MTK_WED_CTRL_RX_ROUTE_QM_BUSY);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_RX_ROUTE_QM);
	} else {
		if (dev->hw->version == 3) {
			wed_set(dev, MTK_WED_RTQM_RST, BIT(0));
			wed_clr(dev, MTK_WED_RTQM_RST, BIT(0));
			mtk_wed_reset(dev, MTK_WED_RESET_RX_ROUTE_QM);
		} else
			wed_set(dev, MTK_WED_RTQM_GLO_CFG,
				MTK_WED_RTQM_Q_RST);
	}

	/* reset tx wdma */
	mtk_wdma_tx_reset(dev);

	/* reset tx wdma drv */
	wed_clr(dev, MTK_WED_WDMA_GLO_CFG, MTK_WED_WDMA_GLO_CFG_TX_DRV_EN);
	if (dev->hw->version == 3)
		mtk_wed_poll_busy(dev, MTK_WED_WPDMA_STATUS,
				  MTK_WED_WPDMA_STATUS_TX_DRV);
	else
		mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				  MTK_WED_CTRL_WDMA_INT_AGENT_BUSY);

	mtk_wed_reset(dev, MTK_WED_RESET_WDMA_TX_DRV);

	/* reset wed rx dma */
	busy = mtk_wed_poll_busy(dev, MTK_WED_GLO_CFG,
				 MTK_WED_GLO_CFG_RX_DMA_BUSY);
	wed_clr(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_RX_DMA_EN);
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WED_RX_DMA);
	} else {
		wed_set(dev, MTK_WED_RESET_IDX,
			MTK_WED_RESET_IDX_RX);
		wed_w32(dev, MTK_WED_RESET_IDX, 0);
	}

	/* reset rx bm */
	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_BM_EN);
	mtk_wed_poll_busy(dev, MTK_WED_CTRL,
			  MTK_WED_CTRL_WED_RX_BM_BUSY);
	mtk_wed_reset(dev, MTK_WED_RESET_RX_BM);

	if (dev->wlan.hwrro) {
		wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_PG_BM_EN);
		mtk_wed_poll_busy(dev, MTK_WED_CTRL,
				  MTK_WED_CTRL_WED_RX_PG_BM_BUSY);
		wed_set(dev, MTK_WED_RESET, MTK_WED_RESET_RX_PG_BM);
		wed_clr(dev, MTK_WED_RESET, MTK_WED_RESET_RX_PG_BM);
	}

	/* wo change to enable state */
	state = WO_STATE_ENABLE;
	mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_CHANGE_STATE,
			     &state, sizeof(state), true);

	/* wed_rx_ring_reset */
	for (i = 0; i < ARRAY_SIZE(dev->rx_ring); i++) {
		struct mtk_wdma_desc *desc = dev->rx_ring[i].desc;

		if (!desc)
			continue;

		mtk_wed_ring_reset(desc, MTK_WED_RX_RING_SIZE, 1, false);
	}

	mtk_wed_free_rx_buffer(dev);

	if (dev->wlan.hwrro)
		mtk_wed_rx_page_free_buffer(dev);
}


static void
mtk_wed_reset_dma(struct mtk_wed_device *dev)
{
	bool busy = false;
	u32 val;
	int i;

	for (i = 0; i < ARRAY_SIZE(dev->tx_ring); i++) {
		struct mtk_wdma_desc *desc = dev->tx_ring[i].desc;

		if (!desc)
			continue;

		mtk_wed_ring_reset(desc, MTK_WED_TX_RING_SIZE, 1, true);
	}

	/* 1.Reset WED Tx DMA */
	wed_clr(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_TX_DMA_EN);
	busy = mtk_wed_poll_busy(dev, MTK_WED_GLO_CFG, MTK_WED_GLO_CFG_TX_DMA_BUSY);

	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WED_TX_DMA);
	} else {
		wed_w32(dev, MTK_WED_RESET_IDX,
			MTK_WED_RESET_IDX_TX);
		wed_w32(dev, MTK_WED_RESET_IDX, 0);
	}

	/* 2. Reset WDMA Rx DMA/Driver_Engine */
	busy = !!mtk_wdma_rx_reset(dev);
	if (dev->hw->version == 3) {
		val = wed_r32(dev, MTK_WED_WDMA_GLO_CFG);
		val |= MTK_WED_WDMA_GLO_CFG_RX_DIS_FSM_AUTO_IDLE;
		val &= ~MTK_WED_WDMA_GLO_CFG_RX_DRV_EN;
		wed_w32(dev, MTK_WED_WDMA_GLO_CFG, val);
	} else
		wed_clr(dev, MTK_WED_WDMA_GLO_CFG, MTK_WED_WDMA_GLO_CFG_RX_DRV_EN);

	busy = !!(busy ||
		  mtk_wed_poll_busy(dev, MTK_WED_WDMA_GLO_CFG,
				    MTK_WED_WDMA_GLO_CFG_RX_DRV_BUSY));
	if (dev->hw->version == 3)
		busy = !!(busy ||
			  mtk_wed_poll_busy(dev, MTK_WED_WDMA_RX_PREF_CFG,
					    MTK_WED_WDMA_RX_PREF_BUSY));

	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WDMA_RX_DRV);
	} else {
		if (dev->hw->version == 3) {
			/*1.a. Disable Prefetch HW*/
			wed_clr(dev, MTK_WED_WDMA_RX_PREF_CFG, MTK_WED_WDMA_RX_PREF_EN);
			mtk_wed_poll_busy(dev, MTK_WED_WDMA_RX_PREF_CFG,
					  MTK_WED_WDMA_RX_PREF_BUSY);
			wed_clr(dev, MTK_WED_WDMA_RX_PREF_CFG, MTK_WED_WDMA_RX_PREF_DDONE2_EN);

			/* reset prefetch index */
			wed_set(dev, MTK_WED_WDMA_RX_PREF_CFG,
				MTK_WED_WDMA_RX_PREF_RX0_SIDX_CLR |
				MTK_WED_WDMA_RX_PREF_RX1_SIDX_CLR);

			wed_clr(dev, MTK_WED_WDMA_RX_PREF_CFG,
				MTK_WED_WDMA_RX_PREF_RX0_SIDX_CLR |
				MTK_WED_WDMA_RX_PREF_RX1_SIDX_CLR);

			/* reset prefetch FIFO */
			wed_w32(dev, MTK_WED_WDMA_RX_PREF_FIFO_CFG,
				MTK_WED_WDMA_RX_PREF_FIFO_RX0_CLR |
				MTK_WED_WDMA_RX_PREF_FIFO_RX1_CLR);
			wed_w32(dev, MTK_WED_WDMA_RX_PREF_FIFO_CFG, 0);
			/*2. Reset dma index*/
			wed_w32(dev, MTK_WED_WDMA_RESET_IDX,
				MTK_WED_WDMA_RESET_IDX_RX_ALL);
		}
		wed_w32(dev, MTK_WED_WDMA_RESET_IDX,
			MTK_WED_WDMA_RESET_IDX_RX |
			MTK_WED_WDMA_RESET_IDX_DRV);
		wed_w32(dev, MTK_WED_WDMA_RESET_IDX, 0);

		wed_set(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_RST_INIT_COMPLETE);

		wed_clr(dev, MTK_WED_WDMA_GLO_CFG,
			MTK_WED_WDMA_GLO_CFG_RST_INIT_COMPLETE);
	}

	/* 3. Reset WED WPDMA Tx Driver Engine */
	wed_clr(dev, MTK_WED_CTRL,
		MTK_WED_CTRL_WED_TX_FREE_AGENT_EN);

	for (i = 0; i < 100; i++) {
		if (dev->ver > MTK_WED_V1) {
			val = wed_r32(dev, MTK_WED_TX_TKID_INTF);
			if (FIELD_GET(MTK_WED_TX_TKID_INTF_TKFIFO_FDEP, val) == 0x40)
				break;
		} else {
			val = wed_r32(dev, MTK_WED_TX_BM_INTF);
			if (FIELD_GET(MTK_WED_TX_BM_INTF_TKFIFO_FDEP, val) == 0x40)
				break;
		}
	}
	mtk_wed_reset(dev, MTK_WED_RESET_TX_FREE_AGENT);

	wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_TX_BM_EN);
	mtk_wed_reset(dev, MTK_WED_RESET_TX_BM);

	/* 4. Reset WED WPDMA Tx Driver Engine */
	busy = mtk_wed_poll_busy(dev, MTK_WED_WPDMA_GLO_CFG,
				 MTK_WED_WPDMA_GLO_CFG_TX_DRV_BUSY);
	wed_clr(dev, MTK_WED_WPDMA_GLO_CFG,
		MTK_WED_WPDMA_GLO_CFG_TX_DRV_EN |
		MTK_WED_WPDMA_GLO_CFG_RX_DRV_EN);

	busy = !!(busy ||
		  mtk_wed_poll_busy(dev, MTK_WED_WPDMA_GLO_CFG,
				    MTK_WED_WPDMA_GLO_CFG_RX_DRV_BUSY));
	if (busy) {
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_INT_AGENT);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_TX_DRV);
		mtk_wed_reset(dev, MTK_WED_RESET_WPDMA_RX_DRV);
		if (dev->hw->version == 3)
			wed_w32(dev, MTK_WED_RX1_CTRL2, 0);
	} else {
		wed_w32(dev, MTK_WED_WPDMA_RESET_IDX,
			MTK_WED_WPDMA_RESET_IDX_TX |
			MTK_WED_WPDMA_RESET_IDX_RX);
		wed_w32(dev, MTK_WED_WPDMA_RESET_IDX, 0);
		if (dev->ver > MTK_WED_V1) {
			wed_w32(dev, MTK_WED_RESET_IDX,
				MTK_WED_RESET_WPDMA_IDX_RX);
			wed_w32(dev, MTK_WED_RESET_IDX, 0);
		}
	}

	dev->init_done = false;

	if (dev->hw->version == 3) {
		/*reset wed pao*/
		wed_clr(dev, MTK_WED_CTRL, MTK_WED_CTRL_TX_PAO_EN);
		mtk_wed_reset(dev, MTK_WED_RESET_TX_PAO);
	}

	if (mtk_wed_get_rx_capa(dev))
		mtk_wed_rx_reset(dev);

}

static int
mtk_wed_rro_ring_alloc(struct mtk_wed_device *dev, struct mtk_wed_ring *ring,
		   int size)
{
	ring->desc = dma_alloc_coherent(dev->hw->dev,
					size * sizeof(*ring->desc),
					&ring->desc_phys, GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	ring->size = size;
	memset(ring->desc, 0, size);
	return 0;
}

static int
mtk_wed_ring_alloc(struct mtk_wed_device *dev, struct mtk_wed_ring *ring,
		   int size, int scale, bool tx)
{
	ring->desc = dma_alloc_coherent(dev->hw->dev,
					size * sizeof(*ring->desc) * scale,
					&ring->desc_phys, GFP_KERNEL);
	if (!ring->desc)
		return -ENOMEM;

	ring->size = size;
	mtk_wed_ring_reset(ring->desc, size, scale, tx);

	return 0;
}

static int
mtk_wed_wdma_rx_ring_setup(struct mtk_wed_device *dev,
		int idx, int size, bool reset)
{
	struct mtk_wed_ring *wdma = &dev->tx_wdma[idx];
	int scale = dev->hw->version > 1 ? 2 : 1;

	if(!reset)
		if (mtk_wed_ring_alloc(dev, wdma, MTK_WED_WDMA_RING_SIZE,
				       scale, true))
			return -ENOMEM;

	wdma->flags |= MTK_WED_RING_CONFIGURED;

	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_BASE,
		 wdma->desc_phys);
	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_COUNT,
		 size);
	wdma_w32(dev, MTK_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);

	wed_w32(dev, MTK_WED_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_BASE,
		wdma->desc_phys);
	wed_w32(dev, MTK_WED_WDMA_RING_RX(idx) + MTK_WED_RING_OFS_COUNT,
		size);

	return 0;
}

static int
mtk_wed_wdma_tx_ring_setup(struct mtk_wed_device *dev,
	int idx, int size, bool reset)
{
	struct mtk_wed_ring *wdma = &dev->rx_wdma[idx];
	int scale = dev->hw->version > 1 ? 2 : 1;

	if (!reset)
		if (mtk_wed_ring_alloc(dev, wdma, MTK_WED_WDMA_RING_SIZE,
				       scale, true))
			return -ENOMEM;

	if (dev->hw->version == 3) {
		struct mtk_wdma_desc *desc = wdma->desc;
		int i;

		for (i = 0; i < MTK_WED_WDMA_RING_SIZE; i++) {
			desc->buf0 = 0;
			desc->ctrl = MTK_WDMA_DESC_CTRL_DMA_DONE;
			desc->buf1 = 0;
			desc->info = MTK_WDMA_TXD0_DESC_INFO_DMA_DONE;
			desc++;
			desc->buf0 = 0;
			desc->ctrl = MTK_WDMA_DESC_CTRL_DMA_DONE;
			desc->buf1 = 0;
			desc->info = MTK_WDMA_TXD1_DESC_INFO_DMA_DONE;
			desc++;
		}
	}

	wdma->flags |= MTK_WED_RING_CONFIGURED;

	wdma_w32(dev, MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_BASE,
		 wdma->desc_phys);
	wdma_w32(dev, MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_COUNT,
		 size);
	wdma_w32(dev,
		 MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);
	wdma_w32(dev,
		 MTK_WDMA_RING_TX(idx) + MTK_WED_RING_OFS_DMA_IDX, 0);
	if (reset)
		mtk_wed_ring_reset(wdma->desc, MTK_WED_WDMA_RING_SIZE,
				   scale, true);
	if (idx == 0)  {
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_BASE, wdma->desc_phys);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_COUNT, size);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_CPU_IDX, 0);
		wed_w32(dev, MTK_WED_WDMA_RING_TX
			+ MTK_WED_RING_OFS_DMA_IDX, 0);
	}

	return 0;
}

static int
mtk_wed_rro_alloc(struct mtk_wed_device *dev)
{
	struct device_node *np, *node = dev->hw->node;
	struct mtk_wed_ring *ring;
	struct resource res;
	int ret;

	np = of_parse_phandle(node, "mediatek,wocpu_dlm", 0);
	if (!np)
		return -ENODEV;

	ret = of_address_to_resource(np, 0, &res);
	if (ret)
		return ret;

	dev->rro.rro_desc = ioremap(res.start, resource_size(&res));

	ring = &dev->rro.rro_ring;

	dev->rro.miod_desc_phys = res.start;

	dev->rro.mcu_view_miod = MTK_WED_WOCPU_VIEW_MIOD_BASE;
	dev->rro.fdbk_desc_phys = MTK_WED_MIOD_ENTRY_CNT * MTK_WED_MIOD_CNT
				  + dev->rro.miod_desc_phys;

	if (mtk_wed_rro_ring_alloc(dev, ring, MTK_WED_RRO_QUE_CNT))
		return -ENOMEM;

	return 0;
}

static int
mtk_wed_rro_cfg(struct mtk_wed_device *dev)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;
	struct {
		struct wo_cmd_ring ring[2];

		u32 wed;
		u8 ver;
	} req = {
		.ring = {
			[0] = {
				.q_base = dev->rro.mcu_view_miod,
				.cnt = MTK_WED_MIOD_CNT,
				.unit = MTK_WED_MIOD_ENTRY_CNT,
			},
			[1] = {
				.q_base = dev->rro.mcu_view_miod +
					  MTK_WED_MIOD_ENTRY_CNT *
					  MTK_WED_MIOD_CNT,
				.cnt = MTK_WED_FB_CMD_CNT,
				.unit = 4,
			},
		},
		.wed = 0,
	};

	return mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, MTK_WED_WO_CMD_WED_CFG,
				    &req, sizeof(req), true);
}

static int
mtk_wed_send_msg(struct mtk_wed_device *dev, int cmd_id, void *data, int len)
{
	struct mtk_wed_wo *wo = dev->hw->wed_wo;

	if (!mtk_wed_get_rx_capa(dev))
		return 0;

	return mtk_wed_mcu_send_msg(wo, MODULE_ID_WO, cmd_id, data, len, true);
}

static void
mtk_wed_ppe_check(struct mtk_wed_device *dev, struct sk_buff *skb,
			u32 reason, u32 hash)
{
	int idx = dev->hw->index;
	struct mtk_eth *eth = dev->hw->eth;
	struct ethhdr *eh;

	if (reason == MTK_PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED) {
		if (!skb)
			return;

		skb_set_mac_header(skb, 0);
		eh = eth_hdr(skb);
		skb->protocol = eh->h_proto;
		mtk_ppe_check_skb(eth->ppe[idx], skb, hash);
	}
}

static void
mtk_wed_start_hwrro(struct mtk_wed_device *dev, u32 irq_mask, bool reset)
{
	int idx, ret;

	wed_w32(dev, MTK_WED_WPDMA_INT_MASK, irq_mask);
	wed_w32(dev, MTK_WED_INT_MASK, irq_mask);

	if (!mtk_wed_get_rx_capa(dev) || !dev->wlan.hwrro)
		return;

	if (reset) {
		wed_set(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG, MTK_WED_RRO_MSDU_PG_DRV_EN);
		return;
	}
	
	wed_set(dev, MTK_WED_RRO_RX_D_CFG(2), MTK_WED_RRO_MSDU_PG_DRV_CLR);
	wed_w32(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG, MTK_WED_RRO_MSDU_PG_DRV_CLR);

	wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_RRO_RX,
		MTK_WED_WPDMA_INT_CTRL_RRO_RX0_EN |
		MTK_WED_WPDMA_INT_CTRL_RRO_RX0_CLR |
		MTK_WED_WPDMA_INT_CTRL_RRO_RX1_EN |
		MTK_WED_WPDMA_INT_CTRL_RRO_RX1_CLR |
		FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RRO_RX0_DONE_TRIG,
			   dev->wlan.rro_rx_tbit[0]) |
		FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RRO_RX1_DONE_TRIG,
			   dev->wlan.rro_rx_tbit[1]));

	wed_w32(dev, MTK_WED_WPDMA_INT_CTRL_RRO_MSDU_PG,
		MTK_WED_WPDMA_INT_CTRL_RRO_PG0_EN |
		MTK_WED_WPDMA_INT_CTRL_RRO_PG0_CLR |
		MTK_WED_WPDMA_INT_CTRL_RRO_PG1_EN |
		MTK_WED_WPDMA_INT_CTRL_RRO_PG1_CLR |
		MTK_WED_WPDMA_INT_CTRL_RRO_PG2_EN |
		MTK_WED_WPDMA_INT_CTRL_RRO_PG2_CLR |
		FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RRO_PG0_DONE_TRIG,
			   dev->wlan.rx_pg_tbit[0]) |
		FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RRO_PG1_DONE_TRIG,
			   dev->wlan.rx_pg_tbit[1])|
		FIELD_PREP(MTK_WED_WPDMA_INT_CTRL_RRO_PG2_DONE_TRIG,
			   dev->wlan.rx_pg_tbit[2]));

	/*
	 * RRO_MSDU_PG_RING2_CFG1_FLD_DRV_EN should be enabled after
	 * WM FWDL completed, otherwise RRO_MSDU_PG ring may broken
	 */
	wed_set(dev, MTK_WED_RRO_MSDU_PG_RING2_CFG, MTK_WED_RRO_MSDU_PG_DRV_EN);

	for (idx = 0; idx < MTK_WED_RX_QUEUES; idx++) {
		struct mtk_wed_ring *ring = &dev->rx_rro_ring[idx];

		if(!(ring->flags & MTK_WED_RING_CONFIGURED))
			continue;

		ret = mtk_wed_check_wfdma_rx_fill(dev, ring);
		if (!ret)
			dev_err(dev->hw->dev, "mtk_wed%d: rx_rro_ring(%d) init failed!\n",
				dev->hw->index, idx);
	}

	for (idx = 0; idx < MTK_WED_RX_PAGE_QUEUES; idx++){
		struct mtk_wed_ring *ring = &dev->rx_page_ring[idx];
		if(!(ring->flags & MTK_WED_RING_CONFIGURED))
			continue;

		ret = mtk_wed_check_wfdma_rx_fill(dev, ring);
		if (!ret)
			dev_err(dev->hw->dev, "mtk_wed%d: rx_page_ring(%d) init failed!\n",
				dev->hw->index, idx);
	}
}

static void
mtk_wed_start(struct mtk_wed_device *dev, u32 irq_mask)
{
	int i, ret;

	if (mtk_wed_get_rx_capa(dev)) {
		ret = mtk_wed_rx_buffer_alloc(dev);
		if (ret)
			return;

		if (dev->wlan.hwrro)
			mtk_wed_rx_page_buffer_alloc(dev);
	}

	for (i = 0; i < ARRAY_SIZE(dev->tx_wdma); i++)
		if (!dev->tx_wdma[i].desc)
			mtk_wed_wdma_rx_ring_setup(dev, i, 16, false);

	for (i = 0; i < ARRAY_SIZE(dev->rx_page_ring); i++) {
		u32 count = MTK_WED_RRO_MSDU_PG_CTRL0(i) +
			    MTK_WED_RING_OFS_COUNT;

		if (!wed_r32(dev, count))
			wed_w32(dev, count, 1);
	}

	mtk_wed_hw_init(dev);

	mtk_wed_set_int(dev, irq_mask);
	mtk_wed_set_ext_int(dev, true);

	if (dev->hw->version == 1) {
		u32 val;

		val = dev->wlan.wpdma_phys |
		      MTK_PCIE_MIRROR_MAP_EN |
		      FIELD_PREP(MTK_PCIE_MIRROR_MAP_WED_ID, dev->hw->index);

		if (dev->hw->index)
			val |= BIT(1);
		val |= BIT(0);
		regmap_write(dev->hw->mirror, dev->hw->index * 4, val);
	} else if (mtk_wed_get_rx_capa(dev)) {
		/* driver set mid ready and only once */
		wed_w32(dev, MTK_WED_EXT_INT_MASK1,
			MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);
		wed_w32(dev, MTK_WED_EXT_INT_MASK2,
			MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);
		if (dev->hw->version == 3)
			wed_w32(dev, MTK_WED_EXT_INT_MASK3,
				MTK_WED_EXT_INT_STATUS_WPDMA_MID_RDY);

		wed_r32(dev, MTK_WED_EXT_INT_MASK1);
		wed_r32(dev, MTK_WED_EXT_INT_MASK2);
		if (dev->hw->version == 3)
			wed_r32(dev, MTK_WED_EXT_INT_MASK3);

		ret = mtk_wed_rro_cfg(dev);
		if (ret)
			return;
	}

	if (dev->hw->version == 2)
		mtk_wed_set_512_support(dev, dev->wlan.wcid_512);
	else if (dev->hw->version == 3)
		mtk_wed_pao_init(dev);

	mtk_wed_dma_enable(dev);
	dev->running = true;
}

static int
mtk_wed_get_pci_base(struct mtk_wed_device *dev)
{
	if (dev->hw->index == 0)
		return MTK_WED_PCIE_BASE0;
	else if (dev->hw->index == 1)
		return MTK_WED_PCIE_BASE1;
	else
		return MTK_WED_PCIE_BASE2;
}

static int
mtk_wed_attach(struct mtk_wed_device *dev)
	__releases(RCU)
{
	struct mtk_wed_hw *hw;
	struct device *device;
	int ret = 0;

	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
			 "mtk_wed_attach without holding the RCU read lock");

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	rcu_read_unlock();

	mutex_lock(&hw_lock);

	hw = mtk_wed_assign(dev);
	if (!hw) {
		module_put(THIS_MODULE);
		ret = -ENODEV;
		goto out;
	}

	device = dev->wlan.bus_type == MTK_WED_BUS_PCIE ?
				       &dev->wlan.pci_dev->dev
				       : &dev->wlan.platform_dev->dev;
	dev_info(device, "attaching wed device %d version %d\n",
		 hw->index, hw->version);

	dev->hw = hw;
	dev->dev = hw->dev;
	dev->irq = hw->irq;
	dev->wdma_idx = hw->index;
	dev->ver = hw->version;

	ret = dma_set_mask_and_coherent(hw->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	if (dev->hw->version == 3)
		dev->hw->pci_base = mtk_wed_get_pci_base(dev);

	if (hw->eth->dma_dev == hw->eth->dev &&
	    of_dma_is_coherent(hw->eth->dev->of_node))
		mtk_eth_set_dma_device(hw->eth, hw->dev);

	ret = mtk_wed_tx_buffer_alloc(dev);
	if (ret)
		goto error;

	if (mtk_wed_get_rx_capa(dev)) {
		ret = mtk_wed_rro_alloc(dev);
		if (ret)
			goto error;
	}

	mtk_wed_hw_init_early(dev);

	init_completion(&dev->fe_reset_done);
	init_completion(&dev->wlan_reset_done);
	atomic_set(&dev->fe_reset, 0);

	if (dev->hw->version != 1)
		dev->rev_id = wed_r32(dev, MTK_WED_REV_ID);
	else
		regmap_update_bits(hw->hifsys, HIFSYS_DMA_AG_MAP,
				   BIT(hw->index), 0);

	if (mtk_wed_get_rx_capa(dev))
		ret = mtk_wed_wo_init(hw);

error:
	if (ret) {
		pr_info("%s: detach wed\n", __func__);
		mtk_wed_detach(dev);
	}
out:
	mutex_unlock(&hw_lock);

	return ret;
}

static int
mtk_wed_tx_ring_setup(struct mtk_wed_device *dev, int idx,
		void __iomem *regs, bool reset)
{
	struct mtk_wed_ring *ring = &dev->tx_ring[idx];

	/*
	 * Tx ring redirection:
	 * Instead of configuring the WLAN PDMA TX ring directly, the WLAN
	 * driver allocated DMA ring gets configured into WED MTK_WED_RING_TX(n)
	 * registers.
	 *
	 * WED driver posts its own DMA ring as WLAN PDMA TX and configures it
	 * into MTK_WED_WPDMA_RING_TX(n) registers.
	 * It gets filled with packets picked up from WED TX ring and from
	 * WDMA RX.
	 */

	BUG_ON(idx > ARRAY_SIZE(dev->tx_ring));

	if (!reset)
		if (mtk_wed_ring_alloc(dev, ring, MTK_WED_TX_RING_SIZE,
				       1, true))
			return -ENOMEM;

	if (mtk_wed_wdma_rx_ring_setup(dev, idx, MTK_WED_WDMA_RING_SIZE, reset))
		return -ENOMEM;

	if (dev->hw->version == 3 && idx == 1) {
		/* reset prefetch index */
		wed_set(dev, MTK_WED_WDMA_RX_PREF_CFG,
		       MTK_WED_WDMA_RX_PREF_RX0_SIDX_CLR |
		       MTK_WED_WDMA_RX_PREF_RX1_SIDX_CLR);

		wed_clr(dev, MTK_WED_WDMA_RX_PREF_CFG,
		       MTK_WED_WDMA_RX_PREF_RX0_SIDX_CLR |
		       MTK_WED_WDMA_RX_PREF_RX1_SIDX_CLR);

		/* reset prefetch FIFO */
		wed_w32(dev, MTK_WED_WDMA_RX_PREF_FIFO_CFG,
		       MTK_WED_WDMA_RX_PREF_FIFO_RX0_CLR |
		       MTK_WED_WDMA_RX_PREF_FIFO_RX1_CLR);
		wed_w32(dev, MTK_WED_WDMA_RX_PREF_FIFO_CFG, 0);
	}

	ring->reg_base = MTK_WED_RING_TX(idx);
	ring->wpdma = regs;
	ring->flags |= MTK_WED_RING_CONFIGURED;

	/* WED -> WPDMA */
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_BASE, ring->desc_phys);
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_COUNT, MTK_WED_TX_RING_SIZE);
	wpdma_tx_w32(dev, idx, MTK_WED_RING_OFS_CPU_IDX, 0);

	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_BASE,
		ring->desc_phys);
	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_COUNT,
		MTK_WED_TX_RING_SIZE);
	wed_w32(dev, MTK_WED_WPDMA_RING_TX(idx) + MTK_WED_RING_OFS_CPU_IDX, 0);

	return 0;
}

static int
mtk_wed_txfree_ring_setup(struct mtk_wed_device *dev, void __iomem *regs)
{
	struct mtk_wed_ring *ring = &dev->txfree_ring;
	int i, idx = 1;

	if(dev->hw->version > 1)
		idx = 0;

	/*
	 * For txfree event handling, the same DMA ring is shared between WED
	 * and WLAN. The WLAN driver accesses the ring index registers through
	 * WED
	 */
	ring->reg_base = MTK_WED_RING_RX(idx);
	ring->wpdma = regs;

	for (i = 0; i < 12; i += 4) {
		u32 val = readl(regs + i);

		wed_w32(dev, MTK_WED_RING_RX(idx) + i, val);
		wed_w32(dev, MTK_WED_WPDMA_RING_RX(idx) + i, val);
	}

	return 0;
}

static int
mtk_wed_rx_ring_setup(struct mtk_wed_device *dev,
		int idx, void __iomem *regs, bool reset)
{
	struct mtk_wed_ring *ring = &dev->rx_ring[idx];

	BUG_ON(idx > ARRAY_SIZE(dev->rx_ring));

	if (!reset)
		if (mtk_wed_ring_alloc(dev, ring, MTK_WED_RX_RING_SIZE,
				       1, false))
			return -ENOMEM;

	if (mtk_wed_wdma_tx_ring_setup(dev, idx, MTK_WED_WDMA_RING_SIZE, reset))
		return -ENOMEM;

	ring->reg_base = MTK_WED_RING_RX_DATA(idx);
	ring->wpdma = regs;
	ring->flags |= MTK_WED_RING_CONFIGURED;
	dev->hw->ring_num = idx + 1;

	/* WPDMA ->  WED */
	wpdma_rx_w32(dev, idx, MTK_WED_RING_OFS_BASE, ring->desc_phys);
	wpdma_rx_w32(dev, idx, MTK_WED_RING_OFS_COUNT, MTK_WED_RX_RING_SIZE);

	wed_w32(dev, MTK_WED_WPDMA_RING_RX_DATA(idx) + MTK_WED_RING_OFS_BASE,
		ring->desc_phys);
	wed_w32(dev, MTK_WED_WPDMA_RING_RX_DATA(idx) + MTK_WED_RING_OFS_COUNT,
		MTK_WED_RX_RING_SIZE);

	return 0;
}

static int
mtk_wed_rro_rx_ring_setup(struct mtk_wed_device *dev, int idx, void __iomem *regs)
{
	struct mtk_wed_ring *ring = &dev->rx_rro_ring[idx];

	ring->wpdma = regs;

	wed_w32(dev, MTK_WED_RRO_RX_D_RX(idx) + MTK_WED_RING_OFS_BASE,
		readl(regs));
	wed_w32(dev, MTK_WED_RRO_RX_D_RX(idx) + MTK_WED_RING_OFS_COUNT,
		readl(regs + MTK_WED_RING_OFS_COUNT));

	ring->flags |= MTK_WED_RING_CONFIGURED;

	return 0;
}

static int
mtk_wed_msdu_pg_rx_ring_setup(struct mtk_wed_device *dev, int idx, void __iomem *regs)
{
	struct mtk_wed_ring *ring = &dev->rx_page_ring[idx];

	ring->wpdma = regs;

	wed_w32(dev, MTK_WED_RRO_MSDU_PG_CTRL0(idx) + MTK_WED_RING_OFS_BASE,
		readl(regs));
	wed_w32(dev, MTK_WED_RRO_MSDU_PG_CTRL0(idx) + MTK_WED_RING_OFS_COUNT,
		readl(regs + MTK_WED_RING_OFS_COUNT));

	ring->flags |= MTK_WED_RING_CONFIGURED;

	return 0;
}

static int
mtk_wed_ind_rx_ring_setup(struct mtk_wed_device *dev, void __iomem *regs)
{
	struct mtk_wed_ring *ring = &dev->ind_cmd_ring;
	u32 val = readl(regs + MTK_WED_RING_OFS_COUNT);
	int i = 0, cnt = 0;

	ring->wpdma = regs;

	if (readl(regs) & 0xf)
		pr_info("%s(): address is not 16-byte alignment\n", __func__);

	wed_w32(dev, MTK_WED_IND_CMD_RX_CTRL1 + MTK_WED_RING_OFS_BASE,
		readl(regs) & 0xfffffff0);

	wed_w32(dev, MTK_WED_IND_CMD_RX_CTRL1 + MTK_WED_RING_OFS_COUNT,
		readl(regs + MTK_WED_RING_OFS_COUNT));

	/* ack sn cr */
	wed_w32(dev, MTK_WED_RRO_CFG0, dev->wlan.phy_base +
		dev->wlan.ind_cmd.ack_sn_addr);
	wed_w32(dev, MTK_WED_RRO_CFG1,
		FIELD_PREP(MTK_WED_RRO_CFG1_MAX_WIN_SZ,
			   dev->wlan.ind_cmd.win_size) |
		FIELD_PREP(MTK_WED_RRO_CFG1_PARTICL_SE_ID,
			   dev->wlan.ind_cmd.particular_sid));

	/* particular session addr element */
	wed_w32(dev, MTK_WED_ADDR_ELEM_CFG0, dev->wlan.ind_cmd.particular_se_phys);

	for (i = 0; i < dev->wlan.ind_cmd.se_group_nums; i++) {
		wed_w32(dev, MTK_WED_RADDR_ELEM_TBL_WDATA,
			dev->wlan.ind_cmd.addr_elem_phys[i] >> 4);
		wed_w32(dev, MTK_WED_ADDR_ELEM_TBL_CFG,
			MTK_WED_ADDR_ELEM_TBL_WR | (i & 0x7f));

		val = wed_r32(dev, MTK_WED_ADDR_ELEM_TBL_CFG);
		while (!(val & MTK_WED_ADDR_ELEM_TBL_WR_RDY) &&
			 cnt < 100) {
			val = wed_r32(dev, MTK_WED_ADDR_ELEM_TBL_CFG);
			cnt++;
		}
		if (cnt >= 100) {
			dev_err(dev->hw->dev, "mtk_wed%d: write ba session base failed!\n",
				dev->hw->index);
		}
		/*if (mtk_wed_poll_busy(dev, MTK_WED_ADDR_ELEM_TBL_CFG,
				      MTK_WED_ADDR_ELEM_TBL_WR_RDY)) {
			dev_err(dev->hw->dev, "mtk_wed%d: write ba session base failed!\n",
				dev->hw->index);
			return -1;
		}*/
	}

	/* pn check init */
	for (i = 0; i < dev->wlan.ind_cmd.particular_sid; i++) {
		wed_w32(dev, MTK_WED_PN_CHECK_WDATA_M,
			MTK_WED_PN_CHECK_IS_FIRST);

		wed_w32(dev, MTK_WED_PN_CHECK_CFG, MTK_WED_PN_CHECK_WR |
			FIELD_PREP(MTK_WED_PN_CHECK_SE_ID, i));

		cnt = 0;
		val = wed_r32(dev, MTK_WED_PN_CHECK_CFG);
		while (!(val & MTK_WED_PN_CHECK_WR_RDY) &&
			 cnt < 100) {
			val = wed_r32(dev, MTK_WED_PN_CHECK_CFG);
			cnt++;
		}
		if (cnt >= 100) {
			dev_err(dev->hw->dev, "mtk_wed%d: session(%d) init failed!\n",
				dev->hw->index, i);
		}
		/*if (mtk_wed_poll_busy(dev, MTK_WED_PN_CHECK_CFG,
				      MTK_WED_PN_CHECK_WR_RDY)) {
			dev_err(dev->hw->dev, "mtk_wed%d: session(%d) init failed!\n",
				dev->hw->index, i);
			//return -1;
		}*/
	}

	wed_w32(dev, MTK_WED_RX_IND_CMD_CNT0, MTK_WED_RX_IND_CMD_DBG_CNT_EN);

	wed_set(dev, MTK_WED_CTRL, MTK_WED_CTRL_WED_RX_IND_CMD_EN);

	return 0;
}


static u32
mtk_wed_irq_get(struct mtk_wed_device *dev, u32 mask)
{
	u32 val;

	val = wed_r32(dev, MTK_WED_EXT_INT_STATUS);
	wed_w32(dev, MTK_WED_EXT_INT_STATUS, val);
	if (dev->hw->version == 3) {
		val &= MTK_WED_EXT_INT_STATUS_RX_DRV_COHERENT;
	} else {
		val &= MTK_WED_EXT_INT_STATUS_ERROR_MASK;	
		if (!dev->hw->num_flows)
			val &= ~MTK_WED_EXT_INT_STATUS_TKID_WO_PYLD;	
	}
	if (val && net_ratelimit())
		pr_err("mtk_wed%d: error status=%08x\n", dev->hw->index, val);

	val = wed_r32(dev, MTK_WED_INT_STATUS);
	val &= mask;
	wed_w32(dev, MTK_WED_INT_STATUS, val); /* ACK */

	return val;
}

static void
mtk_wed_irq_set_mask(struct mtk_wed_device *dev, u32 mask)
{
	if (!dev->running)
		return;

	mtk_wed_set_ext_int(dev, !!mask);
	wed_w32(dev, MTK_WED_INT_MASK, mask);
}

int mtk_wed_flow_add(int index)
{
	struct mtk_wed_hw *hw = hw_list[index];
	int ret;

	if (!hw || !hw->wed_dev)
		return -ENODEV;

	if (hw->num_flows) {
		hw->num_flows++;
		return 0;
	}

	mutex_lock(&hw_lock);
	if (!hw->wed_dev) {
		ret = -ENODEV;
		goto out;
	}

	ret = hw->wed_dev->wlan.offload_enable(hw->wed_dev);
	if (!ret)
		hw->num_flows++;
	mtk_wed_set_ext_int(hw->wed_dev, true);

out:
	mutex_unlock(&hw_lock);

	return ret;
}

void mtk_wed_flow_remove(int index)
{
	struct mtk_wed_hw *hw = hw_list[index];

	if (!hw)
		return;

	if (--hw->num_flows)
		return;

	mutex_lock(&hw_lock);
	if (!hw->wed_dev)
		goto out;

	hw->wed_dev->wlan.offload_disable(hw->wed_dev);
	mtk_wed_set_ext_int(hw->wed_dev, true);

out:
	mutex_unlock(&hw_lock);
}

static int mtk_wed_eth_setup_tc(struct mtk_wed_device *wed, struct net_device *dev,
		int type, void *type_data)
{
	switch (type) {
	case TC_SETUP_BLOCK:
	case TC_SETUP_FT:
		return mtk_eth_setup_tc_block(dev, type_data, wed->hw->eth);
	default:
		return -EOPNOTSUPP;
	}
}

void mtk_wed_add_hw(struct device_node *np, struct mtk_eth *eth,
			void __iomem *wdma, u32 wdma_phy, int index)

{
	static const struct mtk_wed_ops wed_ops = {
		.attach = mtk_wed_attach,
		.tx_ring_setup = mtk_wed_tx_ring_setup,
		.txfree_ring_setup = mtk_wed_txfree_ring_setup,
		.rx_ring_setup = mtk_wed_rx_ring_setup,
		.rro_rx_ring_setup = mtk_wed_rro_rx_ring_setup,
		.msdu_pg_rx_ring_setup = mtk_wed_msdu_pg_rx_ring_setup,
		.ind_rx_ring_setup = mtk_wed_ind_rx_ring_setup,
		.msg_update = mtk_wed_send_msg,
		.start = mtk_wed_start,
		.stop = mtk_wed_stop,
		.reset_dma = mtk_wed_reset_dma,
		.reg_read = wed_r32,
		.reg_write = wed_w32,
		.irq_get = mtk_wed_irq_get,
		.irq_set_mask = mtk_wed_irq_set_mask,
		.detach = mtk_wed_detach,
		.setup_tc = mtk_wed_eth_setup_tc,
		.ppe_check = mtk_wed_ppe_check,
		.start_hwrro = mtk_wed_start_hwrro,
	};
	struct device_node *eth_np = eth->dev->of_node;
	struct platform_device *pdev;
	struct mtk_wed_hw *hw;
	struct regmap *regs;
	int irq;

	if (!np)
		return;

	pdev = of_find_device_by_node(np);
	if (!pdev)
		return;

	get_device(&pdev->dev);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return;

	regs = syscon_regmap_lookup_by_phandle(np, NULL);
	if (!regs)
		return;

	rcu_assign_pointer(mtk_soc_wed_ops, &wed_ops);

	mutex_lock(&hw_lock);

	if (WARN_ON(hw_list[index]))
		goto unlock;

	hw = kzalloc(sizeof(*hw), GFP_KERNEL);
	hw->node = np;
	hw->regs = regs;
	hw->eth = eth;
	hw->dev = &pdev->dev;
	hw->wdma = wdma;
	hw->wdma_phy = wdma_phy;
	hw->index = index;
	hw->irq = irq;
	hw->version = MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V3) ?
		      3 : MTK_HAS_CAPS(eth->soc->caps, MTK_NETSYS_V2) ? 2 : 1;

	if (hw->version == 1) {
		hw->mirror = syscon_regmap_lookup_by_phandle(eth_np,
							     "mediatek,pcie-mirror");
		hw->hifsys = syscon_regmap_lookup_by_phandle(eth_np,
							     "mediatek,hifsys");

		if (IS_ERR(hw->mirror) || IS_ERR(hw->hifsys)) {
			kfree(hw);
			goto unlock;
		}

		if (!index) {
			regmap_write(hw->mirror, 0, 0);
			regmap_write(hw->mirror, 4, 0);
		}
	}

	mtk_wed_hw_add_debugfs(hw);

	hw_list[index] = hw;

unlock:
	mutex_unlock(&hw_lock);
}

void mtk_wed_exit(void)
{
	int i;

	rcu_assign_pointer(mtk_soc_wed_ops, NULL);

	synchronize_rcu();

	for (i = 0; i < ARRAY_SIZE(hw_list); i++) {
		struct mtk_wed_hw *hw;

		hw = hw_list[i];
		if (!hw)
			continue;

		hw_list[i] = NULL;
		debugfs_remove(hw->debugfs_dir);
		put_device(hw->dev);
		kfree(hw);
	}
}
