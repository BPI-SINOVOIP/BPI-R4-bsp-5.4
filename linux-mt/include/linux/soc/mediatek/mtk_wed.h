#ifndef __MTK_WED_H
#define __MTK_WED_H

#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/regmap.h>
#include <linux/pci.h>
#include <linux/skbuff.h>
#include <linux/iopoll.h>

#define WED_WO_STA_REC			0x6

#define MTK_WED_TX_QUEUES		2
#define MTK_WED_RX_QUEUES		2
#define MTK_WED_RX_PAGE_QUEUES         3

enum mtk_wed_wo_cmd {
	MTK_WED_WO_CMD_WED_CFG,
	MTK_WED_WO_CMD_WED_RX_STAT,
	MTK_WED_WO_CMD_RRO_SER,
	MTK_WED_WO_CMD_DBG_INFO,
	MTK_WED_WO_CMD_DEV_INFO,
	MTK_WED_WO_CMD_BSS_INFO,
	MTK_WED_WO_CMD_STA_REC,
	MTK_WED_WO_CMD_DEV_INFO_DUMP,
	MTK_WED_WO_CMD_BSS_INFO_DUMP,
	MTK_WED_WO_CMD_STA_REC_DUMP,
	MTK_WED_WO_CMD_BA_INFO_DUMP,
	MTK_WED_WO_CMD_FBCMD_Q_DUMP,
	MTK_WED_WO_CMD_FW_LOG_CTRL,
	MTK_WED_WO_CMD_LOG_FLUSH,
	MTK_WED_WO_CMD_CHANGE_STATE,
	MTK_WED_WO_CMD_CPU_STATS_ENABLE,
	MTK_WED_WO_CMD_CPU_STATS_DUMP,
	MTK_WED_WO_CMD_EXCEPTION_INIT,
	MTK_WED_WO_CMD_PROF_CTRL,
	MTK_WED_WO_CMD_STA_BA_DUMP,
	MTK_WED_WO_CMD_BA_CTRL_DUMP,
	MTK_WED_WO_CMD_RXCNT_CTRL,
	MTK_WED_WO_CMD_RXCNT_INFO,
	MTK_WED_WO_CMD_SET_CAP,
	MTK_WED_WO_CMD_CCIF_RING_DUMP,
	MTK_WED_WO_CMD_WED_END
};

enum {
	MTK_NO_WED,
	MTK_WED_V1,
	MTK_WED_V2,
	MTK_WED_VMAX
};

enum mtk_wed_bus_tye {
	MTK_WED_BUS_PCIE,
	MTK_WED_BUS_AXI,
};

struct mtk_wed_hw;
struct mtk_wdma_desc;

#define MTK_WED_RING_CONFIGURED		BIT(0)

struct mtk_wed_ring {
	struct mtk_wdma_desc *desc;
	dma_addr_t desc_phys;
	int size;
	u32 flags;

	u32 reg_base;
	void __iomem *wpdma;
};

struct mtk_rxbm_desc {
	__le32 buf0;
	__le32 token;
} __packed __aligned(4);

struct dma_page_info {
	void *addr;
	dma_addr_t addr_phys;
};

struct dma_buf {
	int size;
	int pkt_nums;
	void *desc;
	int desc_size;
	dma_addr_t desc_phys;
	struct dma_page_info *pages;
};

struct dma_entry {
	int size;
	struct mtk_rxbm_desc *desc;
	dma_addr_t desc_phys;
};

struct mtk_wed_wo_rx_stats {
	__le16 wlan_idx;
	__le16 tid;
	__le32 rx_pkt_cnt;
	__le32 rx_byte_cnt;
	__le32 rx_err_cnt;
	__le32 rx_drop_cnt;
};

struct mtk_wed_device {
#ifdef CONFIG_NET_MEDIATEK_SOC_WED
	const struct mtk_wed_ops *ops;
	struct device *dev;
	struct mtk_wed_hw *hw;
	bool init_done, running;
	bool wdma_init_done;
	int wdma_idx;
	int irq;
	u8 ver;
	u32 rev_id;

	struct mtk_wed_ring tx_ring[MTK_WED_TX_QUEUES];
	struct mtk_wed_ring txfree_ring;
	struct mtk_wed_ring tx_wdma[MTK_WED_TX_QUEUES];
	struct mtk_wed_ring rx_ring[MTK_WED_RX_QUEUES];
	struct mtk_wed_ring rx_wdma[MTK_WED_RX_QUEUES];

	struct mtk_wed_ring rx_rro_ring[MTK_WED_RX_QUEUES];
	struct mtk_wed_ring rx_page_ring[MTK_WED_RX_PAGE_QUEUES];
	struct mtk_wed_ring ind_cmd_ring;

	struct dma_buf tx_buf_ring;

	struct {
		int size;
		struct page_frag_cache rx_page;
		struct mtk_rxbm_desc *desc;
		dma_addr_t desc_phys;
	} rx_buf_ring;

