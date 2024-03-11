/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2022 MediaTek Inc.
 * Author: Henry Yen <henry.yen@mediatek.com>
 *         Bo-Cun Chen <bc-bocun.chen@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include "mtk_eth_soc.h"

#define MAX_PPPQ_PORT_NUM	6

static struct mtk_eth *_eth;

static void mtk_qdma_qos_shaper_ebl(struct mtk_eth *eth, u32 id, u32 enable)
{
	u32 val;

	if (enable) {
		val = MTK_QTX_SCH_MIN_RATE_EN | MTK_QTX_SCH_MAX_RATE_EN;
		val |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_MAN,  1) |
		       FIELD_PREP(MTK_QTX_SCH_MIN_RATE_EXP,  4) |
		       FIELD_PREP(MTK_QTX_SCH_MAX_RATE_MAN, 25) |
		       FIELD_PREP(MTK_QTX_SCH_MAX_RATE_EXP,  5) |
		       FIELD_PREP(MTK_QTX_SCH_MAX_RATE_WGHT, 4);

		writel(val, eth->base + MTK_QTX_SCH(id % MTK_QTX_PER_PAGE));
	} else {
		writel(0, eth->base + MTK_QTX_SCH(id % MTK_QTX_PER_PAGE));
	}
}

static void mtk_qdma_qos_disable(struct mtk_eth *eth)
{
	u32 id, val;

	for (id = 0; id < MAX_PPPQ_PORT_NUM; id++) {
		mtk_qdma_qos_shaper_ebl(eth, id, 0);

		writel(FIELD_PREP(MTK_QTX_CFG_HW_RESV_CNT_OFFSET, 4) |
		       FIELD_PREP(MTK_QTX_CFG_SW_RESV_CNT_OFFSET, 4),
		       eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));
	}

	val = (MTK_QDMA_TX_SCH_MAX_WFQ) | (MTK_QDMA_TX_SCH_MAX_WFQ << 16);
	for (id = 0; id < eth->soc->txrx.qdma_tx_sch; id += 2) {
		if (eth->soc->txrx.qdma_tx_sch == 4)
			writel(val, eth->base + MTK_QDMA_TX_4SCH_BASE(id));
		else
			writel(val, eth->base + MTK_QDMA_TX_2SCH_BASE);
	}
}

static void mtk_qdma_qos_pppq_enable(struct mtk_eth *eth)
{
	u32 id, val;

	for (id = 0; id < MAX_PPPQ_PORT_NUM; id++) {
		mtk_qdma_qos_shaper_ebl(eth, id, 1);

		writel(FIELD_PREP(MTK_QTX_CFG_HW_RESV_CNT_OFFSET, 4) |
		       FIELD_PREP(MTK_QTX_CFG_SW_RESV_CNT_OFFSET, 4),
		       eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));
	}

	val = (MTK_QDMA_TX_SCH_MAX_WFQ) | (MTK_QDMA_TX_SCH_MAX_WFQ << 16);
	for (id = 0; id < eth->soc->txrx.qdma_tx_sch; id+= 2) {
		if (eth->soc->txrx.qdma_tx_sch == 4)
			writel(val, eth->base + MTK_QDMA_TX_4SCH_BASE(id));
		else
			writel(val, eth->base + MTK_QDMA_TX_2SCH_BASE);
	}
}

 static ssize_t mtk_qmda_debugfs_write_qos(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	struct seq_file *m = file->private_data;
	struct mtk_eth *eth = m->private;
	char buf[8];
	int len = count;

	if ((len > 8) || copy_from_user(buf, buffer, len))
		return -EFAULT;

	if (buf[0] == '0') {
		pr_info("HQoS is going to be disabled !\n");
		eth->qos_toggle = 0;
		mtk_qdma_qos_disable(eth);
	} else if (buf[0] == '1') {
		pr_info("HQoS mode is going to be enabled !\n");
		eth->qos_toggle = 1;
	} else if (buf[0] == '2') {
		pr_info("Per-port-per-queue mode is going to be enabled !\n");
		pr_info("PPPQ use qid 0~5 (scheduler 0).\n");
		eth->qos_toggle = 2;
		mtk_qdma_qos_pppq_enable(eth);
	}

	return len;
}

