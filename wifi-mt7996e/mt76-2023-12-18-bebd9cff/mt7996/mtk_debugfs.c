// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2023 MediaTek Inc.
 */
#include "mt7996.h"
#include "../mt76.h"
#include "mcu.h"
#include "mac.h"
#include "eeprom.h"
#include "mtk_debug.h"
#include "mtk_mcu.h"
#include "coredump.h"

#ifdef CONFIG_MTK_DEBUG

/* AGG INFO */
static int
mt7996_agginfo_read_per_band(struct seq_file *s, int band_idx)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u64 total_burst, total_ampdu, ampdu_cnt[16];
	u32 value, idx, row_idx, col_idx, start_range, agg_rang_sel[16], burst_cnt[16], band_offset = 0;
	u8 partial_str[16] = {}, full_str[64] = {};

	switch (band_idx) {
	case 0:
		band_offset = 0;
		break;
	case 1:
		band_offset = BN1_WF_AGG_TOP_BASE - BN0_WF_AGG_TOP_BASE;
		break;
	case 2:
		band_offset = IP1_BN0_WF_AGG_TOP_BASE - BN0_WF_AGG_TOP_BASE;
		break;
	default:
		return 0;
	}

	seq_printf(s, "Band %d AGG Status\n", band_idx);
	seq_printf(s, "===============================\n");
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR0_ADDR + band_offset);
	seq_printf(s, "AC00 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR0_AC00_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR0_AC00_AGG_LIMIT_SHFT);
	seq_printf(s, "AC01 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR0_AC01_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR0_AC01_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR1_ADDR + band_offset);
	seq_printf(s, "AC02 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR1_AC02_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR1_AC02_AGG_LIMIT_SHFT);
	seq_printf(s, "AC03 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR1_AC03_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR1_AC03_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR2_ADDR + band_offset);
	seq_printf(s, "AC10 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR2_AC10_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR2_AC10_AGG_LIMIT_SHFT);
	seq_printf(s, "AC11 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR2_AC11_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR2_AC11_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR3_ADDR + band_offset);
	seq_printf(s, "AC12 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR3_AC12_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR3_AC12_AGG_LIMIT_SHFT);
	seq_printf(s, "AC13 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR3_AC13_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR3_AC13_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR4_ADDR + band_offset);
	seq_printf(s, "AC20 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR4_AC20_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR4_AC20_AGG_LIMIT_SHFT);
	seq_printf(s, "AC21 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR4_AC21_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR4_AC21_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR5_ADDR + band_offset);
	seq_printf(s, "AC22 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR5_AC22_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR5_AC22_AGG_LIMIT_SHFT);
	seq_printf(s, "AC23 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR5_AC23_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR5_AC23_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR6_ADDR + band_offset);
	seq_printf(s, "AC30 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR6_AC30_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR6_AC30_AGG_LIMIT_SHFT);
	seq_printf(s, "AC31 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR6_AC31_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR6_AC31_AGG_LIMIT_SHFT);
	value = mt76_rr(dev, BN0_WF_AGG_TOP_AALCR7_ADDR + band_offset);
	seq_printf(s, "AC32 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR7_AC32_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR7_AC32_AGG_LIMIT_SHFT);
	seq_printf(s, "AC33 Agg limit = %d\t", (value & BN0_WF_AGG_TOP_AALCR7_AC33_AGG_LIMIT_MASK) >>  BN0_WF_AGG_TOP_AALCR7_AC33_AGG_LIMIT_SHFT);

	switch (band_idx) {
	case 0:
		band_offset = 0;
		break;
	case 1:
		band_offset = BN1_WF_MIB_TOP_BASE - BN0_WF_MIB_TOP_BASE;
		break;
	case 2:
		band_offset = IP1_BN0_WF_MIB_TOP_BASE - BN0_WF_MIB_TOP_BASE;
		break;
	default:
		return 0;
	}

	seq_printf(s, "===AMPDU Related Counters===\n");

	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC0_ADDR + band_offset);
	agg_rang_sel[0] = (value & BN0_WF_MIB_TOP_TRARC0_AGG_RANG_SEL_0_MASK) >> BN0_WF_MIB_TOP_TRARC0_AGG_RANG_SEL_0_SHFT;
	agg_rang_sel[1] = (value & BN0_WF_MIB_TOP_TRARC0_AGG_RANG_SEL_1_MASK) >> BN0_WF_MIB_TOP_TRARC0_AGG_RANG_SEL_1_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC1_ADDR + band_offset);
	agg_rang_sel[2] = (value & BN0_WF_MIB_TOP_TRARC1_AGG_RANG_SEL_2_MASK) >> BN0_WF_MIB_TOP_TRARC1_AGG_RANG_SEL_2_SHFT;
	agg_rang_sel[3] = (value & BN0_WF_MIB_TOP_TRARC1_AGG_RANG_SEL_3_MASK) >> BN0_WF_MIB_TOP_TRARC1_AGG_RANG_SEL_3_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC2_ADDR + band_offset);
	agg_rang_sel[4] = (value & BN0_WF_MIB_TOP_TRARC2_AGG_RANG_SEL_4_MASK) >> BN0_WF_MIB_TOP_TRARC2_AGG_RANG_SEL_4_SHFT;
	agg_rang_sel[5] = (value & BN0_WF_MIB_TOP_TRARC2_AGG_RANG_SEL_5_MASK) >> BN0_WF_MIB_TOP_TRARC2_AGG_RANG_SEL_5_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC3_ADDR + band_offset);
	agg_rang_sel[6] = (value & BN0_WF_MIB_TOP_TRARC3_AGG_RANG_SEL_6_MASK) >> BN0_WF_MIB_TOP_TRARC3_AGG_RANG_SEL_6_SHFT;
	agg_rang_sel[7] = (value & BN0_WF_MIB_TOP_TRARC3_AGG_RANG_SEL_7_MASK) >> BN0_WF_MIB_TOP_TRARC3_AGG_RANG_SEL_7_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC4_ADDR + band_offset);
	agg_rang_sel[8] = (value & BN0_WF_MIB_TOP_TRARC4_AGG_RANG_SEL_8_MASK) >> BN0_WF_MIB_TOP_TRARC4_AGG_RANG_SEL_8_SHFT;
	agg_rang_sel[9] = (value & BN0_WF_MIB_TOP_TRARC4_AGG_RANG_SEL_9_MASK) >> BN0_WF_MIB_TOP_TRARC4_AGG_RANG_SEL_9_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC5_ADDR + band_offset);
	agg_rang_sel[10] = (value & BN0_WF_MIB_TOP_TRARC5_AGG_RANG_SEL_10_MASK) >> BN0_WF_MIB_TOP_TRARC5_AGG_RANG_SEL_10_SHFT;
	agg_rang_sel[11] = (value & BN0_WF_MIB_TOP_TRARC5_AGG_RANG_SEL_11_MASK) >> BN0_WF_MIB_TOP_TRARC5_AGG_RANG_SEL_11_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC6_ADDR + band_offset);
	agg_rang_sel[12] = (value & BN0_WF_MIB_TOP_TRARC6_AGG_RANG_SEL_12_MASK) >> BN0_WF_MIB_TOP_TRARC6_AGG_RANG_SEL_12_SHFT;
	agg_rang_sel[13] = (value & BN0_WF_MIB_TOP_TRARC6_AGG_RANG_SEL_13_MASK) >> BN0_WF_MIB_TOP_TRARC6_AGG_RANG_SEL_13_SHFT;
	value = mt76_rr(dev, BN0_WF_MIB_TOP_TRARC7_ADDR + band_offset);
	agg_rang_sel[14] = (value & BN0_WF_MIB_TOP_TRARC7_AGG_RANG_SEL_14_MASK) >> BN0_WF_MIB_TOP_TRARC7_AGG_RANG_SEL_14_SHFT;

	burst_cnt[0] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR0_ADDR + band_offset);
	burst_cnt[1] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR1_ADDR + band_offset);
	burst_cnt[2] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR2_ADDR + band_offset);
	burst_cnt[3] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR3_ADDR + band_offset);
	burst_cnt[4] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR4_ADDR + band_offset);
	burst_cnt[5] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR5_ADDR + band_offset);
	burst_cnt[6] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR6_ADDR + band_offset);
	burst_cnt[7] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR7_ADDR + band_offset);
	burst_cnt[8] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR8_ADDR + band_offset);
	burst_cnt[9] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR9_ADDR + band_offset);
	burst_cnt[10] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR10_ADDR + band_offset);
	burst_cnt[11] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR11_ADDR + band_offset);
	burst_cnt[12] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR12_ADDR + band_offset);
	burst_cnt[13] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR13_ADDR + band_offset);
	burst_cnt[14] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR14_ADDR + band_offset);
	burst_cnt[15] = mt76_rr(dev, BN0_WF_MIB_TOP_TRDR15_ADDR + band_offset);

	start_range = 1;
	total_burst = 0;
	total_ampdu = 0;
	agg_rang_sel[15] = 1023;

	/* Need to add 1 after read from AGG_RANG_SEL CR */
	for (idx = 0; idx < 16; idx++) {
		agg_rang_sel[idx]++;
		total_burst += burst_cnt[idx];

		if (start_range == agg_rang_sel[idx])
			ampdu_cnt[idx] = (u64) start_range * burst_cnt[idx];
		else
			ampdu_cnt[idx] = (u64) ((start_range + agg_rang_sel[idx]) >> 1) * burst_cnt[idx];

		start_range = agg_rang_sel[idx] + 1;
		total_ampdu += ampdu_cnt[idx];
	}

	start_range = 1;
	sprintf(full_str, "%13s ", "Tx Agg Range:");

	for (row_idx = 0; row_idx < 4; row_idx++) {
		for (col_idx = 0; col_idx < 4; col_idx++, idx++) {
			idx = 4 * row_idx + col_idx;

			if (start_range == agg_rang_sel[idx])
				sprintf(partial_str, "%d", agg_rang_sel[idx]);
			else
				sprintf(partial_str, "%d~%d", start_range, agg_rang_sel[idx]);

			start_range = agg_rang_sel[idx] + 1;
			sprintf(full_str + strlen(full_str), "%-11s ", partial_str);
		}

		idx = 4 * row_idx;

		seq_printf(s, "%s\n", full_str);
		seq_printf(s, "%13s 0x%-9x 0x%-9x 0x%-9x 0x%-9x\n",
			row_idx ? "" : "Burst count:",
			burst_cnt[idx], burst_cnt[idx + 1],
			burst_cnt[idx + 2], burst_cnt[idx + 3]);

		if (total_burst != 0) {
			if (row_idx == 0)
				sprintf(full_str, "%13s ",
					"Burst ratio:");
			else
				sprintf(full_str, "%13s ", "");

			for (col_idx = 0; col_idx < 4; col_idx++) {
				u64 count = (u64) burst_cnt[idx + col_idx] * 100;

				sprintf(partial_str, "(%llu%%)",
					div64_u64(count, total_burst));
				sprintf(full_str + strlen(full_str),
					"%-11s ", partial_str);
			}

			seq_printf(s, "%s\n", full_str);

			if (row_idx == 0)
				sprintf(full_str, "%13s ",
					"MDPU ratio:");
			else
				sprintf(full_str, "%13s ", "");

			for (col_idx = 0; col_idx < 4; col_idx++) {
				u64 count = ampdu_cnt[idx + col_idx] * 100;

				sprintf(partial_str, "(%llu%%)",
					div64_u64(count, total_ampdu));
				sprintf(full_str + strlen(full_str),
					"%-11s ", partial_str);
			}

			seq_printf(s, "%s\n", full_str);
		}

		sprintf(full_str, "%13s ", "");
	}

	return 0;
}

static int mt7996_agginfo_read_band0(struct seq_file *s, void *data)
{
	mt7996_agginfo_read_per_band(s, MT_BAND0);
	return 0;
}

static int mt7996_agginfo_read_band1(struct seq_file *s, void *data)
{
	mt7996_agginfo_read_per_band(s, MT_BAND1);
	return 0;
}

static int mt7996_agginfo_read_band2(struct seq_file *s, void *data)
{
	mt7996_agginfo_read_per_band(s, MT_BAND2);
	return 0;
}

/* AMSDU INFO */
static int mt7996_amsdu_result_read(struct seq_file *s, void *data)
{
#define HW_MSDU_CNT_ADDR 0xf400
#define HW_MSDU_NUM_MAX 33
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u32 ple_stat[HW_MSDU_NUM_MAX] = {0}, total_amsdu = 0;
	u8 i;

	for (i = 0; i < HW_MSDU_NUM_MAX; i++)
		ple_stat[i] = mt76_rr(dev, HW_MSDU_CNT_ADDR + i * 0x04);

	seq_printf(s, "TXD counter status of MSDU:\n");

	for (i = 0; i < HW_MSDU_NUM_MAX; i++)
		total_amsdu += ple_stat[i];

	for (i = 0; i < HW_MSDU_NUM_MAX; i++) {
		seq_printf(s, "AMSDU pack count of %d MSDU in TXD: 0x%x ", i, ple_stat[i]);
		if (total_amsdu != 0)
			seq_printf(s, "(%d%%)\n", ple_stat[i] * 100 / total_amsdu);
		else
			seq_printf(s, "\n");
	}

	return 0;
}

/* DBG MODLE */
static int
mt7996_fw_debug_module_set(void *data, u64 module)
{
	struct mt7996_dev *dev = data;

	dev->dbg.fw_dbg_module = module;
	return 0;
}

static int
mt7996_fw_debug_module_get(void *data, u64 *module)
{
	struct mt7996_dev *dev = data;

	*module = dev->dbg.fw_dbg_module;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_fw_debug_module, mt7996_fw_debug_module_get,
			 mt7996_fw_debug_module_set, "%lld\n");

static int
mt7996_fw_debug_level_set(void *data, u64 level)
{
	struct mt7996_dev *dev = data;

	dev->dbg.fw_dbg_lv = level;
	mt7996_mcu_fw_dbg_ctrl(dev, dev->dbg.fw_dbg_module, dev->dbg.fw_dbg_lv);
	return 0;
}

static int
mt7996_fw_debug_level_get(void *data, u64 *level)
{
	struct mt7996_dev *dev = data;

	*level = dev->dbg.fw_dbg_lv;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_fw_debug_level, mt7996_fw_debug_level_get,
			 mt7996_fw_debug_level_set, "%lld\n");

/* usage: echo 0x[arg3][arg2][arg1] > fw_wa_set */
static int
mt7996_wa_set(void *data, u64 val)
{
	struct mt7996_dev *dev = data;
	u32 arg1, arg2, arg3;

	arg1 = FIELD_GET(GENMASK_ULL(7, 0), val);
	arg2 = FIELD_GET(GENMASK_ULL(15, 8), val);
	arg3 = FIELD_GET(GENMASK_ULL(23, 16), val);

	return mt7996_mcu_wa_cmd(dev, MCU_WA_PARAM_CMD(SET),
				arg1, arg2, arg3);
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_wa_set, NULL, mt7996_wa_set,
			 "0x%llx\n");

/* usage: echo 0x[arg3][arg2][arg1] > fw_wa_query */
static int
mt7996_wa_query(void *data, u64 val)
{
	struct mt7996_dev *dev = data;
	u32 arg1, arg2, arg3;

	arg1 = FIELD_GET(GENMASK_ULL(7, 0), val);
	arg2 = FIELD_GET(GENMASK_ULL(15, 8), val);
	arg3 = FIELD_GET(GENMASK_ULL(23, 16), val);

	return mt7996_mcu_wa_cmd(dev, MCU_WA_PARAM_CMD(QUERY),
				arg1, arg2, arg3);
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_wa_query, NULL, mt7996_wa_query,
			 "0x%llx\n");

static int mt7996_dump_version(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	seq_printf(s, "Version: 3.3.15.0\n");

	if (!test_bit(MT76_STATE_MCU_RUNNING, &dev->mphy.state))
		return 0;

	seq_printf(s, "Rom Patch Build Time: %.16s\n", dev->patch_build_date);
	seq_printf(s, "WM Patch Build Time: %.15s, Mode: %s\n",
		   dev->ram_build_date[MT7996_RAM_TYPE_WM],
		   dev->testmode_enable ? "Testmode" : "Normal mode");
	seq_printf(s, "WA Patch Build Time: %.15s\n",
		   dev->ram_build_date[MT7996_RAM_TYPE_WA]);
	seq_printf(s, "DSP Patch Build Time: %.15s\n",
		   dev->ram_build_date[MT7996_RAM_TYPE_DSP]);
	return 0;
}

/* fw wm call trace info dump */
void mt7996_show_lp_history(struct seq_file *s, u32 type)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	struct mt7996_crash_data *crash_data;
	struct mt7996_coredump *dump;
	u64 now = 0;
	int i = 0;
	u8 fw_type = !!type;

	mutex_lock(&dev->dump_mutex);

	crash_data = mt7996_coredump_new(dev, fw_type);
	if (!crash_data) {
		mutex_unlock(&dev->dump_mutex);
		seq_printf(s, "the coredump is disable!\n");
		return;
	}
	mutex_unlock(&dev->dump_mutex);

	dump = mt7996_coredump_build(dev, fw_type, false);
	if (!dump) {
		seq_printf(s, "no call stack data found!\n");
		return;
	}

	seq_printf(s, "\x1b[32m%s log output\x1b[0m\n", dump->fw_type);
	seq_printf(s, "\x1b[32mfw status: %s\n", dump->fw_state);
	mt7996_dump_version(s, NULL);
	/* PC log */
	now = jiffies;
	for (i = 0; i < 10; i++)
		seq_printf(s, "\tCurrent PC=%x\n", dump->pc_cur[i]);

	seq_printf(s, "PC log contorl=0x%x(T=%llu)(latest PC index = 0x%x)\n",
		dump->pc_dbg_ctrl, now, dump->pc_cur_idx);
	for (i = 0; i < 32; i++)
		seq_printf(s, "\tPC log(%d)=0x%08x\n", i, dump->pc_stack[i]);

	/* LR log */
	now = jiffies;
	seq_printf(s, "\nLR log contorl=0x%x(T=%llu)(latest LR index = 0x%x)\n",
		dump->lr_dbg_ctrl, now, dump->lr_cur_idx);
	for (i = 0; i < 32; i++)
		seq_printf(s, "\tLR log(%d)=0x%08x\n", i, dump->lr_stack[i]);

	vfree(dump);
}

static int mt7996_fw_wa_info_read(struct seq_file *s, void *data)
{
	seq_printf(s, "======[ShowPcLpHistory]======\n");
	mt7996_show_lp_history(s, MT7996_RAM_TYPE_WA);
	seq_printf(s, "======[End ShowPcLpHistory]==\n");

	return 0;
}