	struct dma_buf rx_page_buf_ring;

	struct {
		struct mtk_wed_ring rro_ring;
		void __iomem *rro_desc;
		dma_addr_t miod_desc_phys;
		dma_addr_t fdbk_desc_phys;
		u32 mcu_view_miod;
	} rro;

	/* filled by driver: */
	struct {
		union {
			struct platform_device *platform_dev;
			struct pci_dev *pci_dev;
		};
		enum mtk_wed_bus_tye bus_type;
		void __iomem *base;
		void __iomem *regs;
		u32 phy_base;

		u32 wpdma_phys;
		u32 wpdma_int;
		u32 wpdma_mask;
		u32 wpdma_tx;
		u32 wpdma_txfree;
		u32 wpdma_rx_glo;
		u32 wpdma_rx[MTK_WED_RX_QUEUES];
		u32 wpdma_rx_rro[MTK_WED_RX_QUEUES];
		u32 wpdma_rx_pg;

		u8 tx_tbit[MTK_WED_TX_QUEUES];
		u8 rx_tbit[MTK_WED_RX_QUEUES];
		u8 rro_rx_tbit[MTK_WED_RX_QUEUES];
		u8 rx_pg_tbit[MTK_WED_RX_PAGE_QUEUES];
		u8 txfree_tbit;

		u16 token_start;
		unsigned int nbuf;
		unsigned int rx_nbuf;
		unsigned int rx_npkt;
		unsigned int rx_size;

		bool wcid_512;
		bool hwrro;
		bool msi;

		u8 max_amsdu_nums;
		u32 max_amsdu_len;

		struct {
			u8 se_group_nums;
			u16 win_size;
			u16 particular_sid;
			u32 ack_sn_addr;
			dma_addr_t particular_se_phys;
			dma_addr_t addr_elem_phys[1024];
		} ind_cmd;

		u32 chip_id;
		u32 (*init_buf)(void *ptr, dma_addr_t phys, int token_id);
		int (*offload_enable)(struct mtk_wed_device *wed);
		void (*offload_disable)(struct mtk_wed_device *wed);
		u32 (*init_rx_buf)(struct mtk_wed_device *wed, int size);
		void (*release_rx_buf)(struct mtk_wed_device *wed);
		void (*update_wo_rx_stats)(struct mtk_wed_device *wed,
					   struct mtk_wed_wo_rx_stats *stats);
		int (*reset)(struct mtk_wed_device *wed);
		void (*reset_complete)(struct mtk_wed_device *wed);
	} wlan;
	struct completion fe_reset_done;
	struct completion wlan_reset_done;
	atomic_t fe_reset;
#endif
};

struct mtk_wed_ops {
	int (*attach)(struct mtk_wed_device *dev);
	int (*tx_ring_setup)(struct mtk_wed_device *dev, int ring,
			     void __iomem *regs, bool reset);
	int (*txfree_ring_setup)(struct mtk_wed_device *dev,
				 void __iomem *regs);
	int (*rx_ring_setup)(struct mtk_wed_device *dev, int ring,
			     void __iomem *regs, bool reset);
	int (*rro_rx_ring_setup)(struct mtk_wed_device *dev, int ring,
			     void __iomem *regs);
	int (*msdu_pg_rx_ring_setup)(struct mtk_wed_device *dev, int ring,
			     void __iomem *regs);
	int (*ind_rx_ring_setup)(struct mtk_wed_device *dev, void __iomem *regs);
	int (*msg_update)(struct mtk_wed_device *dev, int cmd_id,
			  void *data, int len);
	void (*detach)(struct mtk_wed_device *dev);
	int (*setup_tc)(struct mtk_wed_device *wed, struct net_device *dev,
			 int type, void *type_data);
	void (*stop)(struct mtk_wed_device *dev);
	void (*start)(struct mtk_wed_device *dev, u32 irq_mask);
	void (*reset_dma)(struct mtk_wed_device *dev);

	u32 (*reg_read)(struct mtk_wed_device *dev, u32 reg);
	void (*reg_write)(struct mtk_wed_device *dev, u32 reg, u32 val);

	u32 (*irq_get)(struct mtk_wed_device *dev, u32 mask);
	void (*irq_set_mask)(struct mtk_wed_device *dev, u32 mask);
	void (*ppe_check)(struct mtk_wed_device *dev, struct sk_buff *skb,
			  u32 reason, u32 hash);
	void (*start_hwrro)(struct mtk_wed_device *dev, u32 irq_mask, bool reset);
};

extern const struct mtk_wed_ops __rcu *mtk_soc_wed_ops;

static inline int
mtk_wed_device_attach(struct mtk_wed_device *dev)
{
	int ret = -ENODEV;

#ifdef CONFIG_NET_MEDIATEK_SOC_WED
	rcu_read_lock();
	dev->ops = rcu_dereference(mtk_soc_wed_ops);
	if (dev->ops)
		ret = dev->ops->attach(dev);
	else
		rcu_read_unlock();

	if (ret)
		dev->ops = NULL;
#endif

	return ret;
}