static int mtk_qmda_debugfs_read_qos(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;

	if (eth->qos_toggle == 0)
		pr_info("HQoS is disabled now!\n");
	else if (eth->qos_toggle == 1)
		pr_info("HQoS is enabled now!\n");
	else if (eth->qos_toggle == 2)
		pr_info("Per-port-per-queue mode is enabled!\n");

	return 0;
}

static int mtk_qmda_debugfs_open_qos(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_qmda_debugfs_read_qos,
			   inode->i_private);
}

static ssize_t mtk_qmda_debugfs_read_qos_sched(struct file *file, char __user *user_buf,
					       size_t count, loff_t *ppos)
{
	struct mtk_eth *eth = _eth;
	long id = (long)file->private_data;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	int enable, scheduling, max_rate, exp, scheduler, i;
	ssize_t ret_cnt;
	u32 val;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (eth->soc->txrx.qdma_tx_sch == 4)
		val = readl(eth->base + MTK_QDMA_TX_4SCH_BASE(id));
	else
		val = readl(eth->base + MTK_QDMA_TX_2SCH_BASE);

	if (id & 0x1)
		val >>= 16;

	val &= MTK_QDMA_TX_SCH_MASK;
	enable     = FIELD_GET(MTK_QDMA_TX_SCH_RATE_EN, val);
	scheduling = FIELD_GET(MTK_QDMA_TX_SCH_MAX_WFQ, val);
	max_rate   = FIELD_GET(MTK_QDMA_TX_SCH_RATE_MAN, val);
	exp        = FIELD_GET(MTK_QDMA_TX_SCH_RATE_EXP, val);
	while (exp--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "EN\tScheduling\tMAX\tQueue#\n%d\t%s%16d\t", enable,
			 (scheduling == 1) ? "WRR" : "SP", max_rate);

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		val = readl(eth->base + MTK_QDMA_PAGE) & ~MTK_QTX_CFG_PAGE;
		val |= FIELD_PREP(MTK_QTX_CFG_PAGE, i / MTK_QTX_PER_PAGE);
		writel(val, eth->base + MTK_QDMA_PAGE);

		val = readl(eth->base + MTK_QTX_SCH(i % MTK_QTX_PER_PAGE));
		if (eth->soc->txrx.qdma_tx_sch == 4)
			scheduler = FIELD_GET(MTK_QTX_SCH_TX_SCH_SEL_V2, val);
		else
			scheduler = FIELD_GET(MTK_QTX_SCH_TX_SCH_SEL, val);
		if (id == scheduler)
			len += scnprintf(buf + len, buf_len - len, "%d  ", i);
	}

	len += scnprintf(buf + len, buf_len - len, "\n");
	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static ssize_t mtk_qmda_debugfs_write_qos_sched(struct file *file, const char __user *buf,
						size_t length, loff_t *offset)
{
	struct mtk_eth *eth = _eth;
	long id = (long)file->private_data;
	char line[64] = {0}, scheduling[32];
	int enable, rate, exp = 0, shift = 0;
	size_t size;
	u32 sch, val = 0;

	if (length >= sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%d %s %d", &enable, scheduling, &rate) != 3)
		return -EFAULT;

	while (rate > 127) {
		rate /= 10;
		exp++;
	}

	line[length] = '\0';

	if (enable)
		val |= FIELD_PREP(MTK_QDMA_TX_SCH_RATE_EN, 1);
	if (strcmp(scheduling, "sp") != 0)
		val |= FIELD_PREP(MTK_QDMA_TX_SCH_MAX_WFQ, 1);
	val |= FIELD_PREP(MTK_QDMA_TX_SCH_RATE_MAN, rate);
	val |= FIELD_PREP(MTK_QDMA_TX_SCH_RATE_EXP, exp);

	if (id & 0x1)
		shift = 16;

	if (eth->soc->txrx.qdma_tx_sch == 4)
		sch = readl(eth->base + MTK_QDMA_TX_4SCH_BASE(id));
	else
		sch = readl(eth->base + MTK_QDMA_TX_2SCH_BASE);

	sch &= ~(MTK_QDMA_TX_SCH_MASK << shift);
	sch |= val << shift;
	if (eth->soc->txrx.qdma_tx_sch == 4)
		writel(sch, eth->base + MTK_QDMA_TX_4SCH_BASE(id));
	else
		writel(sch, eth->base + MTK_QDMA_TX_2SCH_BASE);

	size = strlen(line);
	*offset += size;

	return length;
}

static ssize_t mtk_qmda_debugfs_read_qos_queue(struct file *file, char __user *user_buf,
					       size_t count, loff_t *ppos)
{
	struct mtk_eth *eth = _eth;
	long id = (long)file->private_data;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	int min_rate_en, min_rate, min_rate_exp;
	int max_rate_en, max_weight, max_rate, max_rate_exp;
	u32 qtx_sch, qtx_cfg, scheduler, val;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	val = readl(eth->base + MTK_QDMA_PAGE) & ~MTK_QTX_CFG_PAGE;
	val |= FIELD_PREP(MTK_QTX_CFG_PAGE, id / MTK_QTX_PER_PAGE);
	writel(val, eth->base + MTK_QDMA_PAGE);

	qtx_cfg = readl(eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));
	qtx_sch = readl(eth->base + MTK_QTX_SCH(id % MTK_QTX_PER_PAGE));
	if (eth->soc->txrx.qdma_tx_sch == 4)
		scheduler = FIELD_GET(MTK_QTX_SCH_TX_SCH_SEL_V2, qtx_sch);
	else
		scheduler = FIELD_GET(MTK_QTX_SCH_TX_SCH_SEL, qtx_sch);

	min_rate_en  = FIELD_GET(MTK_QTX_SCH_MIN_RATE_EN, qtx_sch);
	min_rate     = FIELD_GET(MTK_QTX_SCH_MIN_RATE_MAN, qtx_sch);
	min_rate_exp = FIELD_GET(MTK_QTX_SCH_MIN_RATE_EXP, qtx_sch);
	max_rate_en  = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EN, qtx_sch);
	max_weight   = FIELD_GET(MTK_QTX_SCH_MAX_RATE_WGHT, qtx_sch);
	max_rate     = FIELD_GET(MTK_QTX_SCH_MAX_RATE_MAN, qtx_sch);
	max_rate_exp = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EXP, qtx_sch);
	while (min_rate_exp--)
		min_rate *= 10;

	while (max_rate_exp--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "scheduler: %d\nhw resv: %d\nsw resv: %d\n", scheduler,
			 (qtx_cfg >> 8) & 0xff, qtx_cfg & 0xff);

	/* Switch to debug mode */
	val = readl(eth->base + MTK_QTX_MIB_IF) & ~MTK_MIB_ON_QTX_CFG;
	val |= MTK_MIB_ON_QTX_CFG;
	writel(val, eth->base + MTK_QTX_MIB_IF);

	val = readl(eth->base + MTK_QTX_MIB_IF) & ~MTK_VQTX_MIB_EN;
	val |= MTK_VQTX_MIB_EN;
	writel(val, eth->base + MTK_QTX_MIB_IF);

	qtx_cfg = readl(eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));
	qtx_sch = readl(eth->base + MTK_QTX_SCH(id % MTK_QTX_PER_PAGE));

	len += scnprintf(buf + len, buf_len - len,
			 "packet count: %u\n", qtx_cfg);
	len += scnprintf(buf + len, buf_len - len,
			 "packet drop: %u\n\n", qtx_sch);

	/* Recover to normal mode */
	val = readl(eth->base + MTK_QTX_MIB_IF);
	val &= ~MTK_MIB_ON_QTX_CFG;
	writel(val, eth->base + MTK_QTX_MIB_IF);

	val = readl(eth->base + MTK_QTX_MIB_IF);
	val &= ~MTK_VQTX_MIB_EN;
	writel(val, eth->base + MTK_QTX_MIB_IF);

	len += scnprintf(buf + len, buf_len - len,
			 "      EN     RATE     WEIGHT\n");
	len += scnprintf(buf + len, buf_len - len,
			 "----------------------------\n");
	len += scnprintf(buf + len, buf_len - len,
			 "max%5d%9d%9d\n", max_rate_en, max_rate, max_weight);
	len += scnprintf(buf + len, buf_len - len,
			 "min%5d%9d        -\n", min_rate_en, min_rate);

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
}

