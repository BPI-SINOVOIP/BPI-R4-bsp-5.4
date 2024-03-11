// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#include <linux/firmware.h>
#include "mt7996.h"
#include "eeprom.h"
#include <linux/moduleparam.h>

static bool testmode_enable;
module_param(testmode_enable, bool, 0644);
MODULE_PARM_DESC(testmode_enable, "Enable testmode");

const struct ieee80211_channel dpd_2g_ch_list_bw20[] = {
	CHAN2G(3, 2422),
	CHAN2G(7, 2442),
	CHAN2G(11, 2462)
};

const struct ieee80211_channel dpd_5g_skip_ch_list[] = {
	CHAN5G(68, 5340),
	CHAN5G(72, 5360),
	CHAN5G(76, 5380),
	CHAN5G(80, 5400),
	CHAN5G(84, 5420),
	CHAN5G(88, 5440),
	CHAN5G(92, 5460),
	CHAN5G(96, 5480)
};

const struct ieee80211_channel dpd_5g_ch_list_bw80[] = {
	CHAN5G(42, 5210),
	CHAN5G(58, 5290),
	CHAN5G(106, 5530),
	CHAN5G(122, 5610),
	CHAN5G(138, 5690),
	CHAN5G(155, 5775),
	CHAN5G(171, 5855)
};

const struct ieee80211_channel dpd_5g_ch_list_bw160[] = {
	CHAN5G(50, 5250),
	CHAN5G(114, 5570),
	CHAN5G(163, 5815)
};

const struct ieee80211_channel dpd_6g_ch_list_bw80[] = {
	CHAN6G(7, 5985),
	CHAN6G(23, 6065),
	CHAN6G(39, 6145),
	CHAN6G(55, 6225),
	CHAN6G(71, 6305),
	CHAN6G(87, 6385),
	CHAN6G(103, 6465),
	CHAN6G(119, 6545),
	CHAN6G(135, 6625),
	CHAN6G(151, 6705),
	CHAN6G(167, 6785),
	CHAN6G(183, 6865),
	CHAN6G(199, 6945),
	CHAN6G(215, 7025)
};

const struct ieee80211_channel dpd_6g_ch_list_bw160[] = {
	CHAN6G(15, 6025),
	CHAN6G(47, 6185),
	CHAN6G(79, 6345),
	CHAN6G(111, 6505),
	CHAN6G(143, 6665),
	CHAN6G(175, 6825),
	CHAN6G(207, 6985)
};

const struct ieee80211_channel dpd_6g_ch_list_bw320[] = {
	CHAN6G(31, 6105),
	CHAN6G(63, 6265),
	CHAN6G(95, 6425),
	CHAN6G(127, 6585),
	CHAN6G(159, 6745),
	CHAN6G(191, 6905)
};

static int mt7996_check_eeprom(struct mt7996_dev *dev)
{
#define FEM_INT				0
#define FEM_EXT				3
	u8 *eeprom = dev->mt76.eeprom.data;
	u8 i, fem[__MT_MAX_BAND], fem_type;
	u16 val = get_unaligned_le16(eeprom);

	for (i = 0; i < __MT_MAX_BAND; i++)
		fem[i] = eeprom[MT_EE_WIFI_CONF + 6 + i] & MT_EE_WIFI_PA_LNA_CONFIG;

	switch (val) {
	case 0x7990:
		return is_mt7996(&dev->mt76) ? 0 : -EINVAL;
	case 0x7992:
		if (dev->fem_type == MT7996_FEM_UNSET)
			return is_mt7992(&dev->mt76) ? 0 : -EINVAL;

		if (fem[0] == FEM_EXT && fem[1] == FEM_EXT)
			fem_type = MT7996_FEM_EXT;
		else if (fem[0] == FEM_INT && fem[1] == FEM_INT)
			fem_type = MT7996_FEM_INT;
		else if (fem[0] == FEM_INT && fem[1] == FEM_EXT)
			fem_type = MT7996_FEM_MIX;
		else
			return -EINVAL;

		return (is_mt7992(&dev->mt76) ? 0 : -EINVAL) |
		       (dev->fem_type == fem_type ? 0 : -EINVAL);
	default:
		return -EINVAL;
	}
}