static inline bool
mtk_wed_get_rx_capa(struct mtk_wed_device *dev)
{
#ifdef CONFIG_NET_MEDIATEK_SOC_WED
	if (dev->ver == 3 && !dev->wlan.hwrro)
		return false;

	return dev->ver != 1;
#else
	return false;
#endif
}

static inline bool
mtk_wed_device_support_pao(struct mtk_wed_device *dev)
{
	return dev->ver == 3;
}

#ifdef CONFIG_NET_MEDIATEK_SOC_WED
#define mtk_wed_device_active(_dev) !!(_dev)->ops
#define mtk_wed_device_detach(_dev) (_dev)->ops->detach(_dev)
#define mtk_wed_device_setup_tc(_dev, _ndev, _type, _data) \
	(_dev)->ops->setup_tc(_dev, _ndev, _type, _data)
#define mtk_wed_device_start(_dev, _mask) (_dev)->ops->start(_dev, _mask)
#define mtk_wed_device_stop(_dev) (_dev)->ops->stop(_dev)
#define mtk_wed_device_tx_ring_setup(_dev, _ring, _regs, _reset) \
	(_dev)->ops->tx_ring_setup(_dev, _ring, _regs,  _reset)
#define mtk_wed_device_txfree_ring_setup(_dev, _regs) \
	(_dev)->ops->txfree_ring_setup(_dev, _regs)
#define mtk_wed_device_rx_ring_setup(_dev, _ring, _regs, _reset) \
	(_dev)->ops->rx_ring_setup(_dev, _ring, _regs, _reset)
#define mtk_wed_device_rro_rx_ring_setup(_dev, _ring, _regs) \
	(_dev)->ops->rro_rx_ring_setup(_dev, _ring, _regs)
#define mtk_wed_device_msdu_pg_rx_ring_setup(_dev, _ring, _regs) \
	(_dev)->ops->msdu_pg_rx_ring_setup(_dev, _ring, _regs)
#define mtk_wed_device_ind_rx_ring_setup(_dev, _regs) \
	(_dev)->ops->ind_rx_ring_setup(_dev, _regs)
#define mtk_wed_device_update_msg(_dev, _id, _msg, _len) \
	(_dev)->ops->msg_update(_dev, _id, _msg, _len)
#define mtk_wed_device_reg_read(_dev, _reg) \
	(_dev)->ops->reg_read(_dev, _reg)
#define mtk_wed_device_reg_write(_dev, _reg, _val) \
	(_dev)->ops->reg_write(_dev, _reg, _val)
#define mtk_wed_device_irq_get(_dev, _mask) \
	(_dev)->ops->irq_get(_dev, _mask)
#define mtk_wed_device_irq_set_mask(_dev, _mask) \
	(_dev)->ops->irq_set_mask(_dev, _mask)
#define mtk_wed_device_dma_reset(_dev) \
	(_dev)->ops->reset_dma(_dev)
#define mtk_wed_device_ppe_check(_dev, _skb, _reason, _hash) \
	(_dev)->ops->ppe_check(_dev, _skb, _reason, _hash)
#define mtk_wed_device_start_hwrro(_dev, _mask, _reset) \
	(_dev)->ops->start_hwrro(_dev, _mask, _reset)

#else
static inline bool mtk_wed_device_active(struct mtk_wed_device *dev)
{
	return false;
}
#define mtk_wed_device_detach(_dev) do {} while (0)
#define mtk_wed_device_start(_dev, _mask) do {} while (0)
#define mtk_wed_device_stop(_dev) do {} while (0)
#define mtk_wed_device_tx_ring_setup(_dev, _ring, _regs, _reset) -ENODEV
#define mtk_wed_device_txfree_ring_setup(_dev, _ring, _regs) -ENODEV
#define mtk_wed_device_rx_ring_setup(_dev, _ring, _regs, _reset) -ENODEV
#define mtk_wed_device_rro_rx_ring_setup(_dev, _ring, _regs) -ENODEV
#define mtk_wed_device_msdu_pg_rx_ring_setup(_dev, _ring, _regs)  -ENODEV
#define mtk_wed_device_ind_rx_ring_setup(_dev, _regs)  -ENODEV
#define mtk_wed_device_reg_read(_dev, _reg) 0
#define mtk_wed_device_reg_write(_dev, _reg, _val) do {} while (0)
#define mtk_wed_device_irq_get(_dev, _mask) 0
#define mtk_wed_device_irq_set_mask(_dev, _mask) do {} while (0)
#define mtk_wed_device_dma_reset(_dev) do {} while (0)
#define mtk_wed_device_setup_tc(_dev, _ndev, _type, _data) do {} while (0)
#define mtk_wed_device_ppe_check(_dev, _hash)  do {} while (0)
#define mtk_wed_device_start_hwrro(_dev, _mask) do {} while (0)
#endif

#endif