static ssize_t mtk_qmda_debugfs_write_qos_queue(struct file *file, const char __user *buf,
						size_t length, loff_t *offset)
{
	struct mtk_eth *eth = _eth;
	long id = (long)file->private_data;
	char line[64] = {0};
	int max_enable, max_rate, max_exp = 0;
	int min_enable, min_rate, min_exp = 0;
	int scheduler, weight, resv;
	size_t size;
	u32 val;

	if (length >= sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%d %d %d %d %d %d %d", &scheduler, &min_enable, &min_rate,
		   &max_enable, &max_rate, &weight, &resv) != 7)
		return -EFAULT;

	line[length] = '\0';

	while (max_rate > 127) {
		max_rate /= 10;
		max_exp++;
	}

	while (min_rate > 127) {
		min_rate /= 10;
		min_exp++;
	}

	val = readl(eth->base + MTK_QDMA_PAGE) & ~MTK_QTX_CFG_PAGE;
	val |= FIELD_PREP(MTK_QTX_CFG_PAGE, id / MTK_QTX_PER_PAGE);
	writel(val, eth->base + MTK_QDMA_PAGE);

	if (eth->soc->txrx.qdma_tx_sch == 4)
		val = FIELD_PREP(MTK_QTX_SCH_TX_SCH_SEL_V2, scheduler);
	else
		val = FIELD_PREP(MTK_QTX_SCH_TX_SCH_SEL, scheduler);
	if (min_enable)
		val |= MTK_QTX_SCH_MIN_RATE_EN;
	val |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_MAN, min_rate);
	val |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_EXP, min_exp);
	if (max_enable)
		val |= MTK_QTX_SCH_MAX_RATE_EN;
	val |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_WGHT, weight);
	val |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_MAN, max_rate);
	val |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_EXP, max_exp);
	writel(val, eth->base + MTK_QTX_SCH(id % MTK_QTX_PER_PAGE));

	val = readl(eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));
	val |= FIELD_PREP(MTK_QTX_CFG_HW_RESV_CNT_OFFSET, resv);
	val |= FIELD_PREP(MTK_QTX_CFG_SW_RESV_CNT_OFFSET, resv);
	writel(val, eth->base + MTK_QTX_CFG(id % MTK_QTX_PER_PAGE));

	size = strlen(line);
	*offset += size;

	return length;
}