static int mt7996_fw_wm_info_read(struct seq_file *s, void *data)
{
	seq_printf(s, "======[ShowPcLpHistory]======\n");
	mt7996_show_lp_history(s, MT7996_RAM_TYPE_WM);
	seq_printf(s, "======[End ShowPcLpHistory]==\n");

	return 0;
}

/* dma info dump */
static void
dump_dma_tx_ring_info(struct seq_file *s, struct mt7996_dev *dev,  char *str1, char *str2, u32 ring_base)
{
	u32 base, cnt, cidx, didx, queue_cnt;

	base= mt76_rr(dev, ring_base);
	cnt = mt76_rr(dev, ring_base + 4);
	cidx = mt76_rr(dev, ring_base + 8);
	didx = mt76_rr(dev, ring_base + 12);
	queue_cnt = (cidx >= didx) ? (cidx - didx) : (cidx - didx + cnt);

	seq_printf(s, "%20s %6s %10x %15x %10x %10x %10x\n", str1, str2, base, cnt, cidx, didx, queue_cnt);
}

static void
dump_dma_rx_ring_info(struct seq_file *s, struct mt7996_dev *dev,  char *str1, char *str2, u32 ring_base)
{
	u32 base, ctrl1, cnt, cidx, didx, queue_cnt;

	base= mt76_rr(dev, ring_base);
	ctrl1 = mt76_rr(dev, ring_base + 4);
	cidx = mt76_rr(dev, ring_base + 8) & 0xfff;
	didx = mt76_rr(dev, ring_base + 12) & 0xfff;
	cnt = ctrl1 & 0xfff;
	queue_cnt = (didx > cidx) ? (didx - cidx - 1) : (didx - cidx + cnt - 1);

	seq_printf(s, "%20s %6s %10x %10x(%3x) %10x %10x %10x\n",
		   str1, str2, base, ctrl1, cnt, cidx, didx, queue_cnt);
}

static void
mt7996_show_dma_info(struct seq_file *s, struct mt7996_dev *dev)
{
	u32 sys_ctrl[10];

	/* HOST DMA0 information */
	sys_ctrl[0] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_HOST_INT_STA_ADDR);
	sys_ctrl[1] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_HOST_INT_ENA_ADDR);
	sys_ctrl[2] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_ADDR);

	seq_printf(s, "HOST_DMA Configuration\n");
	seq_printf(s, "%10s %10s %10s %10s %10s %10s\n",
		"DMA", "IntCSR", "IntMask", "Glocfg", "Tx/RxEn", "Tx/RxBusy");
	seq_printf(s, "%10s %10x %10x %10x %4x/%5x %4x/%5x\n",
		"DMA0", sys_ctrl[0], sys_ctrl[1], sys_ctrl[2],
		(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK)
			>> WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_SHFT,
		(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK)
			>> WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_SHFT,
		(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_BUSY_MASK)
			>> WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_TX_DMA_BUSY_SHFT,
		(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_BUSY_MASK)
			>> WF_WFDMA_HOST_DMA0_WPDMA_GLO_CFG_RX_DMA_BUSY_SHFT);

	if (dev->hif2) {
		/* HOST DMA1 information */
		sys_ctrl[0] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_PCIE1_HOST_INT_STA_ADDR);
		sys_ctrl[1] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_PCIE1_HOST_INT_ENA_ADDR);
		sys_ctrl[2] = mt76_rr(dev, WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_ADDR);

		seq_printf(s, "%10s %10x %10x %10x %4x/%5x %4x/%5x\n",
			"DMA0P1", sys_ctrl[0], sys_ctrl[1], sys_ctrl[2],
			(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_MASK)
				>> WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_EN_SHFT,
			(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_MASK)
				>> WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_EN_SHFT,
			(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_BUSY_MASK)
				>> WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_TX_DMA_BUSY_SHFT,
			(sys_ctrl[2] & WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_BUSY_MASK)
				>> WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_GLO_CFG_RX_DMA_BUSY_SHFT);
	}

	seq_printf(s, "HOST_DMA0 Ring Configuration\n");
	seq_printf(s, "%20s %6s %10s %15s %10s %10s %10s\n",
		"Name", "Used", "Base", "Ctrl1(Cnt)", "CIDX", "DIDX", "QCnt");
	dump_dma_tx_ring_info(s, dev, "T0:TXD0(H2MAC)", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING0_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T1:TXD1(H2MAC)", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING1_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T2:TXD2(H2MAC)", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING2_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T3:", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING3_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T4:", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING4_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T5:", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING5_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T6:", "STA",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING6_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T16:FWDL", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING16_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T17:Cmd(H2WM)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING17_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T18:TXD0(H2WA)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING18_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T19:TXD1(H2WA)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING19_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T20:Cmd(H2WA)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING20_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T21:TXD2(H2WA)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING21_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T22:TXD3(H2WA)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_TX_RING22_CTRL0_ADDR);


	dump_dma_rx_ring_info(s, dev, "R0:Event(WM2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING0_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R1:Event(WA2H)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING1_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R2:TxDone0(WA2H)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING2_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R3:TxDone1(WA2H)", "AP",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING3_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R4:Data0(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING4_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R5:Data1(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING5_CTRL0_ADDR);
	if (is_mt7996(&dev->mt76))
		dump_dma_rx_ring_info(s, dev, "R6:BUF1(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR);
	else
		dump_dma_rx_ring_info(s, dev, "R6:TxDone0(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_WPDMA_RX_RING6_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R7:TxDone1(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING7_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R8:BUF0(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING8_CTRL0_ADDR);
	if (is_mt7996(&dev->mt76))
		dump_dma_rx_ring_info(s, dev, "R9:TxDone0(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL0_ADDR);
	else
		dump_dma_rx_ring_info(s, dev, "R9:BUF0(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_WPDMA_RX_RING9_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R10:MSDU_PG0(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING10_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R11:MSDU_PG1(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING11_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R12:MSDU_PG2(MAC2H)", "Both",
		WF_WFDMA_HOST_DMA0_WPDMA_RX_RING12_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "IND:IND_CMD(MAC2H)", "Both",
		WF_RRO_TOP_IND_CMD_0_CTRL0_ADDR);

	if (dev->hif2) {
		seq_printf(s, "HOST_DMA0 PCIe1 Ring Configuration\n");
		seq_printf(s, "%20s %6s %10s %15s %10s %10s %10s\n",
			"Name", "Used", "Base", "Ctrl1(Cnt)", "CIDX", "DIDX", "QCnt");
		dump_dma_tx_ring_info(s, dev, "T21:TXD2(H2WA)", "AP",
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_TX_RING21_CTRL0_ADDR);
		dump_dma_tx_ring_info(s, dev, "T22:TXD?(H2WA)", "AP",
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_TX_RING22_CTRL0_ADDR);
		dump_dma_rx_ring_info(s, dev, "R3:TxDone1(WA2H)", "AP",
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING3_CTRL0_ADDR);
		dump_dma_rx_ring_info(s, dev, "R5:Data1(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING5_CTRL0_ADDR);
		if (is_mt7996(&dev->mt76))
			dump_dma_rx_ring_info(s, dev, "R6:BUF1(MAC2H)", "Both",
				WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING6_CTRL0_ADDR);
		dump_dma_rx_ring_info(s, dev, "R7:TxDone1(MAC2H)", "Both",
			WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING7_CTRL0_ADDR);
		if (is_mt7992(&dev->mt76))
			dump_dma_rx_ring_info(s, dev, "R9:BUF1(MAC2H)", "Both",
				WF_WFDMA_HOST_DMA0_PCIE1_WPDMA_RX_RING9_CTRL0_ADDR);
	}

	/* MCU DMA information */
	sys_ctrl[0] = mt76_rr(dev, WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_ADDR);
	sys_ctrl[1] = mt76_rr(dev, WF_WFDMA_MCU_DMA0_HOST_INT_STA_ADDR);
	sys_ctrl[2] = mt76_rr(dev, WF_WFDMA_MCU_DMA0_HOST_INT_ENA_ADDR);

	seq_printf(s, "MCU_DMA Configuration\n");
	seq_printf(s, "%10s %10s %10s %10s %10s %10s\n",
		"DMA", "IntCSR", "IntMask", "Glocfg", "Tx/RxEn", "Tx/RxBusy");
	seq_printf(s, "%10s %10x %10x %10x %4x/%5x %4x/%5x\n",
		"DMA0", sys_ctrl[1], sys_ctrl[2], sys_ctrl[0],
		(sys_ctrl[0] & WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_MASK)
			>> WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_TX_DMA_EN_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_MASK)
			>> WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_RX_DMA_EN_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_TX_DMA_BUSY_MASK)
			>> WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_TX_DMA_BUSY_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_RX_DMA_BUSY_MASK)
			>> WF_WFDMA_MCU_DMA0_WPDMA_GLO_CFG_RX_DMA_BUSY_SHFT);

	seq_printf(s, "MCU_DMA0 Ring Configuration\n");
	seq_printf(s, "%20s %6s %10s %15s %10s %10s %10s\n",
		"Name", "Used", "Base", "Cnt", "CIDX", "DIDX", "QCnt");
	dump_dma_tx_ring_info(s, dev, "T0:Event(WM2H)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING0_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T1:Event(WA2H)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING1_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T2:TxDone0(WA2H)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING2_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T3:TxDone1(WA2H)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING3_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T4:TXD(WM2MAC)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING4_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T5:TXCMD(WM2MAC)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING5_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T6:TXD(WA2MAC)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_TX_RING6_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R0:FWDL", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING0_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R1:Cmd(H2WM)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING1_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R2:TXD0(H2WA)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING2_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R3:TXD1(H2WA)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING3_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R4:Cmd(H2WA)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING4_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R5:Data0(MAC2WM)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING5_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R6:TxDone(MAC2WM)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING6_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R7:SPL/RPT(MAC2WM)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING7_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R8:TxDone(MAC2WA)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING8_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R9:Data1(MAC2WM)", "Both",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING9_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R10:TXD2(H2WA)", "AP",
		WF_WFDMA_MCU_DMA0_WPDMA_RX_RING10_CTRL0_ADDR);

	/* MEM DMA information */
	sys_ctrl[0] = mt76_rr(dev, WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_ADDR);
	sys_ctrl[1] = mt76_rr(dev, WF_WFDMA_MEM_DMA_HOST_INT_STA_ADDR);
	sys_ctrl[2] = mt76_rr(dev, WF_WFDMA_MEM_DMA_HOST_INT_ENA_ADDR);

	seq_printf(s, "MEM_DMA Configuration\n");
	seq_printf(s, "%10s %10s %10s %10s %10s %10s\n",
		"DMA", "IntCSR", "IntMask", "Glocfg", "Tx/RxEn", "Tx/RxBusy");
	seq_printf(s, "%10s %10x %10x %10x %4x/%5x %4x/%5x\n",
		"MEM", sys_ctrl[1], sys_ctrl[2], sys_ctrl[0],
		(sys_ctrl[0] & WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_TX_DMA_EN_MASK)
			>> WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_TX_DMA_EN_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_RX_DMA_EN_MASK)
			>> WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_RX_DMA_EN_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_TX_DMA_BUSY_MASK)
			>> WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_TX_DMA_BUSY_SHFT,
		(sys_ctrl[0] & WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_RX_DMA_BUSY_MASK)
			>> WF_WFDMA_MEM_DMA_WPDMA_GLO_CFG_RX_DMA_BUSY_SHFT);

	seq_printf(s, "MEM_DMA Ring Configuration\n");
	seq_printf(s, "%20s %6s %10s %10s %10s %10s %10s\n",
		"Name", "Used", "Base", "Cnt", "CIDX", "DIDX", "QCnt");
	dump_dma_tx_ring_info(s, dev, "T0:CmdEvent(WM2WA)", "AP",
		WF_WFDMA_MEM_DMA_WPDMA_TX_RING0_CTRL0_ADDR);
	dump_dma_tx_ring_info(s, dev, "T1:CmdEvent(WA2WM)", "AP",
		WF_WFDMA_MEM_DMA_WPDMA_TX_RING1_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R0:CmdEvent(WM2WA)", "AP",
		WF_WFDMA_MEM_DMA_WPDMA_RX_RING0_CTRL0_ADDR);
	dump_dma_rx_ring_info(s, dev, "R1:CmdEvent(WA2WM)", "AP",
		WF_WFDMA_MEM_DMA_WPDMA_RX_RING1_CTRL0_ADDR);
}

static int mt7996_trinfo_read(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	mt7996_show_dma_info(s, dev);
	return 0;
}

/* MIB INFO */
static int mt7996_mibinfo_read_per_band(struct seq_file *s, int band_idx)
{
#define BSS_NUM	4
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u8 bss_nums = BSS_NUM;
	u32 idx;
	u32 mac_val, band_offset = 0, band_offset_umib = 0;
	u32 msdr6, msdr9, msdr18;
	u32 rvsr0, rscr26, rscr35, mctr5, mctr6, msr0, msr1, msr2;
	u32 tbcr0, tbcr1, tbcr2, tbcr3, tbcr4;
	u32 btscr[7];
	u32 tdrcr[5];
	u32 mbtocr[16], mbtbcr[16], mbrocr[16], mbrbcr[16];
	u32 btcr, btbcr, brocr, brbcr, btdcr, brdcr;
	u32 mu_cnt[5];
	u32 ampdu_cnt[3];
	u64 per;

	switch (band_idx) {
	case 0:
		band_offset = 0;
		band_offset_umib = 0;
		break;
	case 1:
		band_offset = BN1_WF_MIB_TOP_BASE - BN0_WF_MIB_TOP_BASE;
		band_offset_umib = WF_UMIB_TOP_B1BROCR_ADDR - WF_UMIB_TOP_B0BROCR_ADDR;
		break;
	case 2:
		band_offset = IP1_BN0_WF_MIB_TOP_BASE - BN0_WF_MIB_TOP_BASE;
		band_offset_umib = WF_UMIB_TOP_B2BROCR_ADDR - WF_UMIB_TOP_B0BROCR_ADDR;
		break;
	default:
		return true;
	}

	seq_printf(s, "Band %d MIB Status\n", band_idx);
	seq_printf(s, "===============================\n");
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_M0SCR0_ADDR + band_offset);
	seq_printf(s, "MIB Status Control=0x%x\n", mac_val);

	msdr6 = mt76_rr(dev, BN0_WF_MIB_TOP_M0SDR6_ADDR + band_offset);
	rvsr0 = mt76_rr(dev, BN0_WF_MIB_TOP_RVSR0_ADDR + band_offset);
	rscr35 = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR35_ADDR + band_offset);
	msdr9 = mt76_rr(dev, BN0_WF_MIB_TOP_M0SDR9_ADDR + band_offset);
	rscr26 = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR26_ADDR + band_offset);
	mctr5 = mt76_rr(dev, BN0_WF_MIB_TOP_MCTR5_ADDR + band_offset);
	mctr6 = mt76_rr(dev, BN0_WF_MIB_TOP_MCTR6_ADDR + band_offset);
	msdr18 = mt76_rr(dev, BN0_WF_MIB_TOP_M0SDR18_ADDR + band_offset);
	msr0 = mt76_rr(dev, BN0_WF_MIB_TOP_MSR0_ADDR + band_offset);
	msr1 = mt76_rr(dev, BN0_WF_MIB_TOP_MSR1_ADDR + band_offset);
	msr2 = mt76_rr(dev, BN0_WF_MIB_TOP_MSR2_ADDR + band_offset);
	ampdu_cnt[0] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR0_ADDR + band_offset);
	ampdu_cnt[1] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR3_ADDR + band_offset);
	ampdu_cnt[2] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR4_ADDR + band_offset);
	ampdu_cnt[1] &= BN0_WF_MIB_TOP_TSCR3_AMPDU_MPDU_COUNT_MASK;
	ampdu_cnt[2] &= BN0_WF_MIB_TOP_TSCR4_AMPDU_ACKED_COUNT_MASK;

	seq_printf(s, "===Phy/Timing Related Counters===\n");
	seq_printf(s, "\tChannelIdleCnt=0x%x\n",
		msdr6 & BN0_WF_MIB_TOP_M0SDR6_CHANNEL_IDLE_COUNT_MASK);
	seq_printf(s, "\tCCA_NAV_Tx_Time=0x%x\n",
		msdr9 & BN0_WF_MIB_TOP_M0SDR9_CCA_NAV_TX_TIME_MASK);
	seq_printf(s, "\tRx_MDRDY_CNT=0x%x\n",
		rscr26 & BN0_WF_MIB_TOP_RSCR26_RX_MDRDY_COUNT_MASK);
	seq_printf(s, "\tCCK_MDRDY_TIME=0x%x, OFDM_MDRDY_TIME=0x%x",
		msr0 & BN0_WF_MIB_TOP_MSR0_CCK_MDRDY_TIME_MASK,
		msr1 & BN0_WF_MIB_TOP_MSR1_OFDM_LG_MIXED_VHT_MDRDY_TIME_MASK);
	seq_printf(s, ", OFDM_GREEN_MDRDY_TIME=0x%x\n",
		msr2 & BN0_WF_MIB_TOP_MSR2_OFDM_GREEN_MDRDY_TIME_MASK);
	seq_printf(s, "\tPrim CCA Time=0x%x\n",
		mctr5 & BN0_WF_MIB_TOP_MCTR5_P_CCA_TIME_MASK);
	seq_printf(s, "\tSec CCA Time=0x%x\n",
		mctr6 & BN0_WF_MIB_TOP_MCTR6_S_CCA_TIME_MASK);
	seq_printf(s, "\tPrim ED Time=0x%x\n",
		msdr18 & BN0_WF_MIB_TOP_M0SDR18_P_ED_TIME_MASK);

	seq_printf(s, "===Tx Related Counters(Generic)===\n");
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR18_ADDR + band_offset);
	dev->dbg.bcn_total_cnt[band_idx] +=
		(mac_val & BN0_WF_MIB_TOP_TSCR18_BEACONTXCOUNT_MASK);
	seq_printf(s, "\tBeaconTxCnt=0x%x\n", dev->dbg.bcn_total_cnt[band_idx]);
	dev->dbg.bcn_total_cnt[band_idx] = 0;

	tbcr0 = mt76_rr(dev, BN0_WF_MIB_TOP_TBCR0_ADDR + band_offset);
	seq_printf(s, "\tTx 20MHz Cnt=0x%x\n",
		tbcr0 & BN0_WF_MIB_TOP_TBCR0_TX_20MHZ_CNT_MASK);
	tbcr1 = mt76_rr(dev, BN0_WF_MIB_TOP_TBCR1_ADDR + band_offset);
	seq_printf(s, "\tTx 40MHz Cnt=0x%x\n",
		tbcr1 & BN0_WF_MIB_TOP_TBCR1_TX_40MHZ_CNT_MASK);
	tbcr2 = mt76_rr(dev, BN0_WF_MIB_TOP_TBCR2_ADDR + band_offset);
	seq_printf(s, "\tTx 80MHz Cnt=0x%x\n",
		tbcr2 & BN0_WF_MIB_TOP_TBCR2_TX_80MHZ_CNT_MASK);
	tbcr3 = mt76_rr(dev, BN0_WF_MIB_TOP_TBCR3_ADDR + band_offset);
	seq_printf(s, "\tTx 160MHz Cnt=0x%x\n",
		tbcr3 & BN0_WF_MIB_TOP_TBCR3_TX_160MHZ_CNT_MASK);
	tbcr4 = mt76_rr(dev, BN0_WF_MIB_TOP_TBCR4_ADDR + band_offset);
	seq_printf(s, "\tTx 320MHz Cnt=0x%x\n",
		tbcr4 & BN0_WF_MIB_TOP_TBCR4_TX_320MHZ_CNT_MASK);
	seq_printf(s, "\tAMPDU Cnt=0x%x\n", ampdu_cnt[0]);
	seq_printf(s, "\tAMPDU MPDU Cnt=0x%x\n", ampdu_cnt[1]);
	seq_printf(s, "\tAMPDU MPDU Ack Cnt=0x%x\n", ampdu_cnt[2]);
	per = (ampdu_cnt[2] == 0 ?
		0 : 1000 * (ampdu_cnt[1] - ampdu_cnt[2]) / ampdu_cnt[1]);
	seq_printf(s, "\tAMPDU MPDU PER=%llu.%1llu%%\n", per / 10, per % 10);

	seq_printf(s, "===MU Related Counters===\n");
	mu_cnt[0] = mt76_rr(dev, BN0_WF_MIB_TOP_BSCR2_ADDR + band_offset);
	mu_cnt[1] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR5_ADDR + band_offset);
	mu_cnt[2] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR6_ADDR + band_offset);
	mu_cnt[3] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR8_ADDR + band_offset);
	mu_cnt[4] = mt76_rr(dev, BN0_WF_MIB_TOP_TSCR7_ADDR + band_offset);

	seq_printf(s, "\tMUBF_TX_COUNT=0x%x\n",
		mu_cnt[0] & BN0_WF_MIB_TOP_BSCR2_MUBF_TX_COUNT_MASK);
	seq_printf(s, "\tMU_TX_MPDU_COUNT(Ok+Fail)=0x%x\n", mu_cnt[1]);
	seq_printf(s, "\tMU_TX_OK_MPDU_COUNT=0x%x\n", mu_cnt[2]);
	seq_printf(s, "\tMU_TO_MU_FAIL_PPDU_COUNT=0x%x\n", mu_cnt[3]);
	seq_printf(s, "\tSU_TX_OK_MPDU_COUNT=0x%x\n", mu_cnt[4]);

	seq_printf(s, "===Rx Related Counters(Generic)===\n");
	seq_printf(s, "\tVector Mismacth Cnt=0x%x\n",
		rvsr0 & BN0_WF_MIB_TOP_RVSR0_VEC_MISS_COUNT_MASK);
	seq_printf(s, "\tDelimiter Fail Cnt=0x%x\n",
		rscr35 & BN0_WF_MIB_TOP_RSCR35_DELIMITER_FAIL_COUNT_MASK);

	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR1_ADDR + band_offset);
	seq_printf(s, "\tRxFCSErrCnt=0x%x\n",
		(mac_val & BN0_WF_MIB_TOP_RSCR1_RX_FCS_ERROR_COUNT_MASK));
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR33_ADDR + band_offset);
	seq_printf(s, "\tRxFifoFullCnt=0x%x\n",
		(mac_val & BN0_WF_MIB_TOP_RSCR33_RX_FIFO_FULL_COUNT_MASK));
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR36_ADDR + band_offset);
	seq_printf(s, "\tRxLenMismatch=0x%x\n",
		(mac_val & BN0_WF_MIB_TOP_RSCR36_RX_LEN_MISMATCH_MASK));
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR31_ADDR + band_offset);
	seq_printf(s, "\tRxMPDUCnt=0x%x\n",
		(mac_val & BN0_WF_MIB_TOP_RSCR31_RX_MPDU_COUNT_MASK));
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR27_ADDR + band_offset);
	seq_printf(s, "\tRx AMPDU Cnt=0x%x\n", mac_val);
	mac_val = mt76_rr(dev, BN0_WF_MIB_TOP_RSCR28_ADDR + band_offset);
	seq_printf(s, "\tRx Total ByteCnt=0x%x\n", mac_val);


	/* Per-BSS T/RX Counters */
	seq_printf(s, "===Per-BSS Related Tx/Rx Counters===\n");
	seq_printf(s, "BSS Idx TxCnt/DataCnt TxByteCnt RxOkCnt/DataCnt RxByteCnt\n");
	for (idx = 0; idx < bss_nums; idx++) {
		btcr = mt76_rr(dev, BN0_WF_MIB_TOP_BTCR_ADDR + band_offset + idx * 4);
		btdcr = mt76_rr(dev, BN0_WF_MIB_TOP_BTDCR_ADDR + band_offset + idx * 4);
		btbcr = mt76_rr(dev, BN0_WF_MIB_TOP_BTBCR_ADDR + band_offset + idx * 4);

		brocr = mt76_rr(dev, WF_UMIB_TOP_B0BROCR_ADDR + band_offset_umib + idx * 4);
		brdcr = mt76_rr(dev, WF_UMIB_TOP_B0BRDCR_ADDR + band_offset_umib + idx * 4);
		brbcr = mt76_rr(dev, WF_UMIB_TOP_B0BRBCR_ADDR + band_offset_umib + idx * 4);

		seq_printf(s, "%d\t 0x%x/0x%x\t 0x%x \t 0x%x/0x%x \t 0x%x\n",
			idx, btcr, btdcr, btbcr, brocr, brdcr, brbcr);
	}

	seq_printf(s, "===Per-BSS Related MIB Counters===\n");
	seq_printf(s, "BSS Idx RTSTx/RetryCnt BAMissCnt AckFailCnt FrmRetry1/2/3Cnt\n");

	/* Per-BSS TX Status */
	for (idx = 0; idx < bss_nums; idx++) {
		btscr[0] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR5_ADDR + band_offset + idx * 4);
		btscr[1] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR6_ADDR + band_offset + idx * 4);
		btscr[2] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR0_ADDR + band_offset + idx * 4);
		btscr[3] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR1_ADDR + band_offset + idx * 4);
		btscr[4] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR2_ADDR + band_offset + idx * 4);
		btscr[5] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR3_ADDR + band_offset + idx * 4);
		btscr[6] = mt76_rr(dev, BN0_WF_MIB_TOP_BTSCR4_ADDR + band_offset + idx * 4);

		seq_printf(s, "%d:\t0x%x/0x%x  0x%x \t 0x%x \t  0x%x/0x%x/0x%x\n",
			idx, (btscr[0] & BN0_WF_MIB_TOP_BTSCR5_RTSTXCOUNTn_MASK),
			(btscr[1] & BN0_WF_MIB_TOP_BTSCR6_RTSRETRYCOUNTn_MASK),
			(btscr[2] & BN0_WF_MIB_TOP_BTSCR0_BAMISSCOUNTn_MASK),
			(btscr[3] & BN0_WF_MIB_TOP_BTSCR1_ACKFAILCOUNTn_MASK),
			(btscr[4] & BN0_WF_MIB_TOP_BTSCR2_FRAMERETRYCOUNTn_MASK),
			(btscr[5] & BN0_WF_MIB_TOP_BTSCR3_FRAMERETRY2COUNTn_MASK),
			(btscr[6] & BN0_WF_MIB_TOP_BTSCR4_FRAMERETRY3COUNTn_MASK));
	}

	/* Dummy delimiter insertion result */
	seq_printf(s, "===Dummy delimiter insertion result===\n");
	tdrcr[0] = mt76_rr(dev, BN0_WF_MIB_TOP_TDRCR0_ADDR + band_offset);
	tdrcr[1] = mt76_rr(dev, BN0_WF_MIB_TOP_TDRCR1_ADDR + band_offset);
	tdrcr[2] = mt76_rr(dev, BN0_WF_MIB_TOP_TDRCR2_ADDR + band_offset);
	tdrcr[3] = mt76_rr(dev, BN0_WF_MIB_TOP_TDRCR3_ADDR + band_offset);
	tdrcr[4] = mt76_rr(dev, BN0_WF_MIB_TOP_TDRCR4_ADDR + band_offset);

	seq_printf(s, "Range0 = %d\t Range1 = %d\t Range2 = %d\t Range3 = %d\t Range4 = %d\n",
		tdrcr[0],
		tdrcr[1],
		tdrcr[2],
		tdrcr[3],
		tdrcr[4]);

	/* Per-MBSS T/RX Counters */
	seq_printf(s, "===Per-MBSS Related Tx/Rx Counters===\n");
	seq_printf(s, "MBSSIdx   TxOkCnt  TxByteCnt  RxOkCnt  RxByteCnt\n");

	for (idx = 0; idx < 16; idx++) {
		mbtocr[idx] = mt76_rr(dev, BN0_WF_MIB_TOP_BTOCR_ADDR + band_offset + (bss_nums + idx) * 4);
		mbtbcr[idx] = mt76_rr(dev, BN0_WF_MIB_TOP_BTBCR_ADDR + band_offset + (bss_nums + idx) * 4);

		mbrocr[idx] = mt76_rr(dev, WF_UMIB_TOP_B0BROCR_ADDR + band_offset_umib + (bss_nums + idx) * 4);
		mbrbcr[idx] = mt76_rr(dev, WF_UMIB_TOP_B0BRBCR_ADDR + band_offset_umib + (bss_nums + idx) * 4);
	}

	for (idx = 0; idx < 16; idx++) {
		seq_printf(s, "%d\t 0x%x\t 0x%x \t 0x%x \t 0x%x\n",
			idx, mbtocr[idx], mbtbcr[idx], mbrocr[idx], mbrbcr[idx]);
	}

	return 0;
}

