// SPDX-License-Identifier: ISC
/* Copyright (C) 2023 MediaTek Inc. */

#include <linux/devcoredump.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/utsname.h>
#include "coredump.h"

static bool coredump_memdump = true;
module_param(coredump_memdump, bool, 0644);
MODULE_PARM_DESC(coredump_memdump, "Optional ability to dump firmware memory");

static const struct mt7996_mem_region mt7996_wm_mem_regions[] = {
	{
		.start = 0x00800000,
		.len = 0x0004ffff,
		.name = "ULM0",
	},
	{
		.start = 0x00900000,
		.len = 0x00037fff,
		.name = "ULM1",
	},
	{
		.start = 0x02200000,
		.len = 0x0003ffff,
		.name = "ULM2",
	},
	{
		.start = 0x00400000,
		.len = 0x00067fff,
		.name = "SRAM",
	},
	{
		.start = 0xe0000000,
		.len = 0x0015ffff,
		.name = "CRAM0",
	},
	{
		.start = 0xe0160000,
		.len = 0x0011bfff,
		.name = "CRAM1",
	},
};

static const struct mt7996_mem_region mt7996_wa_mem_regions[] = {
	{
		.start = 0xE0000000,
		.len = 0x0000ffff,
		.name = "CRAM",
	},
	{
		.start = 0xE0010000,
		.len = 0x000117ff,
		.name = "CRAM2",
	},
	{
		.start = 0x10000000,
		.len = 0x0001bfff,
		.name = "ILM",
	},
	{
		.start = 0x10200000,
		.len = 0x00063fff,
		.name = "DLM",
	},
};

static const struct mt7996_mem_region mt7992_wm_mem_regions[] = {
	{
		.start = 0x00800000,
		.len = 0x0004bfff,
		.name = "ULM0",
	},
	{
		.start = 0x00900000,
		.len = 0x00035fff,
		.name = "ULM1",
	},
	{
		.start = 0x02200000,
		.len = 0x0003ffff,
		.name = "ULM2",
	},
	{
		.start = 0x00400000,
		.len = 0x00027fff,
		.name = "SRAM",
	},
	{
		.start = 0xe0000000,
		.len = 0x0015ffff,
		.name = "CRAM0",
	},
	{
		.start = 0xe0160000,
		.len = 0x00c7fff,
		.name = "CRAM1",
	},
	{
		.start = 0x7c050000,
		.len = 0x00007fff,
		.name = "CONN_INFRA",
	},
};

const struct mt7996_mem_region*
mt7996_coredump_get_mem_layout(struct mt7996_dev *dev, u8 type, u32 *num)
{
	switch (mt76_chip(&dev->mt76)) {
	case 0x7990:
	case 0x7991:
		if (type == MT7996_RAM_TYPE_WA) {
			*num = ARRAY_SIZE(mt7996_wa_mem_regions);
			return &mt7996_wa_mem_regions[0];
		}

		*num = ARRAY_SIZE(mt7996_wm_mem_regions);
		return &mt7996_wm_mem_regions[0];
	case 0x7992:
		if (type == MT7996_RAM_TYPE_WA) {
			/* mt7992 wa memory regions is the same as mt7996 */
			*num = ARRAY_SIZE(mt7996_wa_mem_regions);
			return &mt7996_wa_mem_regions[0];
		}
		*num = ARRAY_SIZE(mt7992_wm_mem_regions);
		return &mt7992_wm_mem_regions[0];
	default:
		return NULL;
	}
}

static int mt7996_coredump_get_mem_size(struct mt7996_dev *dev, u8 type)
{
	const struct mt7996_mem_region *mem_region;
	size_t size = 0;
	u32 num;
	int i;

	mem_region = mt7996_coredump_get_mem_layout(dev, type, &num);
	if (!mem_region)
		return 0;

	for (i = 0; i < num; i++) {
		size += mem_region->len;
		mem_region++;
	}

	/* reserve space for the headers */
	size += num * sizeof(struct mt7996_mem_hdr);
	/* make sure it is aligned 4 bytes for debug message print out */
	size = ALIGN(size, 4);

	return size;
}

struct mt7996_crash_data *mt7996_coredump_new(struct mt7996_dev *dev, u8 type)
{
	struct mt7996_crash_data *crash_data = dev->coredump.crash_data[type];