int mtk_qdma_debugfs_init(struct mtk_eth *eth)
{
	static const struct file_operations fops_qos = {
		.open = mtk_qmda_debugfs_open_qos,
		.read = seq_read,
		.llseek = seq_lseek,
		.write = mtk_qmda_debugfs_write_qos,
		.release = single_release,
	};

	static const struct file_operations fops_qos_sched = {
		.open = simple_open,
		.read = mtk_qmda_debugfs_read_qos_sched,
		.write = mtk_qmda_debugfs_write_qos_sched,
		.llseek = default_llseek,
	};

	static const struct file_operations fops_qos_queue = {
		.open = simple_open,
		.read = mtk_qmda_debugfs_read_qos_queue,
		.write = mtk_qmda_debugfs_write_qos_queue,
		.llseek = default_llseek,
	};

	struct dentry *root;
	long i;
	char name[16];

	_eth = eth;

	root = debugfs_lookup("mtk_ppe", NULL);
	if (!root)
		return -ENOMEM;

	debugfs_create_file("qos_toggle", S_IRUGO, root, eth, &fops_qos);

	for (i = 0; i < eth->soc->txrx.qdma_tx_sch; i++) {
		snprintf(name, sizeof(name), "qdma_sch%ld", i);
		debugfs_create_file(name, S_IRUGO, root, (void *)i,
				    &fops_qos_sched);
	}

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		snprintf(name, sizeof(name), "qdma_txq%ld", i);
		debugfs_create_file(name, S_IRUGO, root, (void *)i,
				    &fops_qos_queue);
	}

	return 0;
}