static int mt7996_mibinfo_band0(struct seq_file *s, void *data)
{
	mt7996_mibinfo_read_per_band(s, MT_BAND0);
	return 0;
}

static int mt7996_mibinfo_band1(struct seq_file *s, void *data)
{
	mt7996_mibinfo_read_per_band(s, MT_BAND1);
	return 0;
}

static int mt7996_mibinfo_band2(struct seq_file *s, void *data)
{
	mt7996_mibinfo_read_per_band(s, MT_BAND2);
	return 0;
}

/* WTBL INFO */
static int
mt7996_wtbl_read_raw(struct mt7996_dev *dev, u16 idx,
		     enum mt7996_wtbl_type type, u16 start_dw,
		     u16 len, void *buf)
{
	u32 *dest_cpy = (u32 *)buf;
	u32 size_dw = len;
	u32 src = 0;

	if (!buf)
		return 0xFF;

	if (type == WTBL_TYPE_LMAC) {
		mt76_wr(dev, MT_DBG_WTBLON_TOP_WDUCR_ADDR,
			FIELD_PREP(MT_DBG_WTBLON_TOP_WDUCR_GROUP, (idx >> 7)));
		src = LWTBL_IDX2BASE(idx, start_dw);
	} else if (type == WTBL_TYPE_UMAC) {
		mt76_wr(dev,  MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			FIELD_PREP(MT_DBG_UWTBL_TOP_WDUCR_GROUP, (idx >> 7)));
		src = UWTBL_IDX2BASE(idx, start_dw);
	} else if (type == WTBL_TYPE_KEY) {
		mt76_wr(dev,  MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			MT_DBG_UWTBL_TOP_WDUCR_TARGET |
			FIELD_PREP(MT_DBG_UWTBL_TOP_WDUCR_GROUP, (idx >> 7)));
		src = KEYTBL_IDX2BASE(idx, start_dw);
	}

	while (size_dw--) {
		*dest_cpy++ = mt76_rr(dev, src);
		src += 4;
	};

	return 0;
}

#if 0
static int
mt7996_wtbl_write_raw(struct mt7996_dev *dev, u16 idx,
			  enum mt7996_wtbl_type type, u16 start_dw,
			  u32 val)
{
	u32 addr = 0;

	if (type == WTBL_TYPE_LMAC) {
		mt76_wr(dev, MT_DBG_WTBLON_TOP_WDUCR_ADDR,
			FIELD_PREP(MT_DBG_WTBLON_TOP_WDUCR_GROUP, (idx >> 7)));
		addr = LWTBL_IDX2BASE(idx, start_dw);
	} else if (type == WTBL_TYPE_UMAC) {
		mt76_wr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			FIELD_PREP(MT_DBG_UWTBL_TOP_WDUCR_GROUP, (idx >> 7)));
		addr = UWTBL_IDX2BASE(idx, start_dw);
	} else if (type == WTBL_TYPE_KEY) {
		mt76_wr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			MT_DBG_UWTBL_TOP_WDUCR_TARGET |
			FIELD_PREP(MT_DBG_UWTBL_TOP_WDUCR_GROUP, (idx >> 7)));
		addr = KEYTBL_IDX2BASE(idx, start_dw);
	}

	mt76_wr(dev, addr, val);

	return 0;
}
#endif