const char *mt7996_eeprom_name(struct mt7996_dev *dev)
{
	if (dev->bin_file_mode)
		return dev->mt76.bin_file_name;

	if (dev->testmode_enable) {
		if (is_mt7992(&dev->mt76))
			return MT7992_EEPROM_DEFAULT_TM;
		else
			return MT7996_EEPROM_DEFAULT_TM;
	}

	switch (mt76_chip(&dev->mt76)) {
	case 0x7990:
		if (dev->chip_sku == MT7996_SKU_404)
			return MT7996_EEPROM_DEFAULT_404;
		else if (dev->chip_sku == MT7996_SKU_233)
			return MT7996_EEPROM_DEFAULT_233;
		return MT7996_EEPROM_DEFAULT;
	case 0x7992:
		if (dev->chip_sku == MT7992_SKU_23) {
			if (dev->fem_type == MT7996_FEM_INT)
				return MT7992_EEPROM_DEFAULT_23;
			return MT7992_EEPROM_DEFAULT_23_EXT;
		} else if (dev->chip_sku == MT7992_SKU_44) {
			if (dev->fem_type == MT7996_FEM_INT)
				return MT7992_EEPROM_DEFAULT;
			else if (dev->fem_type == MT7996_FEM_MIX)
				return MT7992_EEPROM_DEFAULT_MIX;
			return MT7992_EEPROM_DEFAULT_EXT;
		}
		return MT7992_EEPROM_DEFAULT_24;
	default:
		return MT7996_EEPROM_DEFAULT;
	}
}

static int
mt7996_eeprom_load_default(struct mt7996_dev *dev)
{
	u8 *eeprom = dev->mt76.eeprom.data;
	const struct firmware *fw = NULL;
	int ret;

	ret = request_firmware(&fw, mt7996_eeprom_name(dev), dev->mt76.dev);
	if (ret)
		return ret;

	if (!fw || !fw->data) {
		if (dev->bin_file_mode)
			dev_err(dev->mt76.dev, "Invalid bin (bin file mode)\n");
		else
			dev_err(dev->mt76.dev, "Invalid default bin\n");
		ret = -EINVAL;
		goto out;
	}

	memcpy(eeprom, fw->data, MT7996_EEPROM_SIZE);
	dev->flash_mode = true;

out:
	release_firmware(fw);

	return ret;
}

static int mt7996_eeprom_load_flash(struct mt7996_dev *dev)
{
	int ret = 1;

	/* return > 0 for load success, return 0 for load failed, return < 0 for non memory */
	dev->bin_file_mode = mt76_check_bin_file_mode(&dev->mt76);
	if (dev->bin_file_mode) {
		dev->mt76.eeprom.size = MT7996_EEPROM_SIZE;
		dev->mt76.eeprom.data = devm_kzalloc(dev->mt76.dev, dev->mt76.eeprom.size,
						     GFP_KERNEL);
		if (!dev->mt76.eeprom.data)
			return -ENOMEM;

		if (mt7996_eeprom_load_default(dev))
			return 0;

		if (mt7996_check_eeprom(dev))
			return 0;
	} else {
		ret = mt76_eeprom_init(&dev->mt76, MT7996_EEPROM_SIZE);
	}

	return ret;
}

int mt7996_eeprom_check_fw_mode(struct mt7996_dev *dev)
{
	u8 *eeprom;
	int ret;

	/* load eeprom in flash or bin file mode to determine fw mode */
	ret = mt7996_eeprom_load_flash(dev);

	if (ret < 0)
		return ret;

	if (ret) {
		dev->flash_mode = true;
		dev->eeprom_mode = dev->bin_file_mode ? BIN_FILE_MODE : FLASH_MODE;
		eeprom = dev->mt76.eeprom.data;
		/* testmode enable priority: eeprom field > module parameter */
		dev->testmode_enable = !mt7996_check_eeprom(dev) ? eeprom[MT_EE_TESTMODE_EN] :
								   testmode_enable;
	}

	return ret;
}

