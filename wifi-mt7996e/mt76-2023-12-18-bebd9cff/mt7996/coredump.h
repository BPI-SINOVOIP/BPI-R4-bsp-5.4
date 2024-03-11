/* SPDX-License-Identifier: ISC */
/* Copyright (C) 2023 MediaTek Inc. */

#ifndef _COREDUMP_H_
#define _COREDUMP_H_

#include "mt7996.h"

#define MT7996_COREDUMP_MAX	(MT7996_RAM_TYPE_WA + 1)

struct mt7996_coredump {
	char magic[16];

	u32 len;
	u32 hdr_len;

	guid_t guid;

	/* time-of-day stamp */
	u64 tv_sec;
	/* time-of-day stamp, nano-seconds */
	u64 tv_nsec;
	/* kernel version */
	char kernel[64];
	/* firmware version */
	char fw_ver[ETHTOOL_FWVERS_LEN];
	char fw_patch_date[MT7996_BUILD_TIME_LEN];
	char fw_ram_date[MT7996_COREDUMP_MAX][MT7996_BUILD_TIME_LEN];

	u32 device_id;

	/* fw type */
	char fw_type[8];

	/* exception state */
	char fw_state[12];

	/* program counters */
	u32 pc_dbg_ctrl;
	u32 pc_cur_idx;
	u32 pc_cur[10];
	/* PC registers */
	u32 pc_stack[32];

	u32 lr_dbg_ctrl;
	u32 lr_cur_idx;
	/* LR registers */
	u32 lr_stack[32];

	/* memory content */
	u8 data[];
} __packed;

struct mt7996_coredump_mem {
	u32 len;
	u8 data[];
} __packed;

struct mt7996_mem_hdr {
	char name[64];
	u32 start;
	u32 len;
	u8 data[];
};

struct mt7996_mem_region {
	u32 start;
	size_t len;

	const char *name;
};

#ifdef CONFIG_DEV_COREDUMP

const struct mt7996_mem_region *
mt7996_coredump_get_mem_layout(struct mt7996_dev *dev, u8 type, u32 *num);
struct mt7996_crash_data *mt7996_coredump_new(struct mt7996_dev *dev, u8 type);
struct mt7996_coredump *mt7996_coredump_build(struct mt7996_dev *dev, u8 type, bool full_dump);
int mt7996_coredump_submit(struct mt7996_dev *dev, u8 type);
int mt7996_coredump_register(struct mt7996_dev *dev);
void mt7996_coredump_unregister(struct mt7996_dev *dev);

#else /* CONFIG_DEV_COREDUMP */

static inline const struct mt7996_mem_region *
mt7996_coredump_get_mem_layout(struct mt7996_dev *dev, u8 type, u32 *num)
{
	return NULL;
}

static inline int mt7996_coredump_submit(struct mt7996_dev *dev, u8 type)
{
	return 0;
}

static inline struct
mt7996_coredump *mt7996_coredump_build(struct mt7996_dev *dev, u8 type, bool full_dump)
{
	return NULL;
}

static inline struct
mt7996_crash_data *mt7996_coredump_new(struct mt7996_dev *dev, u8 type)
{
	return NULL;
}

static inline int mt7996_coredump_register(struct mt7996_dev *dev)
{
	return 0;
}

static inline void mt7996_coredump_unregister(struct mt7996_dev *dev)
{
}

#endif /* CONFIG_DEV_COREDUMP */

#endif /* _COREDUMP_H_ */