static const struct berse_wtbl_parse WTBL_LMAC_DW0[] = {
	{"MUAR_IDX",    WF_LWTBL_MUAR_MASK, WF_LWTBL_MUAR_SHIFT,false},
	{"RCA1",        WF_LWTBL_RCA1_MASK, NO_SHIFT_DEFINE,	false},
	{"KID",         WF_LWTBL_KID_MASK,  WF_LWTBL_KID_SHIFT,	false},
	{"RCID",        WF_LWTBL_RCID_MASK, NO_SHIFT_DEFINE,	false},
	{"BAND",        WF_LWTBL_BAND_MASK, WF_LWTBL_BAND_SHIFT,false},
	{"RV",          WF_LWTBL_RV_MASK,   NO_SHIFT_DEFINE,	false},
	{"RCA2",        WF_LWTBL_RCA2_MASK, NO_SHIFT_DEFINE,	false},
	{"WPI_FLAG",    WF_LWTBL_WPI_FLAG_MASK, NO_SHIFT_DEFINE,true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw0_1(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	seq_printf(s, "\t\n");
	seq_printf(s, "LinkAddr: %02x:%02x:%02x:%02x:%02x:%02x(D0[B0~15], D1[B0~31])\n",
		lwtbl[4], lwtbl[5], lwtbl[6], lwtbl[7], lwtbl[0], lwtbl[1]);

	/* LMAC WTBL DW 0 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 0/1\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_PEER_INFO_DW_0*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW0[i].name) {

		if (WTBL_LMAC_DW0[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW0[i].name,
					 (dw_value & WTBL_LMAC_DW0[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW0[i].name,
					  (dw_value & WTBL_LMAC_DW0[i].mask) >> WTBL_LMAC_DW0[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse *WTBL_LMAC_DW2;
static const struct berse_wtbl_parse WTBL_LMAC_DW2_7996[] = {
	{"AID",                 WF_LWTBL_AID_MASK,              WF_LWTBL_AID_SHIFT,	false},
	{"GID_SU",              WF_LWTBL_GID_SU_MASK,           NO_SHIFT_DEFINE,	false},
	{"SPP_EN",              WF_LWTBL_SPP_EN_MASK,           NO_SHIFT_DEFINE,	false},
	{"WPI_EVEN",            WF_LWTBL_WPI_EVEN_MASK,         NO_SHIFT_DEFINE,	false},
	{"AAD_OM",              WF_LWTBL_AAD_OM_MASK,           NO_SHIFT_DEFINE,	false},
	{"CIPHER_PGTK",WF_LWTBL_CIPHER_SUIT_PGTK_MASK, WF_LWTBL_CIPHER_SUIT_PGTK_SHIFT,	true},
	{"FROM_DS",             WF_LWTBL_FD_MASK,               NO_SHIFT_DEFINE,	false},
	{"TO_DS",               WF_LWTBL_TD_MASK,               NO_SHIFT_DEFINE,	false},
	{"SW",                  WF_LWTBL_SW_MASK,               NO_SHIFT_DEFINE,	false},
	{"UL",                  WF_LWTBL_UL_MASK,               NO_SHIFT_DEFINE,	false},
	{"TX_POWER_SAVE",       WF_LWTBL_TX_PS_MASK,            NO_SHIFT_DEFINE,	true},
	{"QOS",                 WF_LWTBL_QOS_MASK,              NO_SHIFT_DEFINE,	false},
	{"HT",                  WF_LWTBL_HT_MASK,               NO_SHIFT_DEFINE,	false},
	{"VHT",                 WF_LWTBL_VHT_MASK,              NO_SHIFT_DEFINE,	false},
	{"HE",                  WF_LWTBL_HE_MASK,               NO_SHIFT_DEFINE,	false},
	{"EHT",                 WF_LWTBL_EHT_MASK,              NO_SHIFT_DEFINE,	false},
	{"MESH",                WF_LWTBL_MESH_MASK,             NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_LMAC_DW2_7992[] = {
	{"AID",                 WF_LWTBL_AID_MASK,              WF_LWTBL_AID_SHIFT,	false},
	{"GID_SU",              WF_LWTBL_GID_SU_MASK,           NO_SHIFT_DEFINE,	false},
	{"DUAL_PTEC_EN",        WF_LWTBL_DUAL_PTEC_EN_MASK,     NO_SHIFT_DEFINE,	false},
	{"DUAL_CTS_CAP",        WF_LWTBL_DUAL_CTS_CAP_MASK,     NO_SHIFT_DEFINE,	false},
	{"CIPHER_PGTK",WF_LWTBL_CIPHER_SUIT_PGTK_MASK, WF_LWTBL_CIPHER_SUIT_PGTK_SHIFT,	true},
	{"FROM_DS",             WF_LWTBL_FD_MASK,               NO_SHIFT_DEFINE,	false},
	{"TO_DS",               WF_LWTBL_TD_MASK,               NO_SHIFT_DEFINE,	false},
	{"SW",                  WF_LWTBL_SW_MASK,               NO_SHIFT_DEFINE,	false},
	{"UL",                  WF_LWTBL_UL_MASK,               NO_SHIFT_DEFINE,	false},
	{"TX_POWER_SAVE",       WF_LWTBL_TX_PS_MASK,            NO_SHIFT_DEFINE,	true},
	{"QOS",                 WF_LWTBL_QOS_MASK,              NO_SHIFT_DEFINE,	false},
	{"HT",                  WF_LWTBL_HT_MASK,               NO_SHIFT_DEFINE,	false},
	{"VHT",                 WF_LWTBL_VHT_MASK,              NO_SHIFT_DEFINE,	false},
	{"HE",                  WF_LWTBL_HE_MASK,               NO_SHIFT_DEFINE,	false},
	{"EHT",                 WF_LWTBL_EHT_MASK,              NO_SHIFT_DEFINE,	false},
	{"MESH",                WF_LWTBL_MESH_MASK,             NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw2(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 2 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 2\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_2*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW2[i].name) {

		if (WTBL_LMAC_DW2[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW2[i].name,
					 (dw_value & WTBL_LMAC_DW2[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW2[i].name,
					  (dw_value & WTBL_LMAC_DW2[i].mask) >> WTBL_LMAC_DW2[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW3[] = {
	{"WMM_Q",           WF_LWTBL_WMM_Q_MASK,		WF_LWTBL_WMM_Q_SHIFT,		false},
	{"EHT_SIG_MCS",     WF_LWTBL_EHT_SIG_MCS_MASK,		WF_LWTBL_EHT_SIG_MCS_SHIFT,	false},
	{"HDRT_MODE",       WF_LWTBL_HDRT_MODE_MASK,		NO_SHIFT_DEFINE,		false},
	{"BEAM_CHG",        WF_LWTBL_BEAM_CHG_MASK,		NO_SHIFT_DEFINE,		false},
	{"EHT_LTF_SYM_NUM", WF_LWTBL_EHT_LTF_SYM_NUM_OPT_MASK,  WF_LWTBL_EHT_LTF_SYM_NUM_OPT_SHIFT,	true},
	{"PFMU_IDX",	    WF_LWTBL_PFMU_IDX_MASK,		WF_LWTBL_PFMU_IDX_SHIFT,	false},
	{"ULPF_IDX",	    WF_LWTBL_ULPF_IDX_MASK,		WF_LWTBL_ULPF_IDX_SHIFT,	false},
	{"RIBF",	    WF_LWTBL_RIBF_MASK,			NO_SHIFT_DEFINE,		false},
	{"ULPF",	    WF_LWTBL_ULPF_MASK,			NO_SHIFT_DEFINE,		false},
	{"BYPASS_TXSMM",    WF_LWTBL_BYPASS_TXSMM_MASK,         NO_SHIFT_DEFINE,		true},
	{"TBF_HT",          WF_LWTBL_TBF_HT_MASK,		NO_SHIFT_DEFINE,		false},
	{"TBF_VHT",         WF_LWTBL_TBF_VHT_MASK,		NO_SHIFT_DEFINE,		false},
	{"TBF_HE",          WF_LWTBL_TBF_HE_MASK,		NO_SHIFT_DEFINE,		false},
	{"TBF_EHT",         WF_LWTBL_TBF_EHT_MASK,		NO_SHIFT_DEFINE,		false},
	{"IGN_FBK",         WF_LWTBL_IGN_FBK_MASK,		NO_SHIFT_DEFINE,		true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw3(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 3 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 3\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_3*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW3[i].name) {

		if (WTBL_LMAC_DW3[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW3[i].name,
					 (dw_value & WTBL_LMAC_DW3[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW3[i].name,
					  (dw_value & WTBL_LMAC_DW3[i].mask) >> WTBL_LMAC_DW3[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW4[] = {
	{"NEGOTIATED_WINSIZE0",	WF_LWTBL_NEGOTIATED_WINSIZE0_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE0_SHIFT,    false},
	{"WINSIZE1",	WF_LWTBL_NEGOTIATED_WINSIZE1_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE1_SHIFT,    false},
	{"WINSIZE2",	WF_LWTBL_NEGOTIATED_WINSIZE2_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE2_SHIFT,    false},
	{"WINSIZE3",	WF_LWTBL_NEGOTIATED_WINSIZE3_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE3_SHIFT,    true},
	{"WINSIZE4",	WF_LWTBL_NEGOTIATED_WINSIZE4_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE4_SHIFT,    false},
	{"WINSIZE5",	WF_LWTBL_NEGOTIATED_WINSIZE5_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE5_SHIFT,    false},
	{"WINSIZE6",	WF_LWTBL_NEGOTIATED_WINSIZE6_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE6_SHIFT,    false},
	{"WINSIZE7",	WF_LWTBL_NEGOTIATED_WINSIZE7_MASK,	WF_LWTBL_NEGOTIATED_WINSIZE7_SHIFT,    true},
	{"PE",              WF_LWTBL_PE_MASK,           WF_LWTBL_PE_SHIFT,	false},
	{"DIS_RHTR",        WF_LWTBL_DIS_RHTR_MASK,     NO_SHIFT_DEFINE,	false},
	{"LDPC_HT",         WF_LWTBL_LDPC_HT_MASK,      NO_SHIFT_DEFINE,	false},
	{"LDPC_VHT",        WF_LWTBL_LDPC_VHT_MASK,     NO_SHIFT_DEFINE,	false},
	{"LDPC_HE",         WF_LWTBL_LDPC_HE_MASK,      NO_SHIFT_DEFINE,	false},
	{"LDPC_EHT",	    WF_LWTBL_LDPC_EHT_MASK,	NO_SHIFT_DEFINE,	true},
	{"BA_MODE",	    WF_LWTBL_BA_MODE_MASK,	NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw4(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 4 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 4\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_4*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW4[i].name) {
		if (WTBL_LMAC_DW4[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW4[i].name,
					 (dw_value & WTBL_LMAC_DW4[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW4[i].name,
					  (dw_value & WTBL_LMAC_DW4[i].mask) >> WTBL_LMAC_DW4[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse *WTBL_LMAC_DW5;
static const struct berse_wtbl_parse WTBL_LMAC_DW5_7996[] = {
	{"AF",                  WF_LWTBL_AF_MASK,           WF_LWTBL_AF_SHIFT,	false},
	{"AF_HE",               WF_LWTBL_AF_HE_MASK,        WF_LWTBL_AF_HE_SHIFT,false},
	{"RTS",                 WF_LWTBL_RTS_MASK,          NO_SHIFT_DEFINE,	false},
	{"SMPS",                WF_LWTBL_SMPS_MASK,         NO_SHIFT_DEFINE,	false},
	{"DYN_BW",              WF_LWTBL_DYN_BW_MASK,       NO_SHIFT_DEFINE,	true},
	{"MMSS",                WF_LWTBL_MMSS_MASK,         WF_LWTBL_MMSS_SHIFT,false},
	{"USR",                 WF_LWTBL_USR_MASK,          NO_SHIFT_DEFINE,	false},
	{"SR_RATE",             WF_LWTBL_SR_R_MASK,         WF_LWTBL_SR_R_SHIFT,false},
	{"SR_ABORT",            WF_LWTBL_SR_ABORT_MASK,     NO_SHIFT_DEFINE,	true},
	{"TX_POWER_OFFSET",     WF_LWTBL_TX_POWER_OFFSET_MASK,  WF_LWTBL_TX_POWER_OFFSET_SHIFT,	false},
	{"LTF_EHT",		WF_LWTBL_LTF_EHT_MASK,      WF_LWTBL_LTF_EHT_SHIFT, false},
	{"GI_EHT",		WF_LWTBL_GI_EHT_MASK,       WF_LWTBL_GI_EHT_SHIFT, false},
	{"DOPPL",               WF_LWTBL_DOPPL_MASK,        NO_SHIFT_DEFINE,	false},
	{"TXOP_PS_CAP",         WF_LWTBL_TXOP_PS_CAP_MASK,  NO_SHIFT_DEFINE,	false},
	{"DONOT_UPDATE_I_PSM",  WF_LWTBL_DU_I_PSM_MASK,     NO_SHIFT_DEFINE,	true},
	{"I_PSM",               WF_LWTBL_I_PSM_MASK,        NO_SHIFT_DEFINE,	false},
	{"PSM",                 WF_LWTBL_PSM_MASK,          NO_SHIFT_DEFINE,	false},
	{"SKIP_TX",             WF_LWTBL_SKIP_TX_MASK,      NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_LMAC_DW5_7992[] = {
	{"AF",                  WF_LWTBL_AF_MASK_7992,      WF_LWTBL_AF_SHIFT,	false},
	{"RTS",                 WF_LWTBL_RTS_MASK,          NO_SHIFT_DEFINE,	false},
	{"SMPS",                WF_LWTBL_SMPS_MASK,         NO_SHIFT_DEFINE,	false},
	{"DYN_BW",              WF_LWTBL_DYN_BW_MASK,       NO_SHIFT_DEFINE,	true},
	{"MMSS",                WF_LWTBL_MMSS_MASK,         WF_LWTBL_MMSS_SHIFT,false},
	{"USR",                 WF_LWTBL_USR_MASK,          NO_SHIFT_DEFINE,	false},
	{"SR_RATE",             WF_LWTBL_SR_R_MASK,         WF_LWTBL_SR_R_SHIFT,false},
	{"SR_ABORT",            WF_LWTBL_SR_ABORT_MASK,     NO_SHIFT_DEFINE,	true},
	{"TX_POWER_OFFSET",     WF_LWTBL_TX_POWER_OFFSET_MASK,  WF_LWTBL_TX_POWER_OFFSET_SHIFT,	false},
	{"LTF_EHT",		WF_LWTBL_LTF_EHT_MASK,      WF_LWTBL_LTF_EHT_SHIFT, false},
	{"GI_EHT",		WF_LWTBL_GI_EHT_MASK,       WF_LWTBL_GI_EHT_SHIFT, false},
	{"DOPPL",               WF_LWTBL_DOPPL_MASK,        NO_SHIFT_DEFINE,	false},
	{"TXOP_PS_CAP",         WF_LWTBL_TXOP_PS_CAP_MASK,  NO_SHIFT_DEFINE,	false},
	{"DONOT_UPDATE_I_PSM",  WF_LWTBL_DU_I_PSM_MASK,     NO_SHIFT_DEFINE,	true},
	{"I_PSM",               WF_LWTBL_I_PSM_MASK,        NO_SHIFT_DEFINE,	false},
	{"PSM",                 WF_LWTBL_PSM_MASK,          NO_SHIFT_DEFINE,	false},
	{"SKIP_TX",             WF_LWTBL_SKIP_TX_MASK,      NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw5(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 5 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 5\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_5*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW5[i].name) {
		if (WTBL_LMAC_DW5[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW5[i].name,
					 (dw_value & WTBL_LMAC_DW5[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW5[i].name,
					  (dw_value & WTBL_LMAC_DW5[i].mask) >> WTBL_LMAC_DW5[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW6[] = {
	{"CBRN",        WF_LWTBL_CBRN_MASK,	    WF_LWTBL_CBRN_SHIFT,	false},
	{"DBNSS_EN",    WF_LWTBL_DBNSS_EN_MASK, NO_SHIFT_DEFINE,	false},
	{"BAF_EN",      WF_LWTBL_BAF_EN_MASK,   NO_SHIFT_DEFINE,	false},
	{"RDGBA",       WF_LWTBL_RDGBA_MASK,    NO_SHIFT_DEFINE,	false},
	{"RDG",         WF_LWTBL_R_MASK,        NO_SHIFT_DEFINE,	false},
	{"SPE_IDX",     WF_LWTBL_SPE_IDX_MASK,  WF_LWTBL_SPE_IDX_SHIFT,	true},
	{"G2",          WF_LWTBL_G2_MASK,       NO_SHIFT_DEFINE,	false},
	{"G4",          WF_LWTBL_G4_MASK,       NO_SHIFT_DEFINE,	false},
	{"G8",          WF_LWTBL_G8_MASK,       NO_SHIFT_DEFINE,	false},
	{"G16",         WF_LWTBL_G16_MASK,      NO_SHIFT_DEFINE,	true},
	{"G2_LTF",      WF_LWTBL_G2_LTF_MASK,   WF_LWTBL_G2_LTF_SHIFT,	false},
	{"G4_LTF",      WF_LWTBL_G4_LTF_MASK,   WF_LWTBL_G4_LTF_SHIFT,	false},
	{"G8_LTF",      WF_LWTBL_G8_LTF_MASK,   WF_LWTBL_G8_LTF_SHIFT,	false},
	{"G16_LTF",     WF_LWTBL_G16_LTF_MASK,  WF_LWTBL_G16_LTF_SHIFT,	true},
	{"G2_HE",       WF_LWTBL_G2_HE_MASK,    WF_LWTBL_G2_HE_SHIFT,	false},
	{"G4_HE",       WF_LWTBL_G4_HE_MASK,    WF_LWTBL_G4_HE_SHIFT,	false},
	{"G8_HE",       WF_LWTBL_G8_HE_MASK,    WF_LWTBL_G8_HE_SHIFT,	false},
	{"G16_HE",      WF_LWTBL_G16_HE_MASK,   WF_LWTBL_G16_HE_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw6(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 6 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 6\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_6*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW6[i].name) {
		if (WTBL_LMAC_DW6[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW6[i].name,
					 (dw_value & WTBL_LMAC_DW6[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW6[i].name,
					  (dw_value & WTBL_LMAC_DW6[i].mask) >> WTBL_LMAC_DW6[i].shift);
		i++;
	}
}

static void parse_fmac_lwtbl_dw7(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	int i = 0;

	/* LMAC WTBL DW 7 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 7\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_7*4]);
	dw_value = *addr;

	for (i = 0; i < 8; i++) {
		seq_printf(s, "\tBA_WIN_SIZE%u:%lu\n", i, ((dw_value & BITS(i*4, i*4+3)) >> i*4));
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW8[] = {
	{"RTS_FAIL_CNT_AC0",    WF_LWTBL_AC0_RTS_FAIL_CNT_MASK,	WF_LWTBL_AC0_RTS_FAIL_CNT_SHIFT,	false},
	{"AC1",                 WF_LWTBL_AC1_RTS_FAIL_CNT_MASK,	WF_LWTBL_AC1_RTS_FAIL_CNT_SHIFT,	false},
	{"AC2",                 WF_LWTBL_AC2_RTS_FAIL_CNT_MASK,	WF_LWTBL_AC2_RTS_FAIL_CNT_SHIFT,	false},
	{"AC3",                 WF_LWTBL_AC3_RTS_FAIL_CNT_MASK,	WF_LWTBL_AC3_RTS_FAIL_CNT_SHIFT,	true},
	{"PARTIAL_AID",         WF_LWTBL_PARTIAL_AID_MASK,		WF_LWTBL_PARTIAL_AID_SHIFT,	false},
	{"CHK_PER",             WF_LWTBL_CHK_PER_MASK,		NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw8(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 8 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 8\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_8*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW8[i].name) {
		if (WTBL_LMAC_DW8[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW8[i].name,
					 (dw_value & WTBL_LMAC_DW8[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW8[i].name,
					  (dw_value & WTBL_LMAC_DW8[i].mask) >> WTBL_LMAC_DW8[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse *WTBL_LMAC_DW9;
static const struct berse_wtbl_parse WTBL_LMAC_DW9_7996[] = {
	{"RX_AVG_MPDU_SIZE",    WF_LWTBL_RX_AVG_MPDU_SIZE_MASK,    WF_LWTBL_RX_AVG_MPDU_SIZE_SHIFT,	false},
	{"PRITX_SW_MODE",       WF_LWTBL_PRITX_SW_MODE_MASK,       NO_SHIFT_DEFINE,	false},
	{"PRITX_ERSU",	    WF_LWTBL_PRITX_ERSU_MASK,	       NO_SHIFT_DEFINE,	false},
	{"PRITX_PLR",           WF_LWTBL_PRITX_PLR_MASK,           NO_SHIFT_DEFINE,	true},
	{"PRITX_DCM",           WF_LWTBL_PRITX_DCM_MASK,           NO_SHIFT_DEFINE,	false},
	{"PRITX_ER106T",        WF_LWTBL_PRITX_ER106T_MASK,        NO_SHIFT_DEFINE,	true},
	/* {"FCAP(0:20 1:~40)",    WTBL_FCAP_20_TO_160_MHZ,	WTBL_FCAP_20_TO_160_MHZ_OFFSET}, */
	{"MPDU_FAIL_CNT",       WF_LWTBL_MPDU_FAIL_CNT_MASK,       WF_LWTBL_MPDU_FAIL_CNT_SHIFT,	false},
	{"MPDU_OK_CNT",         WF_LWTBL_MPDU_OK_CNT_MASK,         WF_LWTBL_MPDU_OK_CNT_SHIFT,	false},
	{"RATE_IDX",            WF_LWTBL_RATE_IDX_MASK,            WF_LWTBL_RATE_IDX_SHIFT,	true},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_LMAC_DW9_7992[] = {
	{"RX_AVG_MPDU_SIZE",    WF_LWTBL_RX_AVG_MPDU_SIZE_MASK,    WF_LWTBL_RX_AVG_MPDU_SIZE_SHIFT,	false},
	{"PRITX_SW_MODE",       WF_LWTBL_PRITX_SW_MODE_MASK_7992,       NO_SHIFT_DEFINE,	false},
	{"PRITX_ERSU",	    WF_LWTBL_PRITX_ERSU_MASK_7992,	       NO_SHIFT_DEFINE,	false},
	{"PRITX_PLR",           WF_LWTBL_PRITX_PLR_MASK_7992,           NO_SHIFT_DEFINE,	true},
	{"PRITX_DCM",           WF_LWTBL_PRITX_DCM_MASK,           NO_SHIFT_DEFINE,	false},
	{"PRITX_ER106T",        WF_LWTBL_PRITX_ER106T_MASK,        NO_SHIFT_DEFINE,	true},
	/* {"FCAP(0:20 1:~40)",    WTBL_FCAP_20_TO_160_MHZ,	WTBL_FCAP_20_TO_160_MHZ_OFFSET}, */
	{"MPDU_FAIL_CNT",       WF_LWTBL_MPDU_FAIL_CNT_MASK,       WF_LWTBL_MPDU_FAIL_CNT_SHIFT,	false},
	{"MPDU_OK_CNT",         WF_LWTBL_MPDU_OK_CNT_MASK,         WF_LWTBL_MPDU_OK_CNT_SHIFT,	false},
	{"RATE_IDX",            WF_LWTBL_RATE_IDX_MASK,            WF_LWTBL_RATE_IDX_SHIFT,	true},
	{NULL,}
};

char *fcap_name[] = {"20MHz", "20/40MHz", "20/40/80MHz", "20/40/80/160/80+80MHz", "20/40/80/160/80+80/320MHz"};

static void parse_fmac_lwtbl_dw9(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 9 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 9\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_9*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW9[i].name) {
		if (WTBL_LMAC_DW9[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW9[i].name,
					 (dw_value & WTBL_LMAC_DW9[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW9[i].name,
					  (dw_value & WTBL_LMAC_DW9[i].mask) >> WTBL_LMAC_DW9[i].shift);
		i++;
	}

	/* FCAP parser */
	seq_printf(s, "\t\n");
	seq_printf(s, "FCAP:%s\n", fcap_name[(dw_value & WF_LWTBL_FCAP_MASK) >> WF_LWTBL_FCAP_SHIFT]);
}

#define HW_TX_RATE_TO_MODE(_x)			(((_x) & WTBL_RATE_TX_MODE_MASK) >> WTBL_RATE_TX_MODE_OFFSET)
#define HW_TX_RATE_TO_MCS(_x, _mode)		((_x) & WTBL_RATE_TX_RATE_MASK >> WTBL_RATE_TX_RATE_OFFSET)
#define HW_TX_RATE_TO_NSS(_x)			(((_x) & WTBL_RATE_NSTS_MASK) >> WTBL_RATE_NSTS_OFFSET)
#define HW_TX_RATE_TO_STBC(_x)			(((_x) & WTBL_RATE_STBC_MASK) >> WTBL_RATE_STBC_OFFSET)

#define MAX_TX_MODE 16
static char *HW_TX_MODE_STR[] = {"CCK", "OFDM", "HT-Mix", "HT-GF", "VHT",
				 "N/A", "N/A", "N/A",
				 "HE_SU", "HE_EXT_SU", "HE_TRIG", "HE_MU",
				 "N/A",
				 "EHT_EXT_SU", "EHT_TRIG", "EHT_MU",
				 "N/A"};
static char *HW_TX_RATE_CCK_STR[] = {"1M", "2Mlong", "5.5Mlong", "11Mlong", "N/A", "2Mshort", "5.5Mshort", "11Mshort", "N/A"};
static char *HW_TX_RATE_OFDM_STR[] = {"6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M", "N/A"};

static char *hw_rate_ofdm_str(uint16_t ofdm_idx)
{
	switch (ofdm_idx) {
	case 11: /* 6M */
		return HW_TX_RATE_OFDM_STR[0];

	case 15: /* 9M */
		return HW_TX_RATE_OFDM_STR[1];

	case 10: /* 12M */
		return HW_TX_RATE_OFDM_STR[2];

	case 14: /* 18M */
		return HW_TX_RATE_OFDM_STR[3];

	case 9: /* 24M */
		return HW_TX_RATE_OFDM_STR[4];

	case 13: /* 36M */
		return HW_TX_RATE_OFDM_STR[5];

	case 8: /* 48M */
		return HW_TX_RATE_OFDM_STR[6];

	case 12: /* 54M */
		return HW_TX_RATE_OFDM_STR[7];

	default:
		return HW_TX_RATE_OFDM_STR[8];
	}
}

static char *hw_rate_str(u8 mode, uint16_t rate_idx)
{
	if (mode == 0)
		return rate_idx < 8 ? HW_TX_RATE_CCK_STR[rate_idx] : HW_TX_RATE_CCK_STR[8];
	else if (mode == 1)
		return hw_rate_ofdm_str(rate_idx);
	else
		return "MCS";
}

static void
parse_rate(struct seq_file *s, uint16_t rate_idx, uint16_t txrate)
{
	uint16_t txmode, mcs, nss, stbc;

	txmode = HW_TX_RATE_TO_MODE(txrate);
	mcs = HW_TX_RATE_TO_MCS(txrate, txmode);
	nss = HW_TX_RATE_TO_NSS(txrate);
	stbc = HW_TX_RATE_TO_STBC(txrate);

	seq_printf(s, "\tRate%d(0x%x):TxMode=%d(%s), TxRate=%d(%s), Nsts=%d, STBC=%d\n",
			  rate_idx + 1, txrate,
			  txmode, (txmode < MAX_TX_MODE ? HW_TX_MODE_STR[txmode] : HW_TX_MODE_STR[MAX_TX_MODE]),
			  mcs, hw_rate_str(txmode, mcs), nss, stbc);
}


static const struct berse_wtbl_parse WTBL_LMAC_DW10[] = {
	{"RATE1",       WF_LWTBL_RATE1_MASK,        WF_LWTBL_RATE1_SHIFT},
	{"RATE2",       WF_LWTBL_RATE2_MASK,        WF_LWTBL_RATE2_SHIFT},
	{NULL,}
};

static void parse_fmac_lwtbl_dw10(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 10 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 10\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_AUTO_RATE_1_2*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW10[i].name) {
		parse_rate(s, i, (dw_value & WTBL_LMAC_DW10[i].mask) >> WTBL_LMAC_DW10[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW11[] = {
	{"RATE3",       WF_LWTBL_RATE3_MASK,        WF_LWTBL_RATE3_SHIFT},
	{"RATE4",       WF_LWTBL_RATE4_MASK,        WF_LWTBL_RATE4_SHIFT},
	{NULL,}
};

static void parse_fmac_lwtbl_dw11(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 11 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 11\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_AUTO_RATE_3_4*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW11[i].name) {
		parse_rate(s, i+2, (dw_value & WTBL_LMAC_DW11[i].mask) >> WTBL_LMAC_DW11[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW12[] = {
	{"RATE5",       WF_LWTBL_RATE5_MASK,        WF_LWTBL_RATE5_SHIFT},
	{"RATE6",       WF_LWTBL_RATE6_MASK,        WF_LWTBL_RATE6_SHIFT},
	{NULL,}
};

static void parse_fmac_lwtbl_dw12(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 12 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 12\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_AUTO_RATE_5_6*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW12[i].name) {
		parse_rate(s, i+4, (dw_value & WTBL_LMAC_DW12[i].mask) >> WTBL_LMAC_DW12[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW13[] = {
	{"RATE7",       WF_LWTBL_RATE7_MASK,        WF_LWTBL_RATE7_SHIFT},
	{"RATE8",       WF_LWTBL_RATE8_MASK,        WF_LWTBL_RATE8_SHIFT},
	{NULL,}
};

static void parse_fmac_lwtbl_dw13(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 13 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 13\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_AUTO_RATE_7_8*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW13[i].name) {
		parse_rate(s, i+6, (dw_value & WTBL_LMAC_DW13[i].mask) >> WTBL_LMAC_DW13[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW14_BMC[] = {
	{"CIPHER_IGTK",         WF_LWTBL_CIPHER_SUIT_IGTK_MASK,    WF_LWTBL_CIPHER_SUIT_IGTK_SHIFT,		false},
	{"CIPHER_BIGTK",        WF_LWTBL_CIPHER_SUIT_BIGTK_MASK,   WF_LWTBL_CIPHER_SUIT_BIGTK_SHIFT,	true},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_LMAC_DW14[] = {
	{"RATE1_TX_CNT",      WF_LWTBL_RATE1_TX_CNT_MASK,     WF_LWTBL_RATE1_TX_CNT_SHIFT,   false},
	{"RATE1_FAIL_CNT",    WF_LWTBL_RATE1_FAIL_CNT_MASK,   WF_LWTBL_RATE1_FAIL_CNT_SHIFT, true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw14(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr, *muar_addr = 0;
	u32 dw_value, muar_dw_value = 0;
	u16 i = 0;

	/* DUMP DW14 for BMC entry only */
	muar_addr = (u32 *)&(lwtbl[WF_LWTBL_MUAR_DW*4]);
	muar_dw_value = *muar_addr;
	if (((muar_dw_value & WF_LWTBL_MUAR_MASK) >> WF_LWTBL_MUAR_SHIFT)
		== MUAR_INDEX_OWN_MAC_ADDR_BC_MC) {
		/* LMAC WTBL DW 14 */
		seq_printf(s, "\t\n");
		seq_printf(s, "LWTBL DW 14\n");
		addr = (u32 *)&(lwtbl[WF_LWTBL_CIPHER_SUIT_IGTK_DW*4]);
		dw_value = *addr;

		while (WTBL_LMAC_DW14_BMC[i].name) {
			if (WTBL_LMAC_DW14_BMC[i].shift == NO_SHIFT_DEFINE)
				seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW14_BMC[i].name,
					(dw_value & WTBL_LMAC_DW14_BMC[i].mask) ? 1 : 0);
			else
				seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW14_BMC[i].name,
					(dw_value & WTBL_LMAC_DW14_BMC[i].mask) >> WTBL_LMAC_DW14_BMC[i].shift);
			i++;
		}
	} else {
		seq_printf(s, "\t\n");
		seq_printf(s, "LWTBL DW 14\n");
		addr = (u32 *)&(lwtbl[WF_LWTBL_CIPHER_SUIT_IGTK_DW*4]);
		dw_value = *addr;

		while (WTBL_LMAC_DW14[i].name) {
			if (WTBL_LMAC_DW14[i].shift == NO_SHIFT_DEFINE)
				seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW14[i].name,
					(dw_value & WTBL_LMAC_DW14[i].mask) ? 1 : 0);
			else
				seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW14[i].name,
					(dw_value & WTBL_LMAC_DW14[i].mask) >> WTBL_LMAC_DW14[i].shift);
			i++;
		}
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW28[] = {
	{"RELATED_IDX0",	WF_LWTBL_RELATED_IDX0_MASK,		WF_LWTBL_RELATED_IDX0_SHIFT,	false},
	{"RELATED_BAND0",	WF_LWTBL_RELATED_BAND0_MASK,		WF_LWTBL_RELATED_BAND0_SHIFT,	false},
	{"PRI_MLD_BAND",    WF_LWTBL_PRIMARY_MLD_BAND_MASK,		WF_LWTBL_PRIMARY_MLD_BAND_SHIFT,	true},
	{"RELATED_IDX1",	WF_LWTBL_RELATED_IDX1_MASK,		WF_LWTBL_RELATED_IDX1_SHIFT,	false},
	{"RELATED_BAND1",   WF_LWTBL_RELATED_BAND1_MASK,		WF_LWTBL_RELATED_BAND1_SHIFT,	false},
	{"SEC_MLD_BAND",	WF_LWTBL_SECONDARY_MLD_BAND_MASK,	WF_LWTBL_SECONDARY_MLD_BAND_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw28(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 28 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 28\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_MLO_INFO_LINE_1*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW28[i].name) {
		if (WTBL_LMAC_DW28[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW28[i].name,
				(dw_value & WTBL_LMAC_DW28[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW28[i].name,
				(dw_value & WTBL_LMAC_DW28[i].mask) >>
					WTBL_LMAC_DW28[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW29[] = {
	{"DISPATCH_POLICY_MLD_TID0", WF_LWTBL_DISPATCH_POLICY0_MASK,	WF_LWTBL_DISPATCH_POLICY0_SHIFT,	false},
	{"MLD_TID1",	WF_LWTBL_DISPATCH_POLICY1_MASK,		WF_LWTBL_DISPATCH_POLICY1_SHIFT,	false},
	{"MLD_TID2",	WF_LWTBL_DISPATCH_POLICY2_MASK,		WF_LWTBL_DISPATCH_POLICY2_SHIFT,	false},
	{"MLD_TID3",	WF_LWTBL_DISPATCH_POLICY3_MASK,	WF_LWTBL_DISPATCH_POLICY3_SHIFT,	true},
	{"MLD_TID4",	WF_LWTBL_DISPATCH_POLICY4_MASK,		WF_LWTBL_DISPATCH_POLICY4_SHIFT,	false},
	{"MLD_TID5",	WF_LWTBL_DISPATCH_POLICY5_MASK,		WF_LWTBL_DISPATCH_POLICY5_SHIFT,	false},
	{"MLD_TID6",	WF_LWTBL_DISPATCH_POLICY6_MASK,		WF_LWTBL_DISPATCH_POLICY6_SHIFT,	false},
	{"MLD_TID7",	WF_LWTBL_DISPATCH_POLICY7_MASK,		WF_LWTBL_DISPATCH_POLICY7_SHIFT,	true},
	{"OMLD_ID",		WF_LWTBL_OWN_MLD_ID_MASK,	WF_LWTBL_OWN_MLD_ID_SHIFT,	false},
	{"EMLSR0",		WF_LWTBL_EMLSR0_MASK,		NO_SHIFT_DEFINE,	false},
	{"EMLMR0",		WF_LWTBL_EMLMR0_MASK,		NO_SHIFT_DEFINE,	false},
	{"EMLSR1",		WF_LWTBL_EMLSR1_MASK,		NO_SHIFT_DEFINE,	false},
	{"EMLMR1",		WF_LWTBL_EMLMR1_MASK,		NO_SHIFT_DEFINE,	true},
	{"EMLSR2",		WF_LWTBL_EMLSR2_MASK,		NO_SHIFT_DEFINE,	false},
	{"EMLMR2",		WF_LWTBL_EMLMR2_MASK,		NO_SHIFT_DEFINE,	false},
	{"STR_BITMAP",	WF_LWTBL_STR_BITMAP_MASK,	WF_LWTBL_STR_BITMAP_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw29(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 29 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 29\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_MLO_INFO_LINE_2*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW29[i].name) {
		if (WTBL_LMAC_DW29[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW29[i].name,
				(dw_value & WTBL_LMAC_DW29[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW29[i].name,
				(dw_value & WTBL_LMAC_DW29[i].mask) >>
					WTBL_LMAC_DW29[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW30[] = {
	{"DISPATCH_ORDER",	WF_LWTBL_DISPATCH_ORDER_MASK,	WF_LWTBL_DISPATCH_ORDER_SHIFT,	false},
	{"DISPATCH_RATIO",	WF_LWTBL_DISPATCH_RATIO_MASK,	WF_LWTBL_DISPATCH_RATIO_SHIFT,	false},
	{"LINK_MGF",		WF_LWTBL_LINK_MGF_MASK,		WF_LWTBL_LINK_MGF_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw30(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 30 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 30\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_MLO_INFO_LINE_3*4]);
	dw_value = *addr;


	while (WTBL_LMAC_DW30[i].name) {
		if (WTBL_LMAC_DW30[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW30[i].name,
				(dw_value & WTBL_LMAC_DW30[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW30[i].name,
				(dw_value & WTBL_LMAC_DW30[i].mask) >> WTBL_LMAC_DW30[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW31[] = {
	{"BFTX_TB",          WF_LWTBL_BFTX_TB_MASK,                 NO_SHIFT_DEFINE,    false},
	{"DROP",          WF_LWTBL_DROP_MASK,                 NO_SHIFT_DEFINE,    false},
	{"CASCAD",	        WF_LWTBL_CASCAD_MASK,			NO_SHIFT_DEFINE,    false},
	{"ALL_ACK",	        WF_LWTBL_ALL_ACK_MASK,			NO_SHIFT_DEFINE,    false},
	{"MPDU_SIZE",	WF_LWTBL_MPDU_SIZE_MASK,		WF_LWTBL_MPDU_SIZE_SHIFT,  false},
	{"RXD_DUP_MODE",	WF_LWTBL_RXD_DUP_MODE_MASK,			WF_LWTBL_RXD_DUP_MODE_SHIFT,  true},
	{"ACK_EN",		WF_LWTBL_ACK_EN_MASK,			NO_SHIFT_DEFINE,		true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw31(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 31 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 31\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_RESP_INFO_DW_31*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW31[i].name) {
		if (WTBL_LMAC_DW31[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW31[i].name,
				(dw_value & WTBL_LMAC_DW31[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW31[i].name,
				(dw_value & WTBL_LMAC_DW31[i].mask) >>
					WTBL_LMAC_DW31[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW32[] = {
	{"OM_INFO",			WF_LWTBL_OM_INFO_MASK,			WF_LWTBL_OM_INFO_SHIFT,		false},
	{"OM_INFO_EHT",         WF_LWTBL_OM_INFO_EHT_MASK,         WF_LWTBL_OM_INFO_EHT_SHIFT,  false},
	{"RXD_DUP_FOR_OM_CHG",		WF_LWTBL_RXD_DUP_FOR_OM_CHG_MASK,	NO_SHIFT_DEFINE,		false},
	{"RXD_DUP_WHITE_LIST",	WF_LWTBL_RXD_DUP_WHITE_LIST_MASK,	WF_LWTBL_RXD_DUP_WHITE_LIST_SHIFT,	false},
	{NULL,}
};

static void parse_fmac_lwtbl_dw32(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 32 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 32\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_RX_DUP_INFO_DW_32*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW32[i].name) {
		if (WTBL_LMAC_DW32[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW32[i].name,
				(dw_value & WTBL_LMAC_DW32[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW32[i].name,
				(dw_value & WTBL_LMAC_DW32[i].mask) >>
					WTBL_LMAC_DW32[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW33[] = {
	{"USER_RSSI",                   WF_LWTBL_USER_RSSI_MASK,            WF_LWTBL_USER_RSSI_SHIFT,	false},
	{"USER_SNR",                    WF_LWTBL_USER_SNR_MASK,             WF_LWTBL_USER_SNR_SHIFT,	false},
	{"RAPID_REACTION_RATE",         WF_LWTBL_RAPID_REACTION_RATE_MASK,  WF_LWTBL_RAPID_REACTION_RATE_SHIFT,	true},
	{"HT_AMSDU(Read Only)",         WF_LWTBL_HT_AMSDU_MASK,             NO_SHIFT_DEFINE,	false},
	{"AMSDU_CROSS_LG(Read Only)",   WF_LWTBL_AMSDU_CROSS_LG_MASK,       NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw33(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 33 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 33\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_RX_STAT_CNT_LINE_1*4]);
	dw_value = *addr;

	while (WTBL_LMAC_DW33[i].name) {
		if (WTBL_LMAC_DW33[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW33[i].name,
				(dw_value & WTBL_LMAC_DW33[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW33[i].name,
				(dw_value & WTBL_LMAC_DW33[i].mask) >>
					WTBL_LMAC_DW33[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW34[] = {
	{"RESP_RCPI0",	WF_LWTBL_RESP_RCPI0_MASK,	WF_LWTBL_RESP_RCPI0_SHIFT,	false},
	{"RCPI1",	WF_LWTBL_RESP_RCPI1_MASK,	WF_LWTBL_RESP_RCPI1_SHIFT,	false},
	{"RCPI2",	WF_LWTBL_RESP_RCPI2_MASK,	WF_LWTBL_RESP_RCPI2_SHIFT,	false},
	{"RCPI3",	WF_LWTBL_RESP_RCPI3_MASK,	WF_LWTBL_RESP_RCPI3_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw34(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 34 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 34\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_RX_STAT_CNT_LINE_2*4]);
	dw_value = *addr;


	while (WTBL_LMAC_DW34[i].name) {
		if (WTBL_LMAC_DW34[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW34[i].name,
				(dw_value & WTBL_LMAC_DW34[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW34[i].name,
				(dw_value & WTBL_LMAC_DW34[i].mask) >>
					WTBL_LMAC_DW34[i].shift);
		i++;
	}
}

static const struct berse_wtbl_parse WTBL_LMAC_DW35[] = {
	{"SNR 0",	WF_LWTBL_SNR_RX0_MASK,		WF_LWTBL_SNR_RX0_SHIFT,	false},
	{"SNR 1",	WF_LWTBL_SNR_RX1_MASK,		WF_LWTBL_SNR_RX1_SHIFT,	false},
	{"SNR 2",	WF_LWTBL_SNR_RX2_MASK,		WF_LWTBL_SNR_RX2_SHIFT,	false},
	{"SNR 3",	WF_LWTBL_SNR_RX3_MASK,		WF_LWTBL_SNR_RX3_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_lwtbl_dw35(struct seq_file *s, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	/* LMAC WTBL DW 35 */
	seq_printf(s, "\t\n");
	seq_printf(s, "LWTBL DW 35\n");
	addr = (u32 *)&(lwtbl[WTBL_GROUP_RX_STAT_CNT_LINE_3*4]);
	dw_value = *addr;


	while (WTBL_LMAC_DW35[i].name) {
		if (WTBL_LMAC_DW35[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_LMAC_DW35[i].name,
				(dw_value & WTBL_LMAC_DW35[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_LMAC_DW35[i].name,
				(dw_value & WTBL_LMAC_DW35[i].mask) >>
					WTBL_LMAC_DW35[i].shift);
		i++;
	}
}

static void parse_fmac_lwtbl_rx_stats(struct seq_file *s, u8 *lwtbl)
{
	parse_fmac_lwtbl_dw33(s, lwtbl);
	parse_fmac_lwtbl_dw34(s, lwtbl);
	parse_fmac_lwtbl_dw35(s, lwtbl);
}

static void parse_fmac_lwtbl_mlo_info(struct seq_file *s, u8 *lwtbl)
{
	parse_fmac_lwtbl_dw28(s, lwtbl);
	parse_fmac_lwtbl_dw29(s, lwtbl);
	parse_fmac_lwtbl_dw30(s, lwtbl);
}

static const struct berse_wtbl_parse WTBL_UMAC_DW9[] = {
	{"RELATED_IDX0",	WF_UWTBL_RELATED_IDX0_MASK,		WF_UWTBL_RELATED_IDX0_SHIFT,	false},
	{"RELATED_BAND0",	WF_UWTBL_RELATED_BAND0_MASK,		WF_UWTBL_RELATED_BAND0_SHIFT,	false},
	{"PRI_MLD_BAND",    WF_UWTBL_PRIMARY_MLD_BAND_MASK,		WF_UWTBL_PRIMARY_MLD_BAND_SHIFT,	true},
	{"RELATED_IDX1",	WF_UWTBL_RELATED_IDX1_MASK,		WF_UWTBL_RELATED_IDX1_SHIFT,	false},
	{"RELATED_BAND1",   WF_UWTBL_RELATED_BAND1_MASK,		WF_UWTBL_RELATED_BAND1_SHIFT,	false},
	{"SEC_MLD_BAND",	WF_UWTBL_SECONDARY_MLD_BAND_MASK,	WF_UWTBL_SECONDARY_MLD_BAND_SHIFT,	true},
	{NULL,}
};

static void parse_fmac_uwtbl_mlo_info(struct seq_file *s, u8 *uwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	seq_printf(s, "\t\n");
	seq_printf(s, "MldAddr: %02x:%02x:%02x:%02x:%02x:%02x(D0[B0~15], D1[B0~31])\n",
		uwtbl[4], uwtbl[5], uwtbl[6], uwtbl[7], uwtbl[0], uwtbl[1]);

	/* UMAC WTBL DW 0 */
	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL DW 0\n");
	addr = (u32 *)&(uwtbl[WF_UWTBL_OWN_MLD_ID_DW*4]);
	dw_value = *addr;

	seq_printf(s, "\t%s:%u\n", "OMLD_ID",
		(dw_value & WF_UWTBL_OWN_MLD_ID_MASK) >> WF_UWTBL_OWN_MLD_ID_SHIFT);

	/* UMAC WTBL DW 9 */
	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL DW 9\n");
	addr = (u32 *)&(uwtbl[WF_UWTBL_RELATED_IDX0_DW*4]);
	dw_value = *addr;

	while (WTBL_UMAC_DW9[i].name) {

		if (WTBL_UMAC_DW9[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_UMAC_DW9[i].name,
				(dw_value & WTBL_UMAC_DW9[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW9[i].name,
				 (dw_value & WTBL_UMAC_DW9[i].mask) >>
					WTBL_UMAC_DW9[i].shift);
		i++;
	}
}

static bool
is_wtbl_bigtk_exist(u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;

	addr = (u32 *)&(lwtbl[WF_LWTBL_MUAR_DW*4]);
	dw_value = *addr;
	if (((dw_value & WF_LWTBL_MUAR_MASK) >> WF_LWTBL_MUAR_SHIFT) ==
					MUAR_INDEX_OWN_MAC_ADDR_BC_MC) {
		addr = (u32 *)&(lwtbl[WF_LWTBL_CIPHER_SUIT_BIGTK_DW*4]);
		dw_value = *addr;
		if (((dw_value & WF_LWTBL_CIPHER_SUIT_BIGTK_MASK) >>
			WF_LWTBL_CIPHER_SUIT_BIGTK_SHIFT) != IGTK_CIPHER_SUIT_NONE)
			return true;
	}

	return false;
}

static const struct berse_wtbl_parse WTBL_UMAC_DW2[] = {
	{"PN0",		WTBL_PN0_MASK,		WTBL_PN0_OFFSET,	false},
	{"PN1",		WTBL_PN1_MASK,		WTBL_PN1_OFFSET,	false},
	{"PN2",		WTBL_PN2_MASK,		WTBL_PN2_OFFSET,	true},
	{"PN3",		WTBL_PN3_MASK,		WTBL_PN3_OFFSET,	false},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_UMAC_DW3[] = {
	{"PN4",     WTBL_PN4_MASK,      WTBL_PN4_OFFSET,	false},
	{"PN5",     WTBL_PN5_MASK,      WTBL_PN5_OFFSET,	true},
	{"COM_SN",     WF_UWTBL_COM_SN_MASK,     WF_UWTBL_COM_SN_SHIFT,	true},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_UMAC_DW4_BIPN[] = {
	{"BIPN0",	WTBL_BIPN0_MASK,	WTBL_BIPN0_OFFSET,	false},
	{"BIPN1",	WTBL_BIPN1_MASK,	WTBL_BIPN1_OFFSET,	false},
	{"BIPN2",	WTBL_BIPN2_MASK,	WTBL_BIPN2_OFFSET,	true},
	{"BIPN3",	WTBL_BIPN3_MASK,	WTBL_BIPN3_OFFSET,	false},
	{NULL,}
};

static const struct berse_wtbl_parse WTBL_UMAC_DW5_BIPN[] = {
	{"BIPN4",	WTBL_BIPN4_MASK,	WTBL_BIPN4_OFFSET,	false},
	{"BIPN5",	WTBL_BIPN5_MASK,	WTBL_BIPN5_OFFSET,	true},
	{NULL,}
};

static void parse_fmac_uwtbl_pn(struct seq_file *s, u8 *uwtbl, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u16 i = 0;

	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL PN\n");

	/* UMAC WTBL DW 2/3 */
	addr = (u32 *)&(uwtbl[WF_UWTBL_PN_31_0__DW*4]);
	dw_value = *addr;

	while (WTBL_UMAC_DW2[i].name) {
		seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW2[i].name,
			(dw_value & WTBL_UMAC_DW2[i].mask) >>
				WTBL_UMAC_DW2[i].shift);
		i++;
	}

	i = 0;
	addr = (u32 *)&(uwtbl[WF_UWTBL_PN_47_32__DW*4]);
	dw_value = *addr;

	while (WTBL_UMAC_DW3[i].name) {
		seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW3[i].name,
			 (dw_value & WTBL_UMAC_DW3[i].mask) >>
			WTBL_UMAC_DW3[i].shift);
		i++;
	}


	/* UMAC WTBL DW 4/5 for BIGTK */
	if (is_wtbl_bigtk_exist(lwtbl) == true) {
		i = 0;
		addr = (u32 *)&(uwtbl[WF_UWTBL_RX_BIPN_31_0__DW*4]);
		dw_value = *addr;

		while (WTBL_UMAC_DW4_BIPN[i].name) {
			seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW4_BIPN[i].name,
				(dw_value & WTBL_UMAC_DW4_BIPN[i].mask) >>
					WTBL_UMAC_DW4_BIPN[i].shift);
			i++;
		}

		i = 0;
		addr = (u32 *)&(uwtbl[WF_UWTBL_RX_BIPN_47_32__DW*4]);
		dw_value = *addr;

		while (WTBL_UMAC_DW5_BIPN[i].name) {
			seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW5_BIPN[i].name,
				(dw_value & WTBL_UMAC_DW5_BIPN[i].mask) >>
				WTBL_UMAC_DW5_BIPN[i].shift);
			i++;
		}
	}
}

static void parse_fmac_uwtbl_sn(struct seq_file *s, u8 *uwtbl)
{
	u32 *addr = 0;
	u32 u2SN = 0;

	/* UMAC WTBL DW SN part */
	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL SN\n");

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID0_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID0_SN_MASK) >> WF_UWTBL_TID0_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID0_AC0_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID1_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID1_SN_MASK) >> WF_UWTBL_TID1_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID1_AC1_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID2_SN_7_0__DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID2_SN_7_0__MASK) >>
				WF_UWTBL_TID2_SN_7_0__SHIFT;
	addr = (u32 *)&(uwtbl[WF_UWTBL_TID2_SN_11_8__DW*4]);
	u2SN |= (((*addr) & WF_UWTBL_TID2_SN_11_8__MASK) >>
			WF_UWTBL_TID2_SN_11_8__SHIFT) << 8;
	seq_printf(s, "\t%s:%u\n", "TID2_AC2_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID3_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID3_SN_MASK) >> WF_UWTBL_TID3_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID3_AC3_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID4_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID4_SN_MASK) >> WF_UWTBL_TID4_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID4_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID5_SN_3_0__DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID5_SN_3_0__MASK) >>
				WF_UWTBL_TID5_SN_3_0__SHIFT;
	addr = (u32 *)&(uwtbl[WF_UWTBL_TID5_SN_11_4__DW*4]);
	u2SN |= (((*addr) & WF_UWTBL_TID5_SN_11_4__MASK) >>
				WF_UWTBL_TID5_SN_11_4__SHIFT) << 4;
	seq_printf(s, "\t%s:%u\n", "TID5_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID6_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID6_SN_MASK) >> WF_UWTBL_TID6_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID6_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_TID7_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_TID7_SN_MASK) >> WF_UWTBL_TID7_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "TID7_SN", u2SN);

	addr = (u32 *)&(uwtbl[WF_UWTBL_COM_SN_DW*4]);
	u2SN = ((*addr) & WF_UWTBL_COM_SN_MASK) >> WF_UWTBL_COM_SN_SHIFT;
	seq_printf(s, "\t%s:%u\n", "COM_SN", u2SN);
}

static void dump_key_table(
	struct seq_file *s,
	uint16_t keyloc0,
	uint16_t keyloc1,
	uint16_t keyloc2
)
{
#define ONE_KEY_ENTRY_LEN_IN_DW                8
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u8 keytbl[ONE_KEY_ENTRY_LEN_IN_DW*4] = {0};
	uint16_t x;

	seq_printf(s, "\t\n");
	seq_printf(s, "\t%s:%d\n", "keyloc0", keyloc0);
	if (keyloc0 != INVALID_KEY_ENTRY) {

		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		mt7996_wtbl_read_raw(dev, keyloc0,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		seq_printf(s, "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%lx\n",
			MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			mt76_rr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR),
			KEYTBL_IDX2BASE(keyloc0, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			seq_printf(s, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}

	seq_printf(s, "\t%s:%d\n", "keyloc1", keyloc1);
	if (keyloc1 != INVALID_KEY_ENTRY) {
		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		mt7996_wtbl_read_raw(dev, keyloc1,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		seq_printf(s, "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%lx\n",
			MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			mt76_rr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR),
			KEYTBL_IDX2BASE(keyloc1, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			seq_printf(s, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}

	seq_printf(s, "\t%s:%d\n", "keyloc2", keyloc2);
	if (keyloc2 != INVALID_KEY_ENTRY) {
		/* Don't swap below two lines, halWtblReadRaw will
		* write new value WF_WTBLON_TOP_WDUCR_ADDR
		*/
		mt7996_wtbl_read_raw(dev, keyloc2,
			WTBL_TYPE_KEY, 0, ONE_KEY_ENTRY_LEN_IN_DW, keytbl);
		seq_printf(s, "\t\tKEY WTBL Addr: group:0x%x=0x%x addr: 0x%lx\n",
			MT_DBG_UWTBL_TOP_WDUCR_ADDR,
			mt76_rr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR),
			KEYTBL_IDX2BASE(keyloc2, 0));
		for (x = 0; x < ONE_KEY_ENTRY_LEN_IN_DW; x++) {
			seq_printf(s, "\t\tDW%02d: %02x %02x %02x %02x\n",
				x,
				keytbl[x * 4 + 3],
				keytbl[x * 4 + 2],
				keytbl[x * 4 + 1],
				keytbl[x * 4]);
		}
	}
}

static void parse_fmac_uwtbl_key_info(struct seq_file *s, u8 *uwtbl, u8 *lwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	uint16_t keyloc0 = INVALID_KEY_ENTRY;
	uint16_t keyloc1 = INVALID_KEY_ENTRY;
	uint16_t keyloc2 = INVALID_KEY_ENTRY;

	/* UMAC WTBL DW 7 */
	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL key info\n");

	addr = (u32 *)&(uwtbl[WF_UWTBL_KEY_LOC0_DW*4]);
	dw_value = *addr;
	keyloc0 = (dw_value & WF_UWTBL_KEY_LOC0_MASK) >> WF_UWTBL_KEY_LOC0_SHIFT;
	keyloc1 = (dw_value & WF_UWTBL_KEY_LOC1_MASK) >> WF_UWTBL_KEY_LOC1_SHIFT;

	seq_printf(s, "\t%s:%u/%u\n", "Key Loc 0/1", keyloc0, keyloc1);

	/* UMAC WTBL DW 6 for BIGTK */
	if (is_wtbl_bigtk_exist(lwtbl) == true) {
		addr = (u32 *)&(uwtbl[WF_UWTBL_KEY_LOC2_DW*4]);
		dw_value = *addr;
		keyloc2 = (dw_value & WF_UWTBL_KEY_LOC2_MASK) >>
			WF_UWTBL_KEY_LOC2_SHIFT;
		seq_printf(s, "\t%s:%u\n", "Key Loc 2", keyloc2);
	}

	/* Parse KEY link */
	dump_key_table(s, keyloc0, keyloc1, keyloc2);
}

static const struct berse_wtbl_parse WTBL_UMAC_DW8[] = {
	{"UWTBL_WMM_Q",		WF_UWTBL_WMM_Q_MASK,		WF_UWTBL_WMM_Q_SHIFT,	false},
	{"UWTBL_QOS",		WF_UWTBL_QOS_MASK,		NO_SHIFT_DEFINE,	false},
	{"UWTBL_HT_VHT_HE",	WF_UWTBL_HT_MASK,		NO_SHIFT_DEFINE,	false},
	{"UWTBL_HDRT_MODE",	WF_UWTBL_HDRT_MODE_MASK,	NO_SHIFT_DEFINE,	true},
	{NULL,}
};

static void parse_fmac_uwtbl_msdu_info(struct seq_file *s, u8 *uwtbl)
{
	u32 *addr = 0;
	u32 dw_value = 0;
	u32 amsdu_len = 0;
	u16 i = 0;

	/* UMAC WTBL DW 8 */
	seq_printf(s, "\t\n");
	seq_printf(s, "UWTBL DW8\n");

	addr = (u32 *)&(uwtbl[WF_UWTBL_AMSDU_CFG_DW*4]);
	dw_value = *addr;

	while (WTBL_UMAC_DW8[i].name) {

		if (WTBL_UMAC_DW8[i].shift == NO_SHIFT_DEFINE)
			seq_printf(s, "\t%s:%d\n", WTBL_UMAC_DW8[i].name,
				(dw_value & WTBL_UMAC_DW8[i].mask) ? 1 : 0);
		else
			seq_printf(s, "\t%s:%u\n", WTBL_UMAC_DW8[i].name,
				(dw_value & WTBL_UMAC_DW8[i].mask) >>
					WTBL_UMAC_DW8[i].shift);
		i++;
	}

	/* UMAC WTBL DW 8 - SEC_ADDR_MODE */
	addr = (u32 *)&(uwtbl[WF_UWTBL_SEC_ADDR_MODE_DW*4]);
	dw_value = *addr;
	seq_printf(s, "\t%s:%lu\n", "SEC_ADDR_MODE",
		(dw_value & WTBL_SEC_ADDR_MODE_MASK) >> WTBL_SEC_ADDR_MODE_OFFSET);

	/* UMAC WTBL DW 8 - AMSDU_CFG */
	seq_printf(s, "\t%s:%d\n", "HW AMSDU Enable",
				(dw_value & WTBL_AMSDU_EN_MASK) ? 1 : 0);

	amsdu_len = (dw_value & WTBL_AMSDU_LEN_MASK) >> WTBL_AMSDU_LEN_OFFSET;
	if (amsdu_len == 0)
		seq_printf(s, "\t%s:invalid (WTBL value=0x%x)\n", "HW AMSDU Len",
			amsdu_len);
	else if (amsdu_len == 1)
		seq_printf(s, "\t%s:%d~%d (WTBL value=0x%x)\n", "HW AMSDU Len",
			1,
			255,
			amsdu_len);
	else if (amsdu_len == 2)
		seq_printf(s, "\t%s:%d~%d (WTBL value=0x%x)\n", "HW AMSDU Len",
			256,
			511,
			amsdu_len);
	else if (amsdu_len == 3)
		seq_printf(s, "\t%s:%d~%d (WTBL value=0x%x)\n", "HW AMSDU Len",
			512,
			767,
			amsdu_len);
	else
		seq_printf(s, "\t%s:%d~%d (WTBL value=0x%x)\n", "HW AMSDU Len",
			256 * (amsdu_len - 1),
			256 * (amsdu_len - 1) + 255,
			amsdu_len);

	seq_printf(s, "\t%s:%lu (WTBL value=0x%lx)\n", "HW AMSDU Num",
		((dw_value & WTBL_AMSDU_NUM_MASK) >> WTBL_AMSDU_NUM_OFFSET) + 1,
		(dw_value & WTBL_AMSDU_NUM_MASK) >> WTBL_AMSDU_NUM_OFFSET);
}

static int mt7996_wtbl_read(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u8 lwtbl[LWTBL_LEN_IN_DW * 4] = {0};
	u8 uwtbl[UWTBL_LEN_IN_DW * 4] = {0};
	int x;

	mt7996_wtbl_read_raw(dev, dev->wlan_idx, WTBL_TYPE_LMAC, 0,
				 LWTBL_LEN_IN_DW, lwtbl);
	seq_printf(s, "Dump WTBL info of WLAN_IDX:%d\n", dev->wlan_idx);
	seq_printf(s, "LMAC WTBL Addr: group:0x%x=0x%x addr: 0x%lx\n",
		   MT_DBG_WTBLON_TOP_WDUCR_ADDR,
		   mt76_rr(dev, MT_DBG_WTBLON_TOP_WDUCR_ADDR),
		   LWTBL_IDX2BASE(dev->wlan_idx, 0));
	for (x = 0; x < LWTBL_LEN_IN_DW; x++) {
		seq_printf(s, "DW%02d: %02x %02x %02x %02x\n",
			   x,
			   lwtbl[x * 4 + 3],
			   lwtbl[x * 4 + 2],
			   lwtbl[x * 4 + 1],
			   lwtbl[x * 4]);
	}

	/* Parse LWTBL */
	parse_fmac_lwtbl_dw0_1(s, lwtbl);
	parse_fmac_lwtbl_dw2(s, lwtbl);
	parse_fmac_lwtbl_dw3(s, lwtbl);
	parse_fmac_lwtbl_dw4(s, lwtbl);
	parse_fmac_lwtbl_dw5(s, lwtbl);
	parse_fmac_lwtbl_dw6(s, lwtbl);
	parse_fmac_lwtbl_dw7(s, lwtbl);
	parse_fmac_lwtbl_dw8(s, lwtbl);
	parse_fmac_lwtbl_dw9(s, lwtbl);
	parse_fmac_lwtbl_dw10(s, lwtbl);
	parse_fmac_lwtbl_dw11(s, lwtbl);
	parse_fmac_lwtbl_dw12(s, lwtbl);
	parse_fmac_lwtbl_dw13(s, lwtbl);
	parse_fmac_lwtbl_dw14(s, lwtbl);
	parse_fmac_lwtbl_mlo_info(s, lwtbl);
	parse_fmac_lwtbl_dw31(s, lwtbl);
	parse_fmac_lwtbl_dw32(s, lwtbl);
	parse_fmac_lwtbl_rx_stats(s, lwtbl);

	mt7996_wtbl_read_raw(dev, dev->wlan_idx, WTBL_TYPE_UMAC, 0,
				 UWTBL_LEN_IN_DW, uwtbl);
	seq_printf(s, "Dump WTBL info of WLAN_IDX:%d\n", dev->wlan_idx);
	seq_printf(s, "UMAC WTBL Addr: group:0x%x=0x%x addr: 0x%lx\n",
		   MT_DBG_UWTBL_TOP_WDUCR_ADDR,
		   mt76_rr(dev, MT_DBG_UWTBL_TOP_WDUCR_ADDR),
		   UWTBL_IDX2BASE(dev->wlan_idx, 0));
	for (x = 0; x < UWTBL_LEN_IN_DW; x++) {
		seq_printf(s, "DW%02d: %02x %02x %02x %02x\n",
			   x,
			   uwtbl[x * 4 + 3],
			   uwtbl[x * 4 + 2],
			   uwtbl[x * 4 + 1],
			   uwtbl[x * 4]);
	}

	/* Parse UWTBL */
	parse_fmac_uwtbl_mlo_info(s, uwtbl);
	parse_fmac_uwtbl_pn(s, uwtbl, lwtbl);
	parse_fmac_uwtbl_sn(s, uwtbl);
	parse_fmac_uwtbl_key_info(s, uwtbl, lwtbl);
	parse_fmac_uwtbl_msdu_info(s, uwtbl);

	return 0;
}

static int mt7996_sta_info(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u8 lwtbl[LWTBL_LEN_IN_DW*4] = {0};
	u16 i = 0;

	for (i=0; i < mt7996_wtbl_size(dev); i++) {
		mt7996_wtbl_read_raw(dev, i, WTBL_TYPE_LMAC, 0,
				     LWTBL_LEN_IN_DW, lwtbl);

		if (lwtbl[4] || lwtbl[5] || lwtbl[6] || lwtbl[7] || lwtbl[0] || lwtbl[1]) {
			u32 *addr, dw_value;

			seq_printf(s, "wcid:%d\tAddr: %02x:%02x:%02x:%02x:%02x:%02x",
					i, lwtbl[4], lwtbl[5], lwtbl[6], lwtbl[7], lwtbl[0], lwtbl[1]);

			addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_2*4]);
			dw_value = *addr;
			seq_printf(s, "\t%s:%u", WTBL_LMAC_DW2[0].name,
					(dw_value & WTBL_LMAC_DW2[0].mask) >> WTBL_LMAC_DW2[0].shift);

			addr = (u32 *)&(lwtbl[WTBL_GROUP_TRX_CAP_DW_5*4]);
			dw_value = *addr;
			seq_printf(s, "\tPSM:%u\n", !!(dw_value & WF_LWTBL_PSM_MASK));
		}
	}

	return 0;
}

static int mt7996_token_read(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	int msdu_id;
	struct mt76_txwi_cache *txwi;

	seq_printf(s, "Token from host:\n");
	spin_lock_bh(&dev->mt76.token_lock);
	idr_for_each_entry(&dev->mt76.token, txwi, msdu_id) {
		seq_printf(s, "%4d (pending time %u ms)\n", msdu_id,
			   jiffies_to_msecs(jiffies - txwi->jiffies));
	}
	spin_unlock_bh(&dev->mt76.token_lock);
	seq_printf(s, "\n");

	return 0;
}

static int
mt7996_scs_enable_set(void *data, u64 val)
{
	struct mt7996_phy *phy = data;

	return mt7996_mcu_set_scs(phy, (u8) val);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_scs_enable, NULL,
			 mt7996_scs_enable_set, "%lld\n");

static int
mt7996_txpower_level_set(void *data, u64 val)
{
	struct mt7996_phy *phy = data;
	int ret;

	if (val > 100)
		return -EINVAL;

	ret = mt7996_mcu_set_tx_power_ctrl(phy, UNI_TXPOWER_PERCENTAGE_CTRL, !!val);
	if (ret)
		return ret;

	return mt7996_mcu_set_tx_power_ctrl(phy, UNI_TXPOWER_PERCENTAGE_DROP_CTRL, val);
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_txpower_level, NULL,
			 mt7996_txpower_level_set, "%lld\n");

static ssize_t
mt7996_get_txpower_info(struct file *file, char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct mt7996_phy *phy = file->private_data;
	struct mt7996_mcu_txpower_event *event;
	struct txpower_basic_info *basic_info;
	static const size_t size = 2048;
	int len = 0;
	ssize_t ret;
	char *buf;

	buf = kzalloc(size, GFP_KERNEL);
	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!buf || !event) {
		ret = -ENOMEM;
		goto out;
	}

	ret = mt7996_mcu_get_tx_power_info(phy, BASIC_INFO, event);
	if (ret ||
	    le32_to_cpu(event->basic_info.category) != UNI_TXPOWER_BASIC_INFO)
		goto out;

	basic_info = &event->basic_info;

	len += scnprintf(buf + len, size - len,
			 "======================== BASIC INFO ========================\n");
	len += scnprintf(buf + len, size - len, "    Band Index: %d, Channel Band: %d\n",
			 basic_info->band_idx, basic_info->band);
	len += scnprintf(buf + len, size - len, "    PA Type: %s\n",
			 basic_info->is_epa ? "ePA" : "iPA");
	len += scnprintf(buf + len, size - len, "    LNA Type: %s\n",
			 basic_info->is_elna ? "eLNA" : "iLNA");

	len += scnprintf(buf + len, size - len,
			 "------------------------------------------------------------\n");
	len += scnprintf(buf + len, size - len, "    SKU: %s\n",
			 basic_info->sku_enable ? "enable" : "disable");
	len += scnprintf(buf + len, size - len, "    Percentage Control: %s\n",
			 basic_info->percentage_ctrl_enable ? "enable" : "disable");
	len += scnprintf(buf + len, size - len, "    Power Drop: %d [dBm]\n",
			 basic_info->power_drop_level >> 1);
	len += scnprintf(buf + len, size - len, "    Backoff: %s\n",
			 basic_info->bf_backoff_enable ? "enable" : "disable");
	len += scnprintf(buf + len, size - len, "    TX Front-end Loss:  %d, %d, %d, %d\n",
			 basic_info->front_end_loss_tx[0], basic_info->front_end_loss_tx[1],
			 basic_info->front_end_loss_tx[2], basic_info->front_end_loss_tx[3]);
	len += scnprintf(buf + len, size - len, "    RX Front-end Loss:  %d, %d, %d, %d\n",
			 basic_info->front_end_loss_rx[0], basic_info->front_end_loss_rx[1],
			 basic_info->front_end_loss_rx[2], basic_info->front_end_loss_rx[3]);
	len += scnprintf(buf + len, size - len,
			 "    MU TX Power Mode:  %s\n",
			 basic_info->mu_tx_power_manual_enable ? "manual" : "auto");
	len += scnprintf(buf + len, size - len,
			 "    MU TX Power (Auto / Manual): %d / %d [0.5 dBm]\n",
			 basic_info->mu_tx_power_auto, basic_info->mu_tx_power_manual);
	len += scnprintf(buf + len, size - len,
			 "    Thermal Compensation:  %s\n",
			 basic_info->thermal_compensate_enable ? "enable" : "disable");
	len += scnprintf(buf + len, size - len,
			 "    Theraml Compensation Value: %d\n",
			 basic_info->thermal_compensate_value);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

out:
	kfree(buf);
	kfree(event);
	return ret;
}

static const struct file_operations mt7996_txpower_info_fops = {
	.read = mt7996_get_txpower_info,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#define mt7996_txpower_puts(rate)							\
({											\
	len += scnprintf(buf + len, size - len, "%-21s:", #rate " (TMAC)");		\
	for (i = 0; i < mt7996_sku_group_len[SKU_##rate]; i++, offs++)			\
		len += scnprintf(buf + len, size - len, " %6d",				\
				 event->phy_rate_info.frame_power[offs][band_idx]);	\
	len += scnprintf(buf + len, size - len, "\n");					\
})

static ssize_t
mt7996_get_txpower_sku(struct file *file, char __user *user_buf,
		       size_t count, loff_t *ppos)
{
	struct mt7996_phy *phy = file->private_data;
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_mcu_txpower_event *event;
	u8 band_idx = phy->mt76->band_idx;
	static const size_t size = 5120;
	int i, offs = 0, len = 0;
	ssize_t ret;
	char *buf;
	u32 reg;

	buf = kzalloc(size, GFP_KERNEL);
	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!buf || !event) {
		ret = -ENOMEM;
		goto out;
	}

	ret = mt7996_mcu_get_tx_power_info(phy, PHY_RATE_INFO, event);
	if (ret ||
	    le32_to_cpu(event->phy_rate_info.category) != UNI_TXPOWER_PHY_RATE_INFO)
		goto out;

	len += scnprintf(buf + len, size - len,
			 "\nPhy %d TX Power Table (Channel %d)\n",
			 band_idx, phy->mt76->chandef.chan->hw_value);
	len += scnprintf(buf + len, size - len, "%-21s  %6s %6s %6s %6s\n",
			 " ", "1m", "2m", "5m", "11m");
	mt7996_txpower_puts(CCK);

	len += scnprintf(buf + len, size - len,
			 "%-21s  %6s %6s %6s %6s %6s %6s %6s %6s\n",
			 " ", "6m", "9m", "12m", "18m", "24m", "36m", "48m",
			 "54m");
	mt7996_txpower_puts(OFDM);

	len += scnprintf(buf + len, size - len,
			 "%-21s  %6s %6s %6s %6s %6s %6s %6s %6s\n",
			 " ", "mcs0", "mcs1", "mcs2", "mcs3", "mcs4",
			 "mcs5", "mcs6", "mcs7");
	mt7996_txpower_puts(HT20);

	len += scnprintf(buf + len, size - len,
			 "%-21s  %6s %6s %6s %6s %6s %6s %6s %6s %6s\n",
			 " ", "mcs0", "mcs1", "mcs2", "mcs3", "mcs4", "mcs5",
			 "mcs6", "mcs7", "mcs32");
	mt7996_txpower_puts(HT40);

	len += scnprintf(buf + len, size - len,
			 "%-21s  %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n",
			 " ", "mcs0", "mcs1", "mcs2", "mcs3", "mcs4", "mcs5",
			 "mcs6", "mcs7", "mcs8", "mcs9", "mcs10", "mcs11");
	mt7996_txpower_puts(VHT20);
	mt7996_txpower_puts(VHT40);
	mt7996_txpower_puts(VHT80);
	mt7996_txpower_puts(VHT160);
	mt7996_txpower_puts(HE26);
	mt7996_txpower_puts(HE52);
	mt7996_txpower_puts(HE106);
	mt7996_txpower_puts(HE242);
	mt7996_txpower_puts(HE484);
	mt7996_txpower_puts(HE996);
	mt7996_txpower_puts(HE2x996);

	len += scnprintf(buf + len, size - len,
			 "%-21s  %6s %6s %6s %6s %6s %6s %6s %6s ",
			 " ", "mcs0", "mcs1", "mcs2", "mcs3", "mcs4", "mcs5", "mcs6", "mcs7");
	len += scnprintf(buf + len, size - len,
			 "%6s %6s %6s %6s %6s %6s %6s %6s\n",
			 "mcs8", "mcs9", "mcs10", "mcs11", "mcs12", "mcs13", "mcs14", "mcs15");
	mt7996_txpower_puts(EHT26);
	mt7996_txpower_puts(EHT52);
	mt7996_txpower_puts(EHT106);
	mt7996_txpower_puts(EHT242);
	mt7996_txpower_puts(EHT484);
	mt7996_txpower_puts(EHT996);
	mt7996_txpower_puts(EHT2x996);
	mt7996_txpower_puts(EHT4x996);
	mt7996_txpower_puts(EHT26_52);
	mt7996_txpower_puts(EHT26_106);
	mt7996_txpower_puts(EHT484_242);
	mt7996_txpower_puts(EHT996_484);
	mt7996_txpower_puts(EHT996_484_242);
	mt7996_txpower_puts(EHT2x996_484);
	mt7996_txpower_puts(EHT3x996);
	mt7996_txpower_puts(EHT3x996_484);

	len += scnprintf(buf + len, size - len, "\nePA Gain: %d\n",
			 event->phy_rate_info.epa_gain);
	len += scnprintf(buf + len, size - len, "Max Power Bound: %d\n",
			 event->phy_rate_info.max_power_bound);
	len += scnprintf(buf + len, size - len, "Min Power Bound: %d\n",
			 event->phy_rate_info.min_power_bound);

	reg = MT_WF_PHYDFE_BAND_TPC_CTRL_STAT0(band_idx);
	len += scnprintf(buf + len, size - len,
			 "BBP TX Power (target power from TMAC)  : %6ld [0.5 dBm]\n",
			 mt76_get_field(dev, reg, MT_WF_PHY_TPC_POWER_TMAC));
	len += scnprintf(buf + len, size - len,
			 "BBP TX Power (target power from RMAC)  : %6ld [0.5 dBm]\n",
			 mt76_get_field(dev, reg, MT_WF_PHY_TPC_POWER_RMAC));
	len += scnprintf(buf + len, size - len,
			 "BBP TX Power (TSSI module power input)  : %6ld [0.5 dBm]\n",
			 mt76_get_field(dev, reg, MT_WF_PHY_TPC_POWER_TSSI));

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

out:
	kfree(buf);
	kfree(event);
	return ret;
}

static const struct file_operations mt7996_txpower_sku_fops = {
	.read = mt7996_get_txpower_sku,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#define mt7996_txpower_path_puts(rate, arr_length)					\
({											\
	len += scnprintf(buf + len, size - len, "%-23s:", #rate " (TMAC)");		\
	for (i = 0; i < arr_length; i++, offs++)					\
		len += scnprintf(buf + len, size - len, " %4d",				\
				 event->backoff_table_info.frame_power[offs]);		\
	len += scnprintf(buf + len, size - len, "\n");					\
})

static ssize_t
mt7996_get_txpower_path(struct file *file, char __user *user_buf,
		       size_t count, loff_t *ppos)
{
	struct mt7996_phy *phy = file->private_data;
	struct mt7996_mcu_txpower_event *event;
	static const size_t size = 5120;
	int i, offs = 0, len = 0;
	ssize_t ret;
	char *buf;

	buf = kzalloc(size, GFP_KERNEL);
	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (!buf || !event) {
		ret = -ENOMEM;
		goto out;
	}

	ret = mt7996_mcu_get_tx_power_info(phy, BACKOFF_TABLE_INFO, event);
	if (ret ||
	    le32_to_cpu(event->phy_rate_info.category) != UNI_TXPOWER_BACKOFF_TABLE_SHOW_INFO)
		goto out;

	len += scnprintf(buf + len, size - len, "\n%*c", 25, ' ');
	len += scnprintf(buf + len, size - len, "1T1S/2T1S/3T1S/4T1S/5T1S/2T2S/3T2S/4T2S/5T2S/"
			 "3T3S/4T3S/5T3S/4T4S/5T4S/5T5S\n");

	mt7996_txpower_path_puts(CCK, 5);
	mt7996_txpower_path_puts(OFDM, 5);
	mt7996_txpower_path_puts(BF-OFDM, 4);

	mt7996_txpower_path_puts(RU26, 15);
	mt7996_txpower_path_puts(BF-RU26, 15);
	mt7996_txpower_path_puts(RU52, 15);
	mt7996_txpower_path_puts(BF-RU52, 15);
	mt7996_txpower_path_puts(RU26_52, 15);
	mt7996_txpower_path_puts(BF-RU26_52, 15);
	mt7996_txpower_path_puts(RU106, 15);
	mt7996_txpower_path_puts(BF-RU106, 15);
	mt7996_txpower_path_puts(RU106_52, 15);
	mt7996_txpower_path_puts(BF-RU106_52, 15);

	mt7996_txpower_path_puts(BW20/RU242, 15);
	mt7996_txpower_path_puts(BF-BW20/RU242, 15);
	mt7996_txpower_path_puts(BW40/RU484, 15);
	mt7996_txpower_path_puts(BF-BW40/RU484, 15);
	mt7996_txpower_path_puts(RU242_484, 15);
	mt7996_txpower_path_puts(BF-RU242_484, 15);
	mt7996_txpower_path_puts(BW80/RU996, 15);
	mt7996_txpower_path_puts(BF-BW80/RU996, 15);
	mt7996_txpower_path_puts(RU484_996, 15);
	mt7996_txpower_path_puts(BF-RU484_996, 15);
	mt7996_txpower_path_puts(RU242_484_996, 15);
	mt7996_txpower_path_puts(BF-RU242_484_996, 15);
	mt7996_txpower_path_puts(BW160/RU996x2, 15);
	mt7996_txpower_path_puts(BF-BW160/RU996x2, 15);
	mt7996_txpower_path_puts(RU484_996x2, 15);
	mt7996_txpower_path_puts(BF-RU484_996x2, 15);
	mt7996_txpower_path_puts(RU996x3, 15);
	mt7996_txpower_path_puts(BF-RU996x3, 15);
	mt7996_txpower_path_puts(RU484_996x3, 15);
	mt7996_txpower_path_puts(BF-RU484_996x3, 15);
	mt7996_txpower_path_puts(BW320/RU996x4, 15);
	mt7996_txpower_path_puts(BF-BW320/RU996x4, 15);

	len += scnprintf(buf + len, size - len, "\nBackoff table: %s\n",
			 event->backoff_table_info.backoff_en ? "enable" : "disable");

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

out:
	kfree(buf);
	kfree(event);
	return ret;
}

static const struct file_operations mt7996_txpower_path_fops = {
	.read = mt7996_get_txpower_path,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int mt7996_show_eeprom_mode(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	struct mt76_dev *mdev = &dev->mt76;
#ifdef CONFIG_NL80211_TESTMODE
	const char *mtd_name = mdev->test_mtd.name;
	u32 mtd_offset = mdev->test_mtd.offset;
#else
	const char *mtd_name = NULL;
	u32 mtd_offset;
#endif

	seq_printf(s, "Current eeprom mode:\n");

	switch (dev->eeprom_mode) {
	case DEFAULT_BIN_MODE:
		seq_printf(s, "   default bin mode\n   filename = %s\n", mt7996_eeprom_name(dev));
		break;
	case EFUSE_MODE:
		seq_printf(s, "   efuse mode\n");
		break;
	case FLASH_MODE:
		if (mtd_name)
			seq_printf(s, "   flash mode\n   mtd name = %s\n   flash offset = 0x%x\n",
				   mtd_name, mtd_offset);
		else
			seq_printf(s, "   flash mode\n");
		break;
	case BIN_FILE_MODE:
		seq_printf(s, "   bin file mode\n   filename = %s\n", mt7996_eeprom_name(dev));
		break;
	default:
		break;
	}

	return 0;
}

static int
mt7996_sr_enable_get(void *data, u64 *val)
{
	struct mt7996_phy *phy = data;

	*val = phy->sr_enable;

	return 0;
}

static int
mt7996_sr_enable_set(void *data, u64 val)
{
	struct mt7996_phy *phy = data;
	int ret;

	if (!!val == phy->sr_enable)
		return 0;

	ret = mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_CFG_SR_ENABLE, val, true);
	if (ret)
		return ret;

	return mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_CFG_SR_ENABLE, 0, false);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_sr_enable, mt7996_sr_enable_get,
			 mt7996_sr_enable_set, "%lld\n");
static int
mt7996_sr_enhanced_enable_get(void *data, u64 *val)
{
	struct mt7996_phy *phy = data;

	*val = phy->enhanced_sr_enable;

	return 0;
}

static int
mt7996_sr_enhanced_enable_set(void *data, u64 val)
{
	struct mt7996_phy *phy = data;
	int ret;

	if (!!val == phy->enhanced_sr_enable)
		return 0;

	ret = mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_HW_ENHANCE_SR_ENABLE, val, true);
	if (ret)
		return ret;

	return mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_HW_ENHANCE_SR_ENABLE, 0, false);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_sr_enhanced_enable, mt7996_sr_enhanced_enable_get,
			 mt7996_sr_enhanced_enable_set, "%lld\n");

static int
mt7996_sr_stats_show(struct seq_file *file, void *data)
{
	struct mt7996_phy *phy = file->private;

	mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_HW_IND, 0, false);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mt7996_sr_stats);

static int
mt7996_sr_scene_cond_show(struct seq_file *file, void *data)
{
	struct mt7996_phy *phy = file->private;

	mt7996_mcu_set_sr_enable(phy, UNI_CMD_SR_SW_SD, 0, false);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mt7996_sr_scene_cond);

static int mt7996_rx_counters(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u32 rx_mac80211 = 0;
	int i = 0;

	for (i = 0; i < __MT_MAX_BAND; i++) {
		struct mt76_phy *phy = mt76_dev_phy(&dev->mt76, i);

		if (!phy)
			continue;

		seq_printf(s, "\n==========PHY%d==========\n", i);

#define SEQ_PRINT(_str, _rx_param) do {					\
		seq_printf(s, _str"\n", phy->rx_stats._rx_param);	\
	} while (0)

		SEQ_PRINT("Rx to mac80211: %u", rx_mac80211);
		SEQ_PRINT("Rx drop: %u", rx_drop);
		SEQ_PRINT("Rx drop due to RXD type error: %u", rx_rxd_drop);
		SEQ_PRINT("Rx duplicated drop: %u", rx_dup_drop);
		SEQ_PRINT("Rx agg miss: %u", rx_agg_miss);
		SEQ_PRINT("Rx ICV error: %u", rx_icv_error);
		SEQ_PRINT("Rx FCS error: %u", rx_fcs_error);
		SEQ_PRINT("Rx TKIP MIC error: %u", rx_tkip_mic_error);
		SEQ_PRINT("Rx PN/IV error: %u", rx_pn_iv_error);
#undef SEQ_PRINT

		rx_mac80211 += phy->rx_stats.rx_mac80211;
	}

	seq_printf(s, "\n==========SUM==========\n");
	seq_printf(s, "Rx to kernel: %u\n", dev->mt76.rx_kernel);
	seq_printf(s, "Rx to mac80211: %u\n", rx_mac80211);


	return 0;
}

static int
mt7996_starec_bf_read_set(void *data, u64 wlan_idx)
{
	struct mt7996_phy *phy = data;

	return mt7996_mcu_set_txbf_internal(phy, BF_STA_REC_READ, wlan_idx, 0);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_starec_bf_read, NULL,
			 mt7996_starec_bf_read_set, "%lld\n");

static ssize_t
mt7996_bf_txsnd_info_set(struct file *file,
			 const char __user *user_buf,
			 size_t count, loff_t *ppos)
{
	struct mt7996_phy *phy = file->private_data;
	char buf[40];
	int ret;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (count && buf[count - 1] == '\n')
		buf[count - 1] = '\0';
	else
		buf[count] = '\0';

	ret = mt7996_mcu_set_txbf_snd_info(phy, buf);

	if (ret) return -EFAULT;

	return count;
}

static const struct file_operations fops_bf_txsnd_info = {
	.write = mt7996_bf_txsnd_info_set,
	.read = NULL,
	.open = simple_open,
	.llseek = default_llseek,
};

static int
mt7996_bf_fbk_rpt_set(void *data, u64 wlan_idx)
{
	struct mt7996_phy *phy = data;

	return mt7996_mcu_set_txbf_internal(phy, BF_FBRPT_DBG_INFO_READ, wlan_idx, 0);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_bf_fbk_rpt, NULL,
			 mt7996_bf_fbk_rpt_set, "%lld\n");

static int
mt7996_bf_pfmu_tag_read_set(void *data, u64 wlan_idx)
{
	struct mt7996_phy *phy = data;

	return mt7996_mcu_set_txbf_internal(phy, BF_PFMU_TAG_READ, wlan_idx, 1);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_bf_pfmu_tag_read, NULL,
			 mt7996_bf_pfmu_tag_read_set, "%lld\n");

static int
mt7996_muru_fixed_rate_set(void *data, u64 val)
{
	struct mt7996_dev *dev = data;

	return mt7996_mcu_set_muru_fixed_rate_enable(dev, UNI_CMD_MURU_FIXED_RATE_CTRL,
						     val);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_muru_fixed_rate_enable, NULL,
			 mt7996_muru_fixed_rate_set, "%lld\n");

static ssize_t
mt7996_muru_fixed_rate_parameter_set(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct mt7996_dev *dev = file->private_data;
	char buf[40];
	int ret;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	if (count && buf[count - 1] == '\n')
		buf[count - 1] = '\0';
	else
		buf[count] = '\0';


	ret = mt7996_mcu_set_muru_fixed_rate_parameter(dev, UNI_CMD_MURU_FIXED_GROUP_RATE_CTRL,
						       buf);

	if (ret) return -EFAULT;

	return count;
}

static const struct file_operations fops_muru_fixed_group_rate = {
	.write = mt7996_muru_fixed_rate_parameter_set,
	.read = NULL,
	.open = simple_open,
	.llseek = default_llseek,
};

static int mt7996_muru_prot_thr_set(void *data, u64 val)
{
	struct mt7996_phy *phy = data;

	return mt7996_mcu_muru_set_prot_frame_thr(phy->dev, (u32)val);
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_muru_prot_thr, NULL,
			 mt7996_muru_prot_thr_set, "%lld\n");

static int
mt7996_red_config_set(void *data, u64 val)
{
	struct mt7996_dev *dev = data;

	return mt7996_mcu_red_config(dev, !!val);
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_red_config, NULL,
			 mt7996_red_config_set, "%lld\n");

static int
mt7996_vow_drr_dbg(void *data, u64 val)
{
	struct mt7996_dev *dev = data;

	return mt7996_mcu_set_vow_drr_dbg(dev, (u32)val);
}
DEFINE_DEBUGFS_ATTRIBUTE(fops_vow_drr_dbg, NULL,
			 mt7996_vow_drr_dbg, "%lld\n");

static int
mt7996_rro_session_read(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	struct mt7996_rro_ba_session *tbl;
	u32 value[2];

	mt76_wr(dev, MT_RRO_DBG_RD_CTRL, MT_RRO_DBG_RD_EXEC +
		(dev->dbg.sid >> 1) + 0x200);

	if (dev->dbg.sid & 0x1) {
		value[0] = mt76_rr(dev, MT_RRO_DBG_RDAT_DW(2));
		value[1] = mt76_rr(dev, MT_RRO_DBG_RDAT_DW(3));
	} else {
		value[0] = mt76_rr(dev, MT_RRO_DBG_RDAT_DW(0));
		value[1] = mt76_rr(dev, MT_RRO_DBG_RDAT_DW(1));
	}

	tbl = (struct mt7996_rro_ba_session *)&value[0];

	seq_printf(s, " seid %d:\nba session table DW0:%08x DW2:%08x\n",
		   dev->dbg.sid, value[0], value[1]);

	seq_printf(s, "ack_sn = 0x%x, last_in_sn = 0x%x, sat/bn/bc/bd/cn = %d/%d/%d/%d/%d\n",
		   tbl->ack_sn, tbl->last_in_sn, tbl->sat, tbl->bn, tbl->bc, tbl->bd, tbl->cn);

	seq_printf(s, "within_cnt = %d, to_sel = %d, last_in_rxtime = %d\n",
		   tbl->within_cnt, tbl->to_sel, tbl->last_in_rxtime);

	return 0;
}

static int
mt7996_show_rro_mib(struct seq_file *s, void *data)
{
	struct mt7996_dev *dev = dev_get_drvdata(s->private);
	u32 reg[12];

	seq_printf(s, "RRO mib Info:\n");

	reg[0] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(0));
	reg[1] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(1));
	reg[2] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(2));
	reg[3] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(3));
	reg[4] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(4));
	reg[5] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(5));
	reg[6] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(6));
	reg[7] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(7));
	reg[8] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(8));
	reg[9] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(9));
	reg[10] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(10));
	reg[11] = mt76_rr(dev, WF_RRO_TOP_STATISTIC(11));

	seq_printf(s, "STEP_ONE/WITHIN/SURPASS = %x/%x/%x\n", reg[0], reg[3], reg[4]);
	seq_printf(s, "REPEAT/OLDPKT/BAR = %x/%x/%x\n", reg[1], reg[2], reg[5]);
	seq_printf(s, "SURPASS with big gap = %x\n", reg[6]);
	seq_printf(s, "DISCONNECT/INVALID = %x/%x\n", reg[7], reg[8]);
	seq_printf(s, "TO(Step one)/TO(flush all) = %x/%x\n", reg[9], reg[10]);
	seq_printf(s, "buf ran out = %x\n", reg[11]);

	return 0;
}

int mt7996_mtk_init_debugfs(struct mt7996_phy *phy, struct dentry *dir)
{
	struct mt7996_dev *dev = phy->dev;
	u32 device_id = (dev->mt76.rev) >> 16;
	int i = 0;
	static const struct mt7996_dbg_reg_desc dbg_reg_s[] = {
		{ 0x7990, mt7996_dbg_offs },
		{ 0x7992, mt7992_dbg_offs },
	};

	for (i = 0; i < ARRAY_SIZE(dbg_reg_s); i++) {
		if (device_id == dbg_reg_s[i].id) {
			dev->dbg_reg = &dbg_reg_s[i];
			break;
		}
	}

	if (is_mt7996(&dev->mt76)) {
		WTBL_LMAC_DW2 = WTBL_LMAC_DW2_7996;
		WTBL_LMAC_DW5 = WTBL_LMAC_DW5_7996;
		WTBL_LMAC_DW9 = WTBL_LMAC_DW9_7996;
	} else {
		WTBL_LMAC_DW2 = WTBL_LMAC_DW2_7992;
		WTBL_LMAC_DW5 = WTBL_LMAC_DW5_7992;
		WTBL_LMAC_DW9 = WTBL_LMAC_DW9_7992;
	}

	/* agg */
	debugfs_create_devm_seqfile(dev->mt76.dev, "agg_info0", dir,
				    mt7996_agginfo_read_band0);
	debugfs_create_devm_seqfile(dev->mt76.dev, "agg_info1", dir,
				    mt7996_agginfo_read_band1);
	debugfs_create_devm_seqfile(dev->mt76.dev, "agg_info2", dir,
				    mt7996_agginfo_read_band2);
	/* amsdu */
	debugfs_create_devm_seqfile(dev->mt76.dev, "amsdu_info", dir,
				    mt7996_amsdu_result_read);

	debugfs_create_file("fw_debug_module", 0600, dir, dev,
			    &fops_fw_debug_module);
	debugfs_create_file("fw_debug_level", 0600, dir, dev,
			    &fops_fw_debug_level);
	debugfs_create_file("fw_wa_query", 0600, dir, dev, &fops_wa_query);
	debugfs_create_file("fw_wa_set", 0600, dir, dev, &fops_wa_set);
	debugfs_create_devm_seqfile(dev->mt76.dev, "fw_version", dir,
				    mt7996_dump_version);
	debugfs_create_devm_seqfile(dev->mt76.dev, "fw_wa_info", dir,
				    mt7996_fw_wa_info_read);
	debugfs_create_devm_seqfile(dev->mt76.dev, "fw_wm_info", dir,
				    mt7996_fw_wm_info_read);

	debugfs_create_devm_seqfile(dev->mt76.dev, "mib_info0", dir,
				    mt7996_mibinfo_band0);
	debugfs_create_devm_seqfile(dev->mt76.dev, "mib_info1", dir,
				    mt7996_mibinfo_band1);
	debugfs_create_devm_seqfile(dev->mt76.dev, "mib_info2", dir,
				    mt7996_mibinfo_band2);

	debugfs_create_devm_seqfile(dev->mt76.dev, "sta_info", dir,
				    mt7996_sta_info);

	debugfs_create_devm_seqfile(dev->mt76.dev, "tr_info", dir,
				    mt7996_trinfo_read);
	debugfs_create_devm_seqfile(dev->mt76.dev, "rx_counters", dir,
				    mt7996_rx_counters);
	debugfs_create_file("txpower_level", 0600, dir, phy, &fops_txpower_level);
	debugfs_create_file("txpower_info", 0600, dir, phy, &mt7996_txpower_info_fops);
	debugfs_create_file("txpower_sku", 0600, dir, phy, &mt7996_txpower_sku_fops);
	debugfs_create_file("txpower_path", 0600, dir, phy, &mt7996_txpower_path_fops);

	debugfs_create_devm_seqfile(dev->mt76.dev, "eeprom_mode", dir,
				    mt7996_show_eeprom_mode);

	debugfs_create_devm_seqfile(dev->mt76.dev, "wtbl_info", dir,
				    mt7996_wtbl_read);

	debugfs_create_devm_seqfile(dev->mt76.dev, "token", dir, mt7996_token_read);
	debugfs_create_file("red", 0200, dir, dev, &fops_red_config);
	debugfs_create_file("vow_drr_dbg", 0200, dir, dev, &fops_vow_drr_dbg);

	debugfs_create_u8("sku_disable", 0600, dir, &dev->dbg.sku_disable);
	debugfs_create_file("scs_enable", 0200, dir, phy, &fops_scs_enable);

	debugfs_create_file("sr_enable", 0600, dir, phy, &fops_sr_enable);
	debugfs_create_file("sr_enhanced_enable", 0600, dir, phy, &fops_sr_enhanced_enable);
	debugfs_create_file("sr_stats", 0400, dir, phy, &mt7996_sr_stats_fops);
	debugfs_create_file("sr_scene_cond", 0400, dir, phy, &mt7996_sr_scene_cond_fops);

	debugfs_create_file("muru_fixed_rate_enable", 0600, dir, dev,
			    &fops_muru_fixed_rate_enable);
	debugfs_create_file("muru_fixed_group_rate", 0600, dir, dev,
			    &fops_muru_fixed_group_rate);
	debugfs_create_file("bf_txsnd_info", 0600, dir, phy, &fops_bf_txsnd_info);
	debugfs_create_file("bf_starec_read", 0600, dir, phy, &fops_starec_bf_read);
	debugfs_create_file("bf_fbk_rpt", 0600, dir, phy, &fops_bf_fbk_rpt);
	debugfs_create_file("pfmu_tag_read", 0600, dir, phy, &fops_bf_pfmu_tag_read);

	debugfs_create_file("muru_prot_thr", 0200, dir, phy, &fops_muru_prot_thr);

	if (dev->has_rro) {
		debugfs_create_u32("rro_sid", 0600, dir, &dev->dbg.sid);
		debugfs_create_devm_seqfile(dev->mt76.dev, "rro_sid_info", dir,
					    mt7996_rro_session_read);
		debugfs_create_devm_seqfile(dev->mt76.dev, "rro_mib", dir,
					    mt7996_show_rro_mib);
	}

	return 0;
}

#endif