static int mt7996_eeprom_load(struct mt7996_dev *dev)
{
	int ret;
	u8 free_block_num;
	u32 block_num, i;
	u32 eeprom_blk_size = MT7996_EEPROM_BLOCK_SIZE;

	/* flash or bin file mode eeprom is loaded before mcu init */
	if (!dev->flash_mode) {
		ret = mt7996_mcu_get_eeprom_free_block(dev, &free_block_num);
		if (ret < 0)
			return ret;

		/* efuse info isn't enough */
		if (free_block_num >= 59)
			return -EINVAL;

		/* read eeprom data from efuse */
		block_num = DIV_ROUND_UP(MT7996_EEPROM_SIZE, eeprom_blk_size);
		for (i = 0; i < block_num; i++) {
			ret = mt7996_mcu_get_eeprom(dev, i * eeprom_blk_size, NULL);
			if (ret < 0)
				return ret;
		}
		dev->eeprom_mode = EFUSE_MODE;
	}

	return mt7996_check_eeprom(dev);
}

static int mt7996_eeprom_parse_efuse_hw_cap(struct mt7996_dev *dev)
{
#define MODE_HE_ONLY		BIT(0)
#define WTBL_SIZE_GROUP		GENMASK(31, 28)
	u32 cap = 0;
	int ret;

	ret = mt7996_mcu_get_chip_config(dev, &cap);
	if (ret)
		return ret;

	cap = 0x4b249248;	/* internal hardcode */
	if (cap) {
		dev->has_eht = !(cap & MODE_HE_ONLY);
		dev->wtbl_size_group = u32_get_bits(cap, WTBL_SIZE_GROUP);
	}

	if (dev->wtbl_size_group < 2 || dev->wtbl_size_group > 4 ||
	    is_mt7992(&dev->mt76))
		dev->wtbl_size_group = 2; /* set default */

	return 0;
}

