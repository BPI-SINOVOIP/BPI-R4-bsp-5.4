/* SPDX-License-Identifier: ISC */
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#ifndef __MT7996_EEPROM_H
#define __MT7996_EEPROM_H

#include "mt7996.h"

enum mt7996_eeprom_field {
	MT_EE_CHIP_ID =		0x000,
	MT_EE_VERSION =		0x002,
	MT_EE_MAC_ADDR =	0x004,
	MT_EE_MAC_ADDR2 =	0x00a,
	MT_EE_WIFI_CONF =	0x190,
	MT_EE_DO_PRE_CAL =	0x1a5,
	MT_EE_TESTMODE_EN =	0x1af,
	MT_EE_MAC_ADDR3 =	0x2c0,
	MT_EE_RATE_DELTA_2G =	0x1400,
	MT_EE_RATE_DELTA_5G =	0x147d,
	MT_EE_RATE_DELTA_6G =	0x154a,
	MT_EE_TX0_POWER_2G =	0x1300,
	MT_EE_TX0_POWER_5G =	0x1301,
	MT_EE_TX0_POWER_6G =	0x1310,

	__MT_EE_MAX =	0x1dff,
	/* 0x1e10 ~ 0x2d644 used to save group cal data */
	MT_EE_PRECAL =		0x1e10,
};

#define MT_EE_WIFI_CONF0_TX_PATH		GENMASK(2, 0)
#define MT_EE_WIFI_CONF0_BAND_SEL		GENMASK(2, 0)
#define MT_EE_WIFI_CONF1_BAND_SEL		GENMASK(5, 3)
#define MT_EE_WIFI_CONF2_BAND_SEL		GENMASK(2, 0)
#define MT_EE_WIFI_PA_LNA_CONFIG		GENMASK(1, 0)

#define MT_EE_WIFI_CAL_GROUP_2G			BIT(0)
#define MT_EE_WIFI_CAL_GROUP_5G			BIT(1)
#define MT_EE_WIFI_CAL_GROUP_6G			BIT(2)
#define MT_EE_WIFI_CAL_GROUP			GENMASK(2, 0)
#define MT_EE_WIFI_CAL_DPD_2G			BIT(3)
#define MT_EE_WIFI_CAL_DPD_5G			BIT(4)
#define MT_EE_WIFI_CAL_DPD_6G			BIT(5)
#define MT_EE_WIFI_CAL_DPD			GENMASK(5, 3)

#define MT_EE_CAL_UNIT				1024

enum mt7996_prek_rev {
	GROUP_SIZE_2G,
	GROUP_SIZE_5G,
	GROUP_SIZE_6G,
	ADCDCOC_SIZE_2G,
	ADCDCOC_SIZE_5G,
	ADCDCOC_SIZE_6G,
	DPD_LEGACY_SIZE,
	DPD_MEM_SIZE,
	DPD_OTFG0_SIZE,
};

static const u32 mt7996_prek_rev[] = {
	[GROUP_SIZE_2G] =			4 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_5G] =			45 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_6G] =			125 * MT_EE_CAL_UNIT,
	[ADCDCOC_SIZE_2G] =			4 * 4,
	[ADCDCOC_SIZE_5G] =			4 * 4,
	[ADCDCOC_SIZE_6G] =			4 * 5,
	[DPD_LEGACY_SIZE] =			4 * MT_EE_CAL_UNIT,
	[DPD_MEM_SIZE] =			13 * MT_EE_CAL_UNIT,
	[DPD_OTFG0_SIZE] =			2 * MT_EE_CAL_UNIT,
};

static const u32 mt7996_prek_rev_233[] = {
	[GROUP_SIZE_2G] =			4 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_5G] =			44 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_6G] =			100 * MT_EE_CAL_UNIT,
	[ADCDCOC_SIZE_2G] =			4 * 4,
	[ADCDCOC_SIZE_5G] =			4 * 4,
	[ADCDCOC_SIZE_6G] =			4 * 5,
	[DPD_LEGACY_SIZE] =			4 * MT_EE_CAL_UNIT,
	[DPD_MEM_SIZE] =			13 * MT_EE_CAL_UNIT,
	[DPD_OTFG0_SIZE] =			2 * MT_EE_CAL_UNIT,
};

/* kite 2/5g config */
static const u32 mt7992_prek_rev[] = {
	[GROUP_SIZE_2G] =			4 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_5G] =			110 * MT_EE_CAL_UNIT,
	[GROUP_SIZE_6G] =			0,
	[ADCDCOC_SIZE_2G] =			4 * 4,
	[ADCDCOC_SIZE_5G] =			4 * 5,
	[ADCDCOC_SIZE_6G] =			0,
	[DPD_LEGACY_SIZE] =			5 * MT_EE_CAL_UNIT,
	[DPD_MEM_SIZE] =			16 * MT_EE_CAL_UNIT,
	[DPD_OTFG0_SIZE] =			2 * MT_EE_CAL_UNIT,
};

extern const struct ieee80211_channel dpd_2g_ch_list_bw20[];
extern const struct ieee80211_channel dpd_5g_skip_ch_list[];
extern const struct ieee80211_channel dpd_5g_ch_list_bw80[];
extern const struct ieee80211_channel dpd_5g_ch_list_bw160[];
extern const struct ieee80211_channel dpd_6g_ch_list_bw80[];
extern const struct ieee80211_channel dpd_6g_ch_list_bw160[];
extern const struct ieee80211_channel dpd_6g_ch_list_bw320[];