	lockdep_assert_held(&dev->dump_mutex);

	if (!coredump_memdump || !crash_data->supported)
		return NULL;

	guid_gen(&crash_data->guid);
	ktime_get_real_ts64(&crash_data->timestamp);

	return crash_data;
}

static void
mt7996_coredump_fw_state(struct mt7996_dev *dev, u8 type, struct mt7996_coredump *dump,
			 bool *exception)
{
	u32 count, reg = MT_FW_WM_DUMP_STATE;

	if (type == MT7996_RAM_TYPE_WA)
		reg = MT_FW_WA_DUMP_STATE;

	count = mt76_rr(dev, reg);

	/* normal mode: driver can manually trigger assert for detail info */
	if (!count)
		strscpy(dump->fw_state, "normal", sizeof(dump->fw_state));
	else
		strscpy(dump->fw_state, "exception", sizeof(dump->fw_state));

	*exception = !!count;
}

static void
mt7996_coredump_fw_stack(struct mt7996_dev *dev, u8 type, struct mt7996_coredump *dump,
			 bool exception)
{
	u32 reg, i, offset = 0, val = MT7996_RAM_TYPE_WM;

	if (type == MT7996_RAM_TYPE_WA) {
		offset = MT_MCU_WA_EXCP_BASE - MT_MCU_WM_EXCP_BASE;
		val = MT7996_RAM_TYPE_WA;
	}

	/* 0: WM PC log output, 1: WA PC log output  */
	mt76_wr(dev, MT_CONN_DBG_CTL_OUT_SEL, val);
	/* choose 33th PC log buffer to read current PC index */
	mt76_wr(dev, MT_CONN_DBG_CTL_PC_LOG_SEL, 0x3f);

	/* read current PC */
	for (i = 0; i < 10; i++)
		dump->pc_cur[i] = mt76_rr(dev, MT_CONN_DBG_CTL_PC_LOG);

	/* stop call stack record */
	if (!exception) {
		mt76_clear(dev, MT_MCU_WM_EXCP_PC_CTRL + offset, BIT(0));
		mt76_clear(dev, MT_MCU_WM_EXCP_LR_CTRL + offset, BIT(0));
	}

	/* read PC log */
	dump->pc_dbg_ctrl = mt76_rr(dev, MT_MCU_WM_EXCP_PC_CTRL + offset);
	dump->pc_cur_idx = FIELD_GET(MT_MCU_WM_EXCP_PC_CTRL_IDX_STATUS,
				     dump->pc_dbg_ctrl);
	for (i = 0; i < 32; i++) {
		reg = MT_MCU_WM_EXCP_PC_LOG + i * 4 + offset;
		dump->pc_stack[i] = mt76_rr(dev, reg);
	}

	/* read LR log */
	dump->lr_dbg_ctrl = mt76_rr(dev, MT_MCU_WM_EXCP_LR_CTRL + offset);
	dump->lr_cur_idx = FIELD_GET(MT_MCU_WM_EXCP_LR_CTRL_IDX_STATUS,
				     dump->lr_dbg_ctrl);
	for (i = 0; i < 32; i++) {
		reg = MT_MCU_WM_EXCP_LR_LOG + i * 4 + offset;
		dump->lr_stack[i] = mt76_rr(dev, reg);
	}

	/* start call stack record */
	if (!exception) {
		mt76_set(dev, MT_MCU_WM_EXCP_PC_CTRL + offset, BIT(0));
		mt76_set(dev, MT_MCU_WM_EXCP_LR_CTRL + offset, BIT(0));
	}
}

struct mt7996_coredump *mt7996_coredump_build(struct mt7996_dev *dev, u8 type, bool full_dump)
{
	struct mt7996_crash_data *crash_data = dev->coredump.crash_data[type];
	struct mt7996_coredump *dump;
	struct mt7996_coredump_mem *dump_mem;
	size_t len, sofar = 0, hdr_len = sizeof(*dump);
	unsigned char *buf;
	bool exception;

	len = hdr_len;

	if (full_dump && coredump_memdump && crash_data->memdump_buf_len)
		len += sizeof(*dump_mem) + crash_data->memdump_buf_len;