static int mt7996_eeprom_parse_band_config(struct mt7996_phy *phy)
{
	u8 *eeprom = phy->dev->mt76.eeprom.data;
	u32 val = eeprom[MT_EE_WIFI_CONF];
	int ret = 0;

	switch (phy->mt76->band_idx) {
	case MT_BAND1:
		val = FIELD_GET(MT_EE_WIFI_CONF1_BAND_SEL, val);
		break;
	case MT_BAND2:
		val = eeprom[MT_EE_WIFI_CONF + 1];
		val = FIELD_GET(MT_EE_WIFI_CONF2_BAND_SEL, val);
		break;
	default:
		val = FIELD_GET(MT_EE_WIFI_CONF0_BAND_SEL, val);
		break;
	}

	switch (val) {
	case MT_EE_BAND_SEL_2GHZ:
		phy->mt76->cap.has_2ghz = true;
		break;
	case MT_EE_BAND_SEL_5GHZ:
		phy->mt76->cap.has_5ghz = true;
		break;
	case MT_EE_BAND_SEL_6GHZ:
		phy->mt76->cap.has_6ghz = true;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int mt7996_eeprom_parse_hw_cap(struct mt7996_dev *dev, struct mt7996_phy *phy)
{
	u8 path, rx_path, nss, band_idx = phy->mt76->band_idx;
	u8 *eeprom = dev->mt76.eeprom.data;
	struct mt76_phy *mphy = phy->mt76;
	int max_path = 5, max_nss = 4;
	int ret;

	switch (band_idx) {
	case MT_BAND1:
		path = FIELD_GET(MT_EE_WIFI_CONF2_TX_PATH_BAND1,
				 eeprom[MT_EE_WIFI_CONF + 2]);
		rx_path = FIELD_GET(MT_EE_WIFI_CONF3_RX_PATH_BAND1,
				    eeprom[MT_EE_WIFI_CONF + 3]);
		nss = FIELD_GET(MT_EE_WIFI_CONF5_STREAM_NUM_BAND1,
				eeprom[MT_EE_WIFI_CONF + 5]);
		break;
	case MT_BAND2:
		path = FIELD_GET(MT_EE_WIFI_CONF2_TX_PATH_BAND2,
				 eeprom[MT_EE_WIFI_CONF + 2]);
		rx_path = FIELD_GET(MT_EE_WIFI_CONF4_RX_PATH_BAND2,
				    eeprom[MT_EE_WIFI_CONF + 4]);
		nss = FIELD_GET(MT_EE_WIFI_CONF5_STREAM_NUM_BAND2,
				eeprom[MT_EE_WIFI_CONF + 5]);
		break;
	default:
		path = FIELD_GET(MT_EE_WIFI_CONF1_TX_PATH_BAND0,
				 eeprom[MT_EE_WIFI_CONF + 1]);
		rx_path = FIELD_GET(MT_EE_WIFI_CONF3_RX_PATH_BAND0,
				    eeprom[MT_EE_WIFI_CONF + 3]);
		nss = FIELD_GET(MT_EE_WIFI_CONF4_STREAM_NUM_BAND0,
				eeprom[MT_EE_WIFI_CONF + 4]);
		break;
	}

	if (!path || path > max_path)
		path = max_path;

	if (!nss || nss > max_nss)
		nss = max_nss;

	nss = min_t(u8, nss, path);

	if (path != rx_path)
		phy->has_aux_rx = true;

	mphy->antenna_mask = BIT(nss) - 1;
	mphy->chainmask = (BIT(path) - 1) << dev->chainshift[band_idx];
	dev->chainmask |= mphy->chainmask;
	if (band_idx < MT_BAND2)
		dev->chainshift[band_idx + 1] = dev->chainshift[band_idx] +
						hweight16(mphy->chainmask);

	ret = mt7996_eeprom_parse_efuse_hw_cap(dev);
	if (ret)
		return ret;

	return mt7996_eeprom_parse_band_config(phy);
}

static int
mt7996_eeprom_load_precal_binfile(struct mt7996_dev *dev, u32 offs, u32 size)
{
	const struct firmware *fw = NULL;
	int ret;

	ret = request_firmware(&fw, dev->mt76.bin_file_name, dev->mt76.dev);
	if (ret)
		return ret;

	if (!fw || !fw->data) {
		dev_err(dev->mt76.dev, "Invalid bin (bin file mode), load precal fail\n");
		ret = -EINVAL;
		goto out;
	}

	memcpy(dev->cal, fw->data + offs, size);

out:
	release_firmware(fw);

	return ret;
}

static void mt7996_eeprom_init_precal(struct mt7996_dev *dev)
{
#define MT76_CHANNELS_5GHZ_SIZE		36	/* ARRAY_SIZE(mt76_channels_5ghz) */
#define MT76_CHANNELS_6GHZ_SIZE		59	/* ARRAY_SIZE(mt76_channels_6ghz) */

	dev->prek.dpd_ch_num[DPD_CH_NUM_BW20_2G] = ARRAY_SIZE(dpd_2g_ch_list_bw20);
	dev->prek.dpd_ch_num[DPD_CH_NUM_BW20_5G_SKIP] = ARRAY_SIZE(dpd_5g_skip_ch_list);
	dev->prek.dpd_ch_num[DPD_CH_NUM_BW20_5G] = MT76_CHANNELS_5GHZ_SIZE -
						   DPD_CH_NUM(BW20_5G_SKIP);
	dev->prek.dpd_ch_num[DPD_CH_NUM_BW160_5G] = ARRAY_SIZE(dpd_5g_ch_list_bw160);
	dev->prek.dpd_ch_num[DPD_CH_NUM_BW20_6G] = MT76_CHANNELS_6GHZ_SIZE;
	dev->prek.dpd_ch_num[DPD_CH_NUM_BW160_6G] = ARRAY_SIZE(dpd_6g_ch_list_bw160);

	switch (mt76_chip(&dev->mt76)) {
	case 0x7990:
		dev->prek.rev = mt7996_prek_rev;
		if (dev->chip_sku == MT7996_SKU_233)
			dev->prek.rev = mt7996_prek_rev_233;
		/* 5g & 6g bw 80 dpd channel list is not used */
		dev->prek.dpd_ch_num[DPD_CH_NUM_BW320_6G] = ARRAY_SIZE(dpd_6g_ch_list_bw320);
		break;
	case 0x7992:
		dev->prek.rev  = mt7992_prek_rev;
		dev->prek.dpd_ch_num[DPD_CH_NUM_BW80_5G] = ARRAY_SIZE(dpd_5g_ch_list_bw80);
		/* 6g is not used in current sku */
		dev->prek.dpd_ch_num[DPD_CH_NUM_BW20_6G] = 0;
		dev->prek.dpd_ch_num[DPD_CH_NUM_BW80_6G] = 0;
		dev->prek.dpd_ch_num[DPD_CH_NUM_BW160_6G] = 0;
		break;
	default:
		dev->prek.rev  = mt7996_prek_rev;
		break;
	}
}

static int mt7996_eeprom_load_precal(struct mt7996_dev *dev)
{
	struct mt76_dev *mdev = &dev->mt76;
	u8 *eeprom = mdev->eeprom.data;
	u32 offs = MT_EE_DO_PRE_CAL;
	u32 size, val = eeprom[offs];
	int ret;

	mt7996_eeprom_init_precal(dev);

	if (!dev->flash_mode || !val)
		return 0;

	size = MT_EE_CAL_GROUP_SIZE + MT_EE_CAL_DPD_SIZE;

	dev->cal = devm_kzalloc(mdev->dev, size, GFP_KERNEL);
	if (!dev->cal)
		return -ENOMEM;

	if (dev->bin_file_mode)
		return mt7996_eeprom_load_precal_binfile(dev, MT_EE_PRECAL, size);

	ret = mt76_get_of_data_from_mtd(mdev, dev->cal, offs, size);
	if (!ret)
		return ret;

	return mt76_get_of_data_from_nvmem(mdev, dev->cal, "precal", size);
}

static int mt7996_apply_cal_free_data(struct mt7996_dev *dev)
{
#define MT_EE_CAL_FREE_MAX_SIZE		30
#define MT_EE_7977BN_OFFSET		(0x1200 - 0x500)
#define MT_EE_END_OFFSET		0xffff
	enum adie_type {
		ADIE_7975,
		ADIE_7976,
		ADIE_7977,
		ADIE_7978,
		ADIE_7979,
	};
	static const u16 adie_offs_list[][MT_EE_CAL_FREE_MAX_SIZE] = {
		[ADIE_7975] = {0x5cd, 0x5cf, 0x5d1, 0x5d3, 0x6c0, 0x6c1, 0x6c2, 0x6c3,
			       0x7a1, 0x7a6, 0x7a8, 0x7aa, -1},
		[ADIE_7976] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x55, 0x57, 0x59,
			       0x70, 0x71, 0x790, 0x791, 0x794, 0x795, 0x7a6, 0x7a8, 0x7aa, -1},
		[ADIE_7977] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x55, 0x57, 0x59,
			       0x69, 0x6a, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, -1},
		[ADIE_7978] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x55, 0x57, 0x59,
			       0x90, 0x91, 0x94, 0x95, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
			       0x100, 0x101, 0x102, 0x103, 0x104, 0x105, -1},
		[ADIE_7979] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x55, 0x57, 0x59,
			       0x69, 0x6a, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, -1},
	};
	static const u16 eep_offs_list[][MT_EE_CAL_FREE_MAX_SIZE] = {
		[ADIE_7975] = {0x451, 0x453, 0x455, 0x457, 0x44c, 0x44d, 0x44e, 0x44f,
			       0xba1, 0xba6, 0xba8, 0xbaa, -1},
		[ADIE_7976] = {0x44c, 0x44d, 0x44e, 0x44f, 0x450,
			       0x451, 0x453, 0x455, 0x457, 0x459,
			       0x470, 0x471, 0xb90, 0xb91, 0xb94, 0xb95,
			       0xba6, 0xba8, 0xbaa, -1},
		[ADIE_7977] = {0x124c, 0x124d, 0x124e, 0x124f, 0x1250,
			       0x1251, 0x1253, 0x1255, 0x1257, 0x1259,
			       0x1269, 0x126a, 0x127a, 0x127b, 0x127c, 0x127d, 0x127e, -1},
		[ADIE_7978] = {0x44c, 0x44d, 0x44e, 0x44f, 0x450,
			       0x451, 0x453, 0x455, 0x457, 0x459,
			       0xb90, 0xb91, 0xb94, 0xb95,
			       0xba6, 0xba7, 0xba8, 0xba9, 0xbaa,
			       0x480, 0x481, 0x482, 0x483, 0x484, 0x485, -1},
		[ADIE_7979] = {0x124c, 0x124d, 0x124e, 0x124f, 0x1250,
			       0x1251, 0x1253, 0x1255, 0x1257, 0x1259,
			       0x1269, 0x126a, 0x127a, 0x127b, 0x127c, 0x127d, 0x127e, -1},
	};
	static const u16 adie_base_7996[] = {
		0x400, 0x1e00, 0x1200
	};
	static const u16 adie_base_7992[] = {
		0x400, 0x1200, 0x0
	};
	static const u16 *adie_offs[__MT_MAX_BAND];
	static const u16 *eep_offs[__MT_MAX_BAND];
	static const u16 *adie_base;
	u8 *eeprom = dev->mt76.eeprom.data;
	u8 buf[MT7996_EEPROM_BLOCK_SIZE];
	int adie_id, band, i, ret;

	switch (mt76_chip(&dev->mt76)) {
	case 0x7990:
		adie_base = adie_base_7996;
		/* adie 0 */
		if (dev->fem_type == MT7996_FEM_INT && dev->chip_sku != MT7996_SKU_233)
			adie_id = ADIE_7975;
		else
			adie_id = ADIE_7976;
		adie_offs[0] = adie_offs_list[adie_id];
		eep_offs[0] = eep_offs_list[adie_id];

		/* adie 1 */
		if (dev->chip_sku == MT7996_SKU_444) {
			adie_offs[1] = adie_offs_list[ADIE_7977];
			eep_offs[1] = eep_offs_list[ADIE_7977];
		}

		/* adie 2 */
		adie_offs[2] = adie_offs_list[ADIE_7977];
		eep_offs[2] = eep_offs_list[ADIE_7977];
		break;
	case 0x7992:
		adie_base = adie_base_7992;
		/* adie 0 */
		if (dev->chip_sku == MT7992_SKU_44 &&
		    dev->fem_type != MT7996_FEM_EXT)
			adie_id = ADIE_7975;
		else if (dev->chip_sku == MT7992_SKU_24)
			adie_id = ADIE_7978;
		else
			adie_id = ADIE_7976;
		adie_offs[0] = adie_offs_list[adie_id];
		eep_offs[0] = eep_offs_list[adie_id];

		/* adie 1 */
		if (dev->chip_sku == MT7992_SKU_44 &&
		    dev->fem_type != MT7996_FEM_INT)
			adie_id = ADIE_7977;
		else if (dev->chip_sku != MT7992_SKU_23)
			adie_id = ADIE_7979;
		else
			break;
		adie_offs[1] = adie_offs_list[adie_id];
		eep_offs[1] = eep_offs_list[adie_id];
		break;
	default:
		return -EINVAL;
	}

	for (band = 0; band < __MT_MAX_BAND; band++) {
		u16 adie_offset, eep_offset;
		u32 block_num, prev_block_num = -1;

		if (!adie_offs[band])
			continue;

		for (i = 0; i < MT_EE_CAL_FREE_MAX_SIZE; i++) {
			adie_offset = adie_offs[band][i] + adie_base[band];
			eep_offset = eep_offs[band][i];
			block_num = adie_offset / MT7996_EEPROM_BLOCK_SIZE;

			if (adie_offs[band][i] == MT_EE_END_OFFSET)
				break;

			if (is_mt7996(&dev->mt76) && dev->chip_sku == MT7996_SKU_444 &&
			    band == MT_BAND1)
				eep_offset -= MT_EE_7977BN_OFFSET;

			if (prev_block_num != block_num) {
				ret = mt7996_mcu_get_eeprom(dev, adie_offset, buf);
				if (ret) {
					prev_block_num = -1;
					continue;
				}
			}

			eeprom[eep_offset] = buf[adie_offset % MT7996_EEPROM_BLOCK_SIZE];
			prev_block_num = block_num;
		}
	}

	return 0;
}