#define PREK(id)				(dev->prek.rev[(id)])
#define DPD_CH_NUM(_type)			(dev->prek.dpd_ch_num[DPD_CH_NUM_##_type])
#define MT_EE_CAL_GROUP_SIZE			(PREK(GROUP_SIZE_2G) + PREK(GROUP_SIZE_5G) + \
						 PREK(GROUP_SIZE_6G) + PREK(ADCDCOC_SIZE_2G) + \
						 PREK(ADCDCOC_SIZE_5G) + PREK(ADCDCOC_SIZE_6G))
#define DPD_PER_CH_BW20_SIZE			(PREK(DPD_LEGACY_SIZE) + PREK(DPD_OTFG0_SIZE))
#define DPD_PER_CH_GT_BW20_SIZE			(PREK(DPD_MEM_SIZE) + PREK(DPD_OTFG0_SIZE))
#define MT_EE_CAL_DPD_SIZE_2G			(DPD_CH_NUM(BW20_2G) * DPD_PER_CH_BW20_SIZE)
#define MT_EE_CAL_DPD_SIZE_5G			(DPD_CH_NUM(BW20_5G) * DPD_PER_CH_BW20_SIZE + \
						 DPD_CH_NUM(BW80_5G) * DPD_PER_CH_GT_BW20_SIZE + \
						 DPD_CH_NUM(BW160_5G) * DPD_PER_CH_GT_BW20_SIZE)
#define MT_EE_CAL_DPD_SIZE_6G			(DPD_CH_NUM(BW20_6G) * DPD_PER_CH_BW20_SIZE + \
						 DPD_CH_NUM(BW80_6G) * DPD_PER_CH_GT_BW20_SIZE + \
						 DPD_CH_NUM(BW160_6G) * DPD_PER_CH_GT_BW20_SIZE + \
						 DPD_CH_NUM(BW320_6G) * DPD_PER_CH_GT_BW20_SIZE)
#define MT_EE_CAL_DPD_SIZE			(MT_EE_CAL_DPD_SIZE_2G + MT_EE_CAL_DPD_SIZE_5G + \
						 MT_EE_CAL_DPD_SIZE_6G)

#define RF_DPD_FLAT_CAL				BIT(28)
#define RF_PRE_CAL				BIT(29)
#define RF_DPD_FLAT_5G_CAL			GENMASK(29, 28)
#define RF_DPD_FLAT_5G_MEM_CAL			(BIT(30) | BIT(28))
#define RF_DPD_FLAT_6G_CAL			GENMASK(30, 28)
#define RF_DPD_FLAT_6G_MEM_CAL			(BIT(31) | BIT(28))

#define MT_EE_WIFI_CONF1_TX_PATH_BAND0		GENMASK(5, 3)
#define MT_EE_WIFI_CONF2_TX_PATH_BAND1		GENMASK(2, 0)
#define MT_EE_WIFI_CONF2_TX_PATH_BAND2		GENMASK(5, 3)
#define MT_EE_WIFI_CONF3_RX_PATH_BAND0		GENMASK(2, 0)
#define MT_EE_WIFI_CONF3_RX_PATH_BAND1		GENMASK(5, 3)
#define MT_EE_WIFI_CONF4_RX_PATH_BAND2		GENMASK(2, 0)
#define MT_EE_WIFI_CONF4_STREAM_NUM_BAND0	GENMASK(5, 3)
#define MT_EE_WIFI_CONF5_STREAM_NUM_BAND1	GENMASK(2, 0)
#define MT_EE_WIFI_CONF5_STREAM_NUM_BAND2	GENMASK(5, 3)

#define MT_EE_RATE_DELTA_MASK			GENMASK(5, 0)
#define MT_EE_RATE_DELTA_SIGN			BIT(6)
#define MT_EE_RATE_DELTA_EN			BIT(7)

enum mt7996_eeprom_band {
	MT_EE_BAND_SEL_DEFAULT,
	MT_EE_BAND_SEL_2GHZ,
	MT_EE_BAND_SEL_5GHZ,
	MT_EE_BAND_SEL_6GHZ,
};

enum mt7915_eeprom_mode {
	DEFAULT_BIN_MODE,
	EFUSE_MODE,
	FLASH_MODE,
	BIN_FILE_MODE,
};

static inline int
mt7996_get_channel_group_5g(int channel)
{
	if (channel <= 64)
		return 0;
	if (channel <= 96)
		return 1;
	if (channel <= 128)
		return 2;
	if (channel <= 144)
		return 3;
	return 4;
}

static inline int
mt7996_get_channel_group_6g(int channel)
{
	if (channel <= 29)
		return 0;

	return DIV_ROUND_UP(channel - 29, 32);
}

enum mt7996_sku_rate_group {
	SKU_CCK,
	SKU_OFDM,

	SKU_HT20,
	SKU_HT40,

	SKU_VHT20,
	SKU_VHT40,
	SKU_VHT80,
	SKU_VHT160,

	SKU_HE26,
	SKU_HE52,
	SKU_HE106,
	SKU_HE242,
	SKU_HE484,
	SKU_HE996,
	SKU_HE2x996,

	SKU_EHT26,
	SKU_EHT52,
	SKU_EHT106,
	SKU_EHT242,
	SKU_EHT484,
	SKU_EHT996,
	SKU_EHT2x996,
	SKU_EHT4x996,
	SKU_EHT26_52,
	SKU_EHT26_106,
	SKU_EHT484_242,
	SKU_EHT996_484,
	SKU_EHT996_484_242,
	SKU_EHT2x996_484,
	SKU_EHT3x996,
	SKU_EHT3x996_484,

	MAX_SKU_RATE_GROUP_NUM,
};

extern const u8 mt7996_sku_group_len[MAX_SKU_RATE_GROUP_NUM];

#endif