	sofar += hdr_len;

	/* this is going to get big when we start dumping memory and such,
	 * so go ahead and use vmalloc.
	 */
	buf = vzalloc(len);
	if (!buf)
		return NULL;

	mutex_lock(&dev->dump_mutex);

	dump = (struct mt7996_coredump *)(buf);
	dump->len = len;
	dump->hdr_len = hdr_len;

	/* plain text */
	strscpy(dump->magic, "mt76-crash-dump", sizeof(dump->magic));
	strscpy(dump->kernel, init_utsname()->release, sizeof(dump->kernel));
	strscpy(dump->fw_type, ((type == MT7996_RAM_TYPE_WA) ? "WA" : "WM"),
		sizeof(dump->fw_type));
	strscpy(dump->fw_ver, dev->mt76.hw->wiphy->fw_version,
		sizeof(dump->fw_ver));
	strscpy(dump->fw_patch_date, dev->patch_build_date,
		sizeof(dump->fw_patch_date));
	strscpy(dump->fw_ram_date[MT7996_RAM_TYPE_WM],
		dev->ram_build_date[MT7996_RAM_TYPE_WM],
		MT7996_BUILD_TIME_LEN);
	strscpy(dump->fw_ram_date[MT7996_RAM_TYPE_WA],
		dev->ram_build_date[MT7996_RAM_TYPE_WA],
		MT7996_BUILD_TIME_LEN);

	guid_copy(&dump->guid, &crash_data->guid);
	dump->tv_sec = crash_data->timestamp.tv_sec;
	dump->tv_nsec = crash_data->timestamp.tv_nsec;
	dump->device_id = mt76_chip(&dev->mt76);

	mt7996_coredump_fw_state(dev, type, dump, &exception);
	mt7996_coredump_fw_stack(dev, type, dump, exception);

	if (!full_dump)
		goto skip_dump_mem;

	/* gather memory content */
	dump_mem = (struct mt7996_coredump_mem *)(buf + sofar);
	dump_mem->len = crash_data->memdump_buf_len;
	if (coredump_memdump && crash_data->memdump_buf_len)
		memcpy(dump_mem->data, crash_data->memdump_buf,
		       crash_data->memdump_buf_len);

skip_dump_mem:
	mutex_unlock(&dev->dump_mutex);

	return dump;
}

int mt7996_coredump_submit(struct mt7996_dev *dev, u8 type)
{
	struct mt7996_coredump *dump;

	dump = mt7996_coredump_build(dev, type, true);
	if (!dump) {
		dev_warn(dev->mt76.dev, "no crash dump data found\n");
		return -ENODATA;
	}

	dev_coredumpv(dev->mt76.dev, dump, dump->len, GFP_KERNEL);
	dev_info(dev->mt76.dev, "%s coredump completed\n",
		 wiphy_name(dev->mt76.hw->wiphy));

	return 0;
}

int mt7996_coredump_register(struct mt7996_dev *dev)
{
	struct mt7996_crash_data *crash_data;
	int i;

	for (i = 0; i < MT7996_COREDUMP_MAX; i++) {
		crash_data = vzalloc(sizeof(*dev->coredump.crash_data[i]));
		if (!crash_data)
			goto nomem;

		dev->coredump.crash_data[i] = crash_data;
		crash_data->supported = false;

		if (coredump_memdump) {
			crash_data->memdump_buf_len = mt7996_coredump_get_mem_size(dev, i);
			if (!crash_data->memdump_buf_len)
				/* no memory content */
				continue;

			crash_data->memdump_buf = vzalloc(crash_data->memdump_buf_len);
			if (!crash_data->memdump_buf)
				goto nomem;

			crash_data->supported = true;
		}
	}

	return 0;
nomem:
	mt7996_coredump_unregister(dev);
	return -ENOMEM;
}

void mt7996_coredump_unregister(struct mt7996_dev *dev)
{
	int i;
	struct mt7996_crash_data *crash_data;

	for (i = 0; i < MT7996_COREDUMP_MAX; i++) {
		crash_data = dev->coredump.crash_data[i];

		if (!crash_data)
			continue;

		if (crash_data->memdump_buf)
			vfree(crash_data->memdump_buf);

		vfree(crash_data);
	}
}