int mt7996_eeprom_init(struct mt7996_dev *dev)
{
	int ret;

	ret = mt7996_get_chip_sku(dev);
	if (ret)
		return ret;

	ret = mt7996_eeprom_load(dev);
	if (ret < 0) {
		if (ret != -EINVAL)
			return ret;

		dev_warn(dev->mt76.dev, "eeprom load fail, use default bin\n");
		dev->bin_file_mode = false;
		dev->eeprom_mode = DEFAULT_BIN_MODE;
		ret = mt7996_eeprom_load_default(dev);
		if (ret)
			return ret;
	}

	ret = mt7996_eeprom_load_precal(dev);
	if (ret)
		return ret;

	ret = mt7996_apply_cal_free_data(dev);
	if (ret)
		return ret;

	ret = mt7996_eeprom_parse_hw_cap(dev, &dev->phy);
	if (ret < 0)
		return ret;

	memcpy(dev->mphy.macaddr, dev->mt76.eeprom.data + MT_EE_MAC_ADDR, ETH_ALEN);
	mt76_eeprom_override(&dev->mphy);

	return 0;
}

int mt7996_eeprom_get_target_power(struct mt7996_dev *dev,
				   struct ieee80211_channel *chan)
{
	u8 *eeprom = dev->mt76.eeprom.data;
	int target_power;

	if (chan->band == NL80211_BAND_5GHZ)
		target_power = eeprom[MT_EE_TX0_POWER_5G +
				      mt7996_get_channel_group_5g(chan->hw_value)];
	else if (chan->band == NL80211_BAND_6GHZ)
		target_power = eeprom[MT_EE_TX0_POWER_6G +
				      mt7996_get_channel_group_6g(chan->hw_value)];
	else
		target_power = eeprom[MT_EE_TX0_POWER_2G];

	return target_power;
}

s8 mt7996_eeprom_get_power_delta(struct mt7996_dev *dev, int band)
{
	u8 *eeprom = dev->mt76.eeprom.data;
	u32 val;
	s8 delta;

	if (band == NL80211_BAND_5GHZ)
		val = eeprom[MT_EE_RATE_DELTA_5G];
	else if (band == NL80211_BAND_6GHZ)
		val = eeprom[MT_EE_RATE_DELTA_6G];
	else
		val = eeprom[MT_EE_RATE_DELTA_2G];

	if (!(val & MT_EE_RATE_DELTA_EN))
		return 0;

	delta = FIELD_GET(MT_EE_RATE_DELTA_MASK, val);

	return val & MT_EE_RATE_DELTA_SIGN ? delta : -delta;
}

const u8 mt7996_sku_group_len[] = {
	[SKU_CCK] = 4,
	[SKU_OFDM] = 8,
	[SKU_HT20] = 8,
	[SKU_HT40] = 9,
	[SKU_VHT20] = 12,
	[SKU_VHT40] = 12,
	[SKU_VHT80] = 12,
	[SKU_VHT160] = 12,
	[SKU_HE26] = 12,
	[SKU_HE52] = 12,
	[SKU_HE106] = 12,
	[SKU_HE242] = 12,
	[SKU_HE484] = 12,
	[SKU_HE996] = 12,
	[SKU_HE2x996] = 12,
	[SKU_EHT26] = 16,
	[SKU_EHT52] = 16,
	[SKU_EHT106] = 16,
	[SKU_EHT242] = 16,
	[SKU_EHT484] = 16,
	[SKU_EHT996] = 16,
	[SKU_EHT2x996] = 16,
	[SKU_EHT4x996] = 16,
	[SKU_EHT26_52] = 16,
	[SKU_EHT26_106] = 16,
	[SKU_EHT484_242] = 16,
	[SKU_EHT996_484] = 16,
	[SKU_EHT996_484_242] = 16,
	[SKU_EHT2x996_484] = 16,
	[SKU_EHT3x996] = 16,
	[SKU_EHT3x996_484] = 16,
};
