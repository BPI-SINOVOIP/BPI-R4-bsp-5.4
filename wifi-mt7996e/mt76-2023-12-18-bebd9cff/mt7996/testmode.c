// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#include "mt7996.h"
#include "mac.h"
#include "mcu.h"
#include "testmode.h"
#include "eeprom.h"
#include "mtk_mcu.h"

enum {
	TM_CHANGED_TXPOWER,
	TM_CHANGED_FREQ_OFFSET,
	TM_CHANGED_SKU_EN,
	TM_CHANGED_TX_LENGTH,
	TM_CHANGED_TX_TIME,
	TM_CHANGED_CFG,
	TM_CHANGED_OFF_CHAN_CH,
	TM_CHANGED_OFF_CHAN_CENTER_CH,
	TM_CHANGED_OFF_CHAN_BW,
	TM_CHANGED_IPI_THRESHOLD,
	TM_CHANGED_IPI_PERIOD,
	TM_CHANGED_IPI_RESET,
	TM_CHANGED_TXBF_ACT,

	/* must be last */
	NUM_TM_CHANGED
};

static const u8 tm_change_map[] = {
	[TM_CHANGED_TXPOWER] = MT76_TM_ATTR_TX_POWER,
	[TM_CHANGED_FREQ_OFFSET] = MT76_TM_ATTR_FREQ_OFFSET,
	[TM_CHANGED_SKU_EN] = MT76_TM_ATTR_SKU_EN,
	[TM_CHANGED_TX_LENGTH] = MT76_TM_ATTR_TX_LENGTH,
	[TM_CHANGED_TX_TIME] = MT76_TM_ATTR_TX_TIME,
	[TM_CHANGED_CFG] = MT76_TM_ATTR_CFG,
	[TM_CHANGED_OFF_CHAN_CH] = MT76_TM_ATTR_OFF_CH_SCAN_CH,
	[TM_CHANGED_OFF_CHAN_CENTER_CH] = MT76_TM_ATTR_OFF_CH_SCAN_CENTER_CH,
	[TM_CHANGED_OFF_CHAN_BW] = MT76_TM_ATTR_OFF_CH_SCAN_BW,
	[TM_CHANGED_IPI_THRESHOLD] = MT76_TM_ATTR_IPI_THRESHOLD,
	[TM_CHANGED_IPI_PERIOD] = MT76_TM_ATTR_IPI_PERIOD,
	[TM_CHANGED_IPI_RESET] = MT76_TM_ATTR_IPI_RESET,
	[TM_CHANGED_TXBF_ACT] = MT76_TM_ATTR_TXBF_ACT,
};

static void mt7996_tm_ipi_work(struct work_struct *work);
static int mt7996_tm_txbf_apply_tx(struct mt7996_phy *phy, u16 wlan_idx,
				   bool ebf, bool ibf, bool phase_cal);

static u32 mt7996_tm_bw_mapping(enum nl80211_chan_width width, enum bw_mapping_method method)
{
	static const u32 width_to_bw[][NUM_BW_MAP] = {
		[NL80211_CHAN_WIDTH_40] = {FW_CDBW_40MHZ, TM_CBW_40MHZ, BF_CDBW_40MHZ, 40,
					   FIRST_CONTROL_CHAN_BITMAP_BW40},
		[NL80211_CHAN_WIDTH_80] = {FW_CDBW_80MHZ, TM_CBW_80MHZ, BF_CDBW_80MHZ, 80,
					   FIRST_CONTROL_CHAN_BITMAP_BW80},
		[NL80211_CHAN_WIDTH_80P80] = {FW_CDBW_8080MHZ, TM_CBW_8080MHZ, BF_CDBW_8080MHZ,
					      80, 0x0},
		[NL80211_CHAN_WIDTH_160] = {FW_CDBW_160MHZ, TM_CBW_160MHZ, BF_CDBW_160MHZ, 160,
					    FIRST_CONTROL_CHAN_BITMAP_BW160},
		[NL80211_CHAN_WIDTH_5] = {FW_CDBW_5MHZ, TM_CBW_5MHZ, BF_CDBW_5MHZ, 5, 0x0},
		[NL80211_CHAN_WIDTH_10] = {FW_CDBW_10MHZ, TM_CBW_10MHZ, BF_CDBW_10MHZ, 10, 0x0},
		[NL80211_CHAN_WIDTH_20] = {FW_CDBW_20MHZ, TM_CBW_20MHZ, BF_CDBW_20MHZ, 20, 0x0},
		[NL80211_CHAN_WIDTH_20_NOHT] = {FW_CDBW_20MHZ, TM_CBW_20MHZ, BF_CDBW_20MHZ,
						20, 0x0},
		[NL80211_CHAN_WIDTH_320] = {FW_CDBW_320MHZ, TM_CBW_320MHZ, BF_CDBW_320MHZ,
					    320, 0x0},
	};

	if (width >= ARRAY_SIZE(width_to_bw))
		return 0;

	return width_to_bw[width][method];
}

static u8 mt7996_tm_rate_mapping(u8 tx_rate_mode, enum rate_mapping_type type)
{
	static const u8 rate_to_phy[][NUM_RATE_MAP] = {
		[MT76_TM_TX_MODE_CCK] = {MT_PHY_TYPE_CCK, BF_LM_LEGACY},
		[MT76_TM_TX_MODE_OFDM] = {MT_PHY_TYPE_OFDM, BF_LM_LEGACY},
		[MT76_TM_TX_MODE_HT] = {MT_PHY_TYPE_HT, BF_LM_HT},
		[MT76_TM_TX_MODE_VHT] = {MT_PHY_TYPE_VHT, BF_LM_VHT},
		[MT76_TM_TX_MODE_HE_SU] = {MT_PHY_TYPE_HE_SU, BF_LM_HE},
		[MT76_TM_TX_MODE_HE_EXT_SU] = {MT_PHY_TYPE_HE_EXT_SU, BF_LM_HE},
		[MT76_TM_TX_MODE_HE_TB] = {MT_PHY_TYPE_HE_TB, BF_LM_HE},
		[MT76_TM_TX_MODE_HE_MU] = {MT_PHY_TYPE_HE_MU, BF_LM_HE},
		[MT76_TM_TX_MODE_EHT_SU] = {MT_PHY_TYPE_EHT_SU, BF_LM_EHT},
		[MT76_TM_TX_MODE_EHT_TRIG] = {MT_PHY_TYPE_EHT_TRIG, BF_LM_EHT},
		[MT76_TM_TX_MODE_EHT_MU] = {MT_PHY_TYPE_EHT_MU, BF_LM_EHT},
	};

	if (tx_rate_mode > MT76_TM_TX_MODE_MAX)
		return -EINVAL;

	return rate_to_phy[tx_rate_mode][type];
}

static int
mt7996_tm_check_antenna(struct mt7996_phy *phy)
{
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;
	u8 band_idx = phy->mt76->band_idx;
	u32 chainmask = phy->mt76->chainmask;
	u32 aux_rx_mask;

	chainmask = chainmask >> dev->chainshift[band_idx];
	aux_rx_mask = BIT(fls(chainmask)) * phy->has_aux_rx;
	if (td->tx_antenna_mask & ~(chainmask | aux_rx_mask)) {
		dev_err(dev->mt76.dev,
			"tx antenna mask 0x%x exceeds hw limit (chainmask 0x%x, has aux rx: %s)\n",
			td->tx_antenna_mask, chainmask, phy->has_aux_rx ? "yes" : "no");
		return -EINVAL;
	}

	return 0;
}

static int
mt7996_tm_set(struct mt7996_dev *dev, u32 func_idx, u32 data)
{
	struct mt7996_tm_req req = {
		.rf_test = {
			.tag = cpu_to_le16(UNI_RF_TEST_CTRL),
			.len = cpu_to_le16(sizeof(req.rf_test)),
			.action = RF_ACTION_SET,
			.op.rf.func_idx = func_idx,
			.op.rf.param.func_data = cpu_to_le32(data),
		},
	};
	bool wait = (data == RF_CMD(START_TX)) ? true : false;

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_CTRL), &req,
				 sizeof(req), wait);
}

static int
mt7996_tm_get(struct mt7996_dev *dev, u32 func_idx, u32 data, u32 *result)
{
	struct mt7996_tm_req req = {
		.rf_test = {
			.tag = cpu_to_le16(UNI_RF_TEST_CTRL),
			.len = cpu_to_le16(sizeof(req.rf_test)),
			.action = RF_ACTION_GET,
			.op.rf.func_idx = func_idx,
			.op.rf.param.func_data = cpu_to_le32(data),
		},
	};
	struct mt7996_tm_event *event;
	struct sk_buff *skb;
	int ret;

	ret = mt76_mcu_send_and_get_msg(&dev->mt76, MCU_WM_UNI_CMD_QUERY(TESTMODE_CTRL),
					&req, sizeof(req), true, &skb);
	if (ret)
		return ret;

	event = (struct mt7996_tm_event *)skb->data;
	*result = event->result.payload_length;

	dev_kfree_skb(skb);

	return ret;
}

static void
mt7996_tm_set_antenna(struct mt7996_phy *phy, u32 func_idx)
{
#define SPE_INDEX_MASK		BIT(31)
#define TX_ANTENNA_MASK		GENMASK(3, 0)
#define RX_ANTENNA_MASK		GENMASK(20, 16)		/* RX antenna mask at most 5 bit */
	struct mt7996_dev *dev = phy->dev;
	struct mt76_testmode_data *td = &phy->mt76->test;
	u32 antenna_mask;

	if (!mt76_testmode_param_present(td, MT76_TM_ATTR_TX_ANTENNA))
		return;

	if (func_idx == SET_ID(TX_PATH))
		antenna_mask = td->tx_spe_idx ? (SPE_INDEX_MASK | td->tx_spe_idx) :
						td->tx_antenna_mask & TX_ANTENNA_MASK;
	else if (func_idx == SET_ID(RX_PATH))
		antenna_mask = u32_encode_bits(td->tx_antenna_mask, RX_ANTENNA_MASK);
	else
		return;

	mt7996_tm_set(dev, func_idx, antenna_mask);
}

static void
mt7996_tm_set_mac_addr(struct mt7996_dev *dev, u8 *addr, u32 func_idx)
{
#define REMAIN_PART_TAG		BIT(18)
	u32 own_mac_first = 0, own_mac_remain = 0;
	int len = sizeof(u32);

	memcpy(&own_mac_first, addr, len);
	mt7996_tm_set(dev, func_idx, own_mac_first);
	/* Set the remain part of mac address */
	memcpy(&own_mac_remain, addr + len, ETH_ALEN - len);
	mt7996_tm_set(dev, func_idx | REMAIN_PART_TAG, own_mac_remain);
}

static int
mt7996_tm_rf_switch_mode(struct mt7996_dev *dev, u32 op_mode)
{
	struct mt7996_tm_req req = {
		.rf_test = {
			.tag = cpu_to_le16(UNI_RF_TEST_CTRL),
			.len = cpu_to_le16(sizeof(req.rf_test)),
			.action = RF_ACTION_SWITCH_TO_RF_TEST,
			.op.op_mode = cpu_to_le32(op_mode),
		},
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_CTRL), &req,
				 sizeof(req), false);
}

static void
mt7996_tm_init(struct mt7996_phy *phy, bool en)
{
	struct mt7996_dev *dev = phy->dev;
	u8 rf_test_mode = en ? RF_OPER_RF_TEST : RF_OPER_NORMAL;

	if (!test_bit(MT76_STATE_RUNNING, &phy->mt76->state))
		return;

	mt7996_mcu_set_tx_power_ctrl(phy, POWER_CTRL(ATE_MODE), en);
	mt7996_mcu_set_tx_power_ctrl(phy, POWER_CTRL(SKU_POWER_LIMIT), !en);
	mt7996_mcu_set_tx_power_ctrl(phy, POWER_CTRL(BACKOFF_POWER_LIMIT), !en);

	mt7996_tm_rf_switch_mode(dev, rf_test_mode);

	mt7996_mcu_add_bss_info(phy, phy->monitor_vif, en);
	mt7996_mcu_add_sta(dev, phy->monitor_vif, NULL, en);

	mt7996_tm_set(dev, SET_ID(BAND_IDX), phy->mt76->band_idx);

	/* use firmware counter for RX stats */
	phy->mt76->test.flag |= MT_TM_FW_RX_COUNT;

	if (en)
		INIT_DELAYED_WORK(&phy->ipi_work, mt7996_tm_ipi_work);
}

void
mt7996_tm_update_channel(struct mt7996_phy *phy)
{
#define CHAN_FREQ_BW_80P80_TAG		(SET_ID(CHAN_FREQ) | BIT(16))
	struct mt7996_dev *dev = phy->dev;
	struct cfg80211_chan_def *chandef = &phy->mt76->chandef;
	struct ieee80211_channel *chan = chandef->chan;
	u8 width = chandef->width;
	static const u8 ch_band[] = {
		[NL80211_BAND_2GHZ] = 0,
		[NL80211_BAND_5GHZ] = 1,
		[NL80211_BAND_6GHZ] = 2,
	};

	if (!chan || !chandef) {
		dev_info(dev->mt76.dev, "chandef not found, channel update failed!\n");
		return;
	}

	/* system bw */
	mt7996_tm_set(dev, SET_ID(CBW), mt7996_tm_bw_mapping(width, BW_MAP_NL_TO_FW));

	if (width == NL80211_CHAN_WIDTH_80P80) {
		width = NL80211_CHAN_WIDTH_160;
		mt7996_tm_set(dev, CHAN_FREQ_BW_80P80_TAG, chandef->center_freq2 * 1000);
	}

	/* TODO: define per-packet bw */
	/* per-packet bw */
	mt7996_tm_set(dev, SET_ID(DBW), mt7996_tm_bw_mapping(width, BW_MAP_NL_TO_FW));

	/* control channel selection index */
	mt7996_tm_set(dev, SET_ID(PRIMARY_CH), 0);
	mt7996_tm_set(dev, SET_ID(BAND), ch_band[chan->band]);

	/* trigger switch channel calibration */
	mt7996_tm_set(dev, SET_ID(CHAN_FREQ), chandef->center_freq1 * 1000);

	// TODO: update power limit table
}

static void
mt7996_tm_tx_stop(struct mt76_phy *mphy)
{
	struct mt76_testmode_data *td = &mphy->test;
	struct mt7996_phy *phy = mphy->priv;
	struct mt7996_dev *dev = phy->dev;

	mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(STOP_TEST));
	td->tx_pending = 0;
}

static void
mt7996_tm_set_tx_frames(struct mt7996_phy *phy, bool en)
{
#define FRAME_CONTROL		0x88
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;

	//TODO: RU operation, replace mcs, nss, and ldpc
	if (en) {
		mt7996_tm_set(dev, SET_ID(MAC_HEADER), FRAME_CONTROL);
		mt7996_tm_set(dev, SET_ID(SEQ_CTRL), 0);
		mt7996_tm_set(dev, SET_ID(TX_COUNT), td->tx_count);
		mt7996_tm_set(dev, SET_ID(TX_MODE),
			      mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_PHY));
		mt7996_tm_set(dev, SET_ID(TX_RATE), td->tx_rate_idx);

		if (mt76_testmode_param_present(td, MT76_TM_ATTR_TX_POWER))
			mt7996_tm_set(dev, SET_ID(POWER), td->tx_power[0]);

		if (mt76_testmode_param_present(td, MT76_TM_ATTR_TX_TIME)) {
			mt7996_tm_set(dev, SET_ID(TX_LEN), 0);
			mt7996_tm_set(dev, SET_ID(TX_TIME), td->tx_time);
		} else {
			mt7996_tm_set(dev, SET_ID(TX_LEN), td->tx_mpdu_len);
			mt7996_tm_set(dev, SET_ID(TX_TIME), 0);
		}

		mt7996_tm_set_antenna(phy, SET_ID(TX_PATH));
		mt7996_tm_set_antenna(phy, SET_ID(RX_PATH));
		mt7996_tm_set(dev, SET_ID(STBC), td->tx_rate_stbc);
		mt7996_tm_set(dev, SET_ID(ENCODE_MODE), td->tx_rate_ldpc);
		mt7996_tm_set(dev, SET_ID(IBF_ENABLE), td->ibf);
		mt7996_tm_set(dev, SET_ID(EBF_ENABLE), td->ebf);
		mt7996_tm_set(dev, SET_ID(IPG), td->tx_ipg);
		mt7996_tm_set(dev, SET_ID(GI), td->tx_rate_sgi);
		mt7996_tm_set(dev, SET_ID(NSS), td->tx_rate_nss);
		mt7996_tm_set(dev, SET_ID(AID_OFFSET), 0);
		mt7996_tm_set(dev, SET_ID(PUNCTURE), td->tx_preamble_puncture);

		mt7996_tm_set(dev, SET_ID(MAX_PE), 2);
		mt7996_tm_set(dev, SET_ID(HW_TX_MODE), 0);
		if (!td->bf_en)
			mt7996_tm_update_channel(phy);

		/* trigger firmware to start TX */
		mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(START_TX));
	} else {
		mt7996_tm_tx_stop(phy->mt76);
	}
}

static int
mt7996_tm_rx_stats_user_ctrl(struct mt7996_phy *phy, u16 user_idx)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_rx_req req = {
		.band = phy->mt76->band_idx,
		.user_ctrl = {
			.tag = cpu_to_le16(UNI_TM_RX_STAT_SET_USER_CTRL),
			.len = cpu_to_le16(sizeof(req.user_ctrl)),
			.band_idx = phy->mt76->band_idx,
			.user_idx = cpu_to_le16(user_idx),
		},
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_RX_STAT), &req,
				 sizeof(req), false);
}

static void
mt7996_tm_set_rx_frames(struct mt7996_phy *phy, bool en)
{
#define RX_MU_DISABLE	0xf800
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;
	int ret;

	if (en) {
		ret = mt7996_tm_rx_stats_user_ctrl(phy, td->aid);
		if (ret) {
			dev_info(dev->mt76.dev, "Set RX stats user control failed!\n");
			return;
		}

		if (!td->bf_en)
			mt7996_tm_update_channel(phy);

		if (td->tx_rate_mode >= MT76_TM_TX_MODE_HE_MU) {
			if (td->aid)
				ret = mt7996_tm_set(dev, SET_ID(RX_MU_AID), td->aid);
			else
				ret = mt7996_tm_set(dev, SET_ID(RX_MU_AID), RX_MU_DISABLE);
		}
		mt7996_tm_set(dev, SET_ID(TX_MODE),
			      mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_PHY));
		mt7996_tm_set(dev, SET_ID(GI), td->tx_rate_sgi);
		mt7996_tm_set_antenna(phy, SET_ID(RX_PATH));
		mt7996_tm_set(dev, SET_ID(MAX_PE), 2);

		mt7996_tm_set_mac_addr(dev, td->addr[1], SET_ID(SA));

		/* trigger firmware to start RX */
		mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(START_RX));
	} else {
		/* trigger firmware to stop RX */
		mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(STOP_TEST));
	}
}

static void
mt7996_tm_set_tx_cont(struct mt7996_phy *phy, bool en)
{
#define CONT_WAVE_MODE_OFDM	3
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;

	if (en) {
		mt7996_tm_update_channel(phy);
		mt7996_tm_set(dev, SET_ID(TX_MODE),
			      mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_PHY));
		mt7996_tm_set(dev, SET_ID(TX_RATE), td->tx_rate_idx);
		/* fix payload is OFDM */
		mt7996_tm_set(dev, SET_ID(CONT_WAVE_MODE), CONT_WAVE_MODE_OFDM);
		mt7996_tm_set(dev, SET_ID(ANT_MASK), td->tx_antenna_mask);

		/* trigger firmware to start CONT TX */
		mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(CONT_WAVE));
	} else {
		/* trigger firmware to stop CONT TX  */
		mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(STOP_TEST));
	}
}

static int
mt7996_tm_group_prek(struct mt7996_phy *phy, enum mt76_testmode_state state)
{
	u8 *eeprom, do_precal;
	u32 i, group_size, dpd_size, size, offs, *pre_cal;
	int ret = 0;
	struct mt7996_dev *dev = phy->dev;
	struct mt76_dev *mdev = &dev->mt76;
	struct mt7996_tm_req req = {
		.rf_test = {
			.tag = cpu_to_le16(UNI_RF_TEST_CTRL),
			.len = cpu_to_le16(sizeof(req.rf_test)),
			.action = RF_ACTION_IN_RF_TEST,
			.icap_len = RF_TEST_ICAP_LEN,
			.op.rf.func_idx = cpu_to_le32(RF_TEST_RE_CAL),
			.op.rf.param.cal_param.func_data = cpu_to_le32(RF_PRE_CAL),
			.op.rf.param.cal_param.band_idx = phy->mt76->band_idx,
		},
	};

	if (!dev->flash_mode) {
		dev_err(dev->mt76.dev, "Currently not in FLASH or BIN FILE mode, return!\n");
		return -EOPNOTSUPP;
	}

	eeprom = mdev->eeprom.data;
	dev->cur_prek_offset = 0;
	group_size = MT_EE_CAL_GROUP_SIZE;
	dpd_size = MT_EE_CAL_DPD_SIZE;
	size = group_size + dpd_size;
	offs = MT_EE_DO_PRE_CAL;
	do_precal = (MT_EE_WIFI_CAL_GROUP_2G * !!PREK(GROUP_SIZE_2G)) |
		    (MT_EE_WIFI_CAL_GROUP_5G * !!PREK(GROUP_SIZE_5G)) |
		    (MT_EE_WIFI_CAL_GROUP_6G * !!PREK(GROUP_SIZE_6G));

	switch (state) {
	case MT76_TM_STATE_GROUP_PREK:
		if (!dev->cal) {
			dev->cal = devm_kzalloc(mdev->dev, size, GFP_KERNEL);
			if (!dev->cal)
				return -ENOMEM;
		}

		ret = mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_CTRL), &req,
					sizeof(req), false);
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == group_size,
				   30 * HZ);

		if (ret)
			dev_err(dev->mt76.dev, "Group Pre-cal: mcu send msg failed!\n");
		else
			eeprom[offs] |= do_precal;
		break;
	case MT76_TM_STATE_GROUP_PREK_DUMP:
		pre_cal = (u32 *)dev->cal;
		if (!pre_cal) {
			dev_info(dev->mt76.dev, "Not group pre-cal yet!\n");
			return ret;
		}
		dev_info(dev->mt76.dev, "Group Pre-Cal:\n");
		for (i = 0; i < (group_size / sizeof(u32)); i += 4) {
			dev_info(dev->mt76.dev, "[0x%08lx] 0x%8x 0x%8x 0x%8x 0x%8x\n",
				 i * sizeof(u32), pre_cal[i], pre_cal[i + 1],
				 pre_cal[i + 2], pre_cal[i + 3]);
		}
		break;
	case MT76_TM_STATE_GROUP_PREK_CLEAN:
		pre_cal = (u32 *)dev->cal;
		if (!pre_cal)
			return ret;
		memset(pre_cal, 0, group_size);
		eeprom[offs] &= ~MT_EE_WIFI_CAL_GROUP;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int
mt7996_tm_dpd_prek_send_req(struct mt7996_phy *phy, struct mt7996_tm_req *req,
			    const struct ieee80211_channel *chan_list, u32 channel_size,
			    enum nl80211_chan_width width, u32 func_data)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt76_phy *mphy = phy->mt76;
	struct cfg80211_chan_def chandef_backup, *chandef = &mphy->chandef;
	struct ieee80211_channel chan_backup;
	int i, ret, skip_ch_num = DPD_CH_NUM(BW20_5G_SKIP);

	if (!chan_list)
		return -EOPNOTSUPP;
	if (!channel_size)
		return 0;

	req->rf_test.op.rf.param.cal_param.func_data = cpu_to_le32(func_data);

	memcpy(&chan_backup, chandef->chan, sizeof(struct ieee80211_channel));
	memcpy(&chandef_backup, chandef, sizeof(struct cfg80211_chan_def));

	for (i = 0; i < channel_size; i++) {
		if (chan_list[i].band == NL80211_BAND_5GHZ &&
		    chan_list[i].hw_value >= dpd_5g_skip_ch_list[0].hw_value &&
		    chan_list[i].hw_value <= dpd_5g_skip_ch_list[skip_ch_num - 1].hw_value)
			continue;

		memcpy(chandef->chan, &chan_list[i], sizeof(struct ieee80211_channel));
		chandef->width = width;

		/* set channel switch reason */
		mphy->hw->conf.flags |= IEEE80211_CONF_OFFCHANNEL;
		mt7996_mcu_set_chan_info(phy, UNI_CHANNEL_SWITCH);

		ret = mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_CTRL), req,
					sizeof(*req), false);
		if (ret) {
			dev_err(dev->mt76.dev, "DPD Pre-cal: mcu send msg failed!\n");
			goto out;
		}
	}

out:
	mphy->hw->conf.flags &= ~IEEE80211_CONF_OFFCHANNEL;
	memcpy(chandef, &chandef_backup, sizeof(struct cfg80211_chan_def));
	memcpy(chandef->chan, &chan_backup, sizeof(struct ieee80211_channel));
	mt7996_mcu_set_chan_info(phy, UNI_CHANNEL_SWITCH);

	return ret;
}

static int
mt7996_tm_dpd_prek(struct mt7996_phy *phy, enum mt76_testmode_state state)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt76_dev *mdev = &dev->mt76;
	struct mt76_phy *mphy = phy->mt76;
	struct mt7996_tm_req req = {
		.rf_test = {
			.tag = cpu_to_le16(UNI_RF_TEST_CTRL),
			.len = cpu_to_le16(sizeof(req.rf_test)),
			.action = RF_ACTION_IN_RF_TEST,
			.icap_len = RF_TEST_ICAP_LEN,
			.op.rf.func_idx = cpu_to_le32(RF_TEST_RE_CAL),
			.op.rf.param.cal_param.band_idx = phy->mt76->band_idx,
		},
	};
	u32 i, j, group_size, dpd_size, size, offs, *pre_cal;
	u32 wait_on_prek_offset = 0;
	u8 do_precal, *eeprom;
	int ret = 0;

	if (!dev->flash_mode) {
		dev_err(dev->mt76.dev, "Currently not in FLASH or BIN FILE mode, return!\n");
		return -EOPNOTSUPP;
	}

	eeprom = mdev->eeprom.data;
	dev->cur_prek_offset = 0;
	group_size = MT_EE_CAL_GROUP_SIZE;
	dpd_size = MT_EE_CAL_DPD_SIZE;
	size = group_size + dpd_size;
	offs = MT_EE_DO_PRE_CAL;

	if (!dev->cal && state < MT76_TM_STATE_DPD_DUMP) {
		dev->cal = devm_kzalloc(mdev->dev, size, GFP_KERNEL);
		if (!dev->cal)
			return -ENOMEM;
	}

	switch (state) {
	case MT76_TM_STATE_DPD_2G:
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_2g_ch_list_bw20,
						  DPD_CH_NUM(BW20_2G),
						  NL80211_CHAN_WIDTH_20, RF_DPD_FLAT_CAL);
		wait_on_prek_offset += DPD_CH_NUM(BW20_2G) * DPD_PER_CH_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		do_precal = MT_EE_WIFI_CAL_DPD_2G;
		break;
	case MT76_TM_STATE_DPD_5G:
		/* 5g channel bw20 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, mphy->sband_5g.sband.channels,
						  mphy->sband_5g.sband.n_channels,
						  NL80211_CHAN_WIDTH_20, RF_DPD_FLAT_5G_CAL);
		if (ret)
			return ret;
		wait_on_prek_offset += DPD_CH_NUM(BW20_5G) * DPD_PER_CH_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		/* 5g channel bw80 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_5g_ch_list_bw80,
						  DPD_CH_NUM(BW80_5G),
						  NL80211_CHAN_WIDTH_80, RF_DPD_FLAT_5G_MEM_CAL);
		if (ret)
			return ret;
		wait_on_prek_offset += DPD_CH_NUM(BW80_5G) * DPD_PER_CH_GT_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		/* 5g channel bw160 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_5g_ch_list_bw160,
						  DPD_CH_NUM(BW160_5G),
						  NL80211_CHAN_WIDTH_160, RF_DPD_FLAT_5G_MEM_CAL);
		wait_on_prek_offset += DPD_CH_NUM(BW160_5G) * DPD_PER_CH_GT_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		do_precal = MT_EE_WIFI_CAL_DPD_5G;
		break;
	case MT76_TM_STATE_DPD_6G:
		/* 6g channel bw20 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, mphy->sband_6g.sband.channels,
						  mphy->sband_6g.sband.n_channels,
						  NL80211_CHAN_WIDTH_20, RF_DPD_FLAT_6G_CAL);
		if (ret)
			return ret;
		wait_on_prek_offset += DPD_CH_NUM(BW20_6G) * DPD_PER_CH_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		/* 6g channel bw80 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_6g_ch_list_bw80,
						  DPD_CH_NUM(BW80_6G),
						  NL80211_CHAN_WIDTH_80, RF_DPD_FLAT_6G_MEM_CAL);
		if (ret)
			return ret;
		wait_on_prek_offset += DPD_CH_NUM(BW80_6G) * DPD_PER_CH_GT_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		/* 6g channel bw160 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_6g_ch_list_bw160,
						  DPD_CH_NUM(BW160_6G),
						  NL80211_CHAN_WIDTH_160, RF_DPD_FLAT_6G_MEM_CAL);
		if (ret)
			return ret;
		wait_on_prek_offset += DPD_CH_NUM(BW160_6G) * DPD_PER_CH_GT_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		/* 6g channel bw320 calibration */
		ret = mt7996_tm_dpd_prek_send_req(phy, &req, dpd_6g_ch_list_bw320,
						  DPD_CH_NUM(BW320_6G),
						  NL80211_CHAN_WIDTH_320, RF_DPD_FLAT_6G_MEM_CAL);
		wait_on_prek_offset += DPD_CH_NUM(BW320_6G) * DPD_PER_CH_GT_BW20_SIZE;
		wait_event_timeout(mdev->mcu.wait, dev->cur_prek_offset == wait_on_prek_offset,
				   30 * HZ);

		do_precal = MT_EE_WIFI_CAL_DPD_6G;
		break;
	case MT76_TM_STATE_DPD_DUMP:
		if (!dev->cal) {
			dev_info(dev->mt76.dev, "Not DPD pre-cal yet!\n");
			return ret;
		}
		pre_cal = (u32 *)dev->cal;
		dev_info(dev->mt76.dev, "DPD Pre-Cal:\n");
		for (i = 0; i < dpd_size / sizeof(u32); i += 4) {
			j = i + (group_size / sizeof(u32));
			dev_info(dev->mt76.dev, "[0x%08lx] 0x%8x 0x%8x 0x%8x 0x%8x\n",
				 j * sizeof(u32), pre_cal[j], pre_cal[j + 1],
				 pre_cal[j + 2], pre_cal[j + 3]);
		}
		return 0;
	case MT76_TM_STATE_DPD_CLEAN:
		pre_cal = (u32 *)dev->cal;
		if (!pre_cal)
			return ret;
		memset(pre_cal + (group_size / sizeof(u32)), 0, dpd_size);
		do_precal = MT_EE_WIFI_CAL_DPD;
		eeprom[offs] &= ~do_precal;
		return 0;
	default:
		return -EINVAL;
	}

	if (!ret)
		eeprom[offs] |= do_precal;

	return ret;
}

static int
mt7996_tm_dump_precal(struct mt76_phy *mphy, struct sk_buff *msg, int flag, int type)
{
#define DPD_PER_CHAN_SIZE_MASK		GENMASK(31, 30)
#define DPD_2G_RATIO_MASK		GENMASK(29, 20)
#define DPD_5G_RATIO_MASK		GENMASK(19, 10)
#define DPD_6G_RATIO_MASK		GENMASK(9, 0)
	struct mt7996_phy *phy = mphy->priv;
	struct mt7996_dev *dev = phy->dev;
	u32 i, group_size, dpd_size, total_size, size, dpd_info = 0;
	u32 dpd_size_2g, dpd_size_5g, dpd_size_6g;
	u32 base, offs, transmit_size = 1000;
	u8 *pre_cal, *eeprom;
	void *precal;
	enum prek_ops {
		PREK_GET_INFO,
		PREK_SYNC_ALL,
		PREK_SYNC_GROUP,
		PREK_SYNC_DPD_2G,
		PREK_SYNC_DPD_5G,
		PREK_SYNC_DPD_6G,
		PREK_CLEAN_GROUP,
		PREK_CLEAN_DPD,
	};

	if (!dev->cal) {
		dev_info(dev->mt76.dev, "Not pre-cal yet!\n");
		return 0;
	}

	group_size = MT_EE_CAL_GROUP_SIZE;
	dpd_size = MT_EE_CAL_DPD_SIZE;
	total_size = group_size + dpd_size;
	pre_cal = dev->cal;
	eeprom = dev->mt76.eeprom.data;
	offs = MT_EE_DO_PRE_CAL;

	dpd_size_2g = MT_EE_CAL_DPD_SIZE_2G;
	dpd_size_5g = MT_EE_CAL_DPD_SIZE_5G;
	dpd_size_6g = MT_EE_CAL_DPD_SIZE_6G;

	switch (type) {
	case PREK_SYNC_ALL:
		base = 0;
		size = total_size;
		break;
	case PREK_SYNC_GROUP:
		base = 0;
		size = group_size;
		break;
	case PREK_SYNC_DPD_2G:
		base = group_size;
		size = dpd_size_2g;
		break;
	case PREK_SYNC_DPD_5G:
		base = group_size + dpd_size_2g;
		size = dpd_size_5g;
		break;
	case PREK_SYNC_DPD_6G:
		base = group_size + dpd_size_2g + dpd_size_5g;
		size = dpd_size_6g;
		break;
	case PREK_GET_INFO:
		break;
	default:
		return 0;
	}

	if (!flag) {
		if (eeprom[offs] & MT_EE_WIFI_CAL_DPD) {
			dpd_info |= u32_encode_bits(1, DPD_PER_CHAN_SIZE_MASK) |
				    u32_encode_bits(dpd_size_2g / MT_EE_CAL_UNIT,
						    DPD_2G_RATIO_MASK) |
				    u32_encode_bits(dpd_size_5g / MT_EE_CAL_UNIT,
						    DPD_5G_RATIO_MASK) |
				    u32_encode_bits(dpd_size_6g / MT_EE_CAL_UNIT,
						    DPD_6G_RATIO_MASK);
		}
		dev->cur_prek_offset = 0;
		precal = nla_nest_start(msg, MT76_TM_ATTR_PRECAL_INFO);
		if (!precal)
			return -ENOMEM;
		nla_put_u32(msg, 0, group_size);
		nla_put_u32(msg, 1, dpd_size);
		nla_put_u32(msg, 2, dpd_info);
		nla_put_u32(msg, 3, transmit_size);
		nla_put_u32(msg, 4, eeprom[offs]);
		nla_nest_end(msg, precal);
	} else {
		precal = nla_nest_start(msg, MT76_TM_ATTR_PRECAL);
		if (!precal)
			return -ENOMEM;

		transmit_size = (dev->cur_prek_offset + transmit_size < size) ?
				transmit_size : (size - dev->cur_prek_offset);
		for (i = 0; i < transmit_size; i++) {
			if (nla_put_u8(msg, i, pre_cal[base + dev->cur_prek_offset + i]))
				return -ENOMEM;
		}
		dev->cur_prek_offset += transmit_size;

		nla_nest_end(msg, precal);
	}

	return 0;
}

static void
mt7996_tm_re_cal_event(struct mt7996_dev *dev, struct mt7996_tm_rf_test_result *result,
		       struct mt7996_tm_rf_test_data *data)
{
	u32 base, dpd_size_2g, dpd_size_5g, dpd_size_6g, cal_idx, cal_type, len = 0;
	u8 *pre_cal;

	pre_cal = dev->cal;
	dpd_size_2g = MT_EE_CAL_DPD_SIZE_2G;
	dpd_size_5g = MT_EE_CAL_DPD_SIZE_5G;
	dpd_size_6g = MT_EE_CAL_DPD_SIZE_6G;

	cal_idx = le32_to_cpu(data->cal_idx);
	cal_type = le32_to_cpu(data->cal_type);
	len = le32_to_cpu(result->payload_length);
	len = len - sizeof(struct mt7996_tm_rf_test_data);

	switch (cal_type) {
	case RF_PRE_CAL:
		base = 0;
		break;
	case RF_DPD_FLAT_CAL:
		base = MT_EE_CAL_GROUP_SIZE;
		break;
	case RF_DPD_FLAT_5G_CAL:
	case RF_DPD_FLAT_5G_MEM_CAL:
		base = MT_EE_CAL_GROUP_SIZE + dpd_size_2g;
		break;
	case RF_DPD_FLAT_6G_CAL:
	case RF_DPD_FLAT_6G_MEM_CAL:
		base = MT_EE_CAL_GROUP_SIZE + dpd_size_2g + dpd_size_5g;
		break;
	default:
		dev_info(dev->mt76.dev, "Unknown calibration type!\n");
		return;
	}
	pre_cal += (base + dev->cur_prek_offset);

	memcpy(pre_cal, data->cal_data, len);
	dev->cur_prek_offset += len;
}

void mt7996_tm_rf_test_event(struct mt7996_dev *dev, struct sk_buff *skb)
{
	struct mt7996_tm_event *event;
	struct mt7996_tm_rf_test_result *result;
	struct mt7996_tm_rf_test_data *data;
	static u32 event_type;

	skb_pull(skb, sizeof(struct mt7996_mcu_rxd));
	event = (struct mt7996_tm_event *)skb->data;
	result = (struct mt7996_tm_rf_test_result *)&event->result;
	data = (struct mt7996_tm_rf_test_data *)result->data;

	event_type = le32_to_cpu(result->func_idx);

	switch (event_type) {
	case RF_TEST_RE_CAL:
		mt7996_tm_re_cal_event(dev, result, data);
		break;
	default:
		break;
	}
}

static u8
mt7996_tm_get_center_chan(struct mt7996_phy *phy, struct cfg80211_chan_def *chandef)
{
	struct mt76_phy *mphy = phy->mt76;
	const struct ieee80211_channel *chan = mphy->sband_5g.sband.channels;
	u32 bitmap, i, offset, width_mhz, size = mphy->sband_5g.sband.n_channels;
	u16 first_control = 0, control_chan = chandef->chan->hw_value;
	bool not_first;

	bitmap = mt7996_tm_bw_mapping(chandef->width, BW_MAP_NL_TO_CONTROL_BITMAP_5G);
	if (!bitmap)
		return control_chan;

	width_mhz = mt7996_tm_bw_mapping(chandef->width, BW_MAP_NL_TO_MHZ);
	offset = width_mhz / 10 - 2;

	for (i = 0; i < size; i++) {
		not_first = (chandef->width != NL80211_CHAN_WIDTH_160) ?
			    (i % bitmap) : (i >= 32) || !((1 << i) & bitmap);
		if (not_first)
			continue;

		if (control_chan >= chan[i].hw_value)
			first_control = chan[i].hw_value;
		else
			break;
	}

	if (first_control == 0)
		return control_chan;

	return first_control + offset;
}

static int
mt7996_tm_set_offchan(struct mt7996_phy *phy, bool no_center)
{
	struct mt76_phy *mphy = phy->mt76;
	struct mt7996_dev *dev = phy->dev;
	struct ieee80211_hw *hw = mphy->hw;
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct cfg80211_chan_def chandef = {};
	struct ieee80211_channel *chan;
	int ret, freq = ieee80211_channel_to_frequency(td->offchan_ch, NL80211_BAND_5GHZ);

	if (!mphy->cap.has_5ghz || !freq) {
		ret = -EINVAL;
		dev_info(dev->mt76.dev, "Failed to set offchan (invalid band or channel)!\n");
		goto out;
	}

	chandef.width = td->offchan_bw;
	chan = ieee80211_get_channel(hw->wiphy, freq);
	chandef.chan = chan;
	if (no_center)
		td->offchan_center_ch = mt7996_tm_get_center_chan(phy, &chandef);
	chandef.center_freq1 = ieee80211_channel_to_frequency(td->offchan_center_ch,
							      NL80211_BAND_5GHZ);
	if (!cfg80211_chandef_valid(&chandef)) {
		ret = -EINVAL;
		dev_info(dev->mt76.dev, "Failed to set offchan, chandef is invalid!\n");
		goto out;
	}

	memset(&dev->rdd2_chandef, 0, sizeof(struct cfg80211_chan_def));

	ret = mt7996_mcu_rdd_background_enable(phy, &chandef);

	if (ret)
		goto out;

	dev->rdd2_phy = phy;
	dev->rdd2_chandef = chandef;

	return 0;

out:
	td->offchan_ch = 0;
	td->offchan_center_ch = 0;
	td->offchan_bw = 0;

	return ret;
}

static void
mt7996_tm_ipi_hist_ctrl(struct mt7996_phy *phy, struct mt7996_tm_rdd_ipi_ctrl *data, u8 cmd)
{
#define MT_IPI_RESET		0x830a5dfc
#define MT_IPI_RESET_MASK	BIT(28)
#define MT_IPI_COUNTER_BASE	0x83041000
#define MT_IPI_COUNTER(idx)	(MT_IPI_COUNTER_BASE + ((idx) * 4))
	struct mt7996_dev *dev = phy->dev;
	bool val;
	int i;

	if (cmd == RDD_SET_IPI_HIST_RESET) {
		val = mt76_rr(dev, MT_IPI_RESET) & MT_IPI_RESET_MASK;
		mt76_rmw_field(dev, MT_IPI_RESET, MT_IPI_RESET_MASK, !val);
		return;
	}

	for (i = 0; i < POWER_INDICATE_HIST_MAX; i++)
		data->ipi_hist_val[i] = mt76_rr(dev, MT_IPI_COUNTER(i));
}

static void
mt7996_tm_ipi_work(struct work_struct *work)
{
#define PRECISION	100
	struct mt7996_phy *phy = container_of(work, struct mt7996_phy, ipi_work.work);
	struct mt7996_dev *dev = phy->dev;
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_tm_rdd_ipi_ctrl data;
	u32 ipi_idx, ipi_free_count, ipi_percentage;
	u32 ipi_hist_count_th = 0, ipi_hist_total_count = 0;
	u32 self_idle_ratio, ipi_idle_ratio, channel_load;
	u32 *ipi_hist_data;
	const char *power_lower_bound, *power_upper_bound;
	static const char * const ipi_idx_to_power_bound[] = {
		[RDD_IPI_HIST_0] = "-92",
		[RDD_IPI_HIST_1] = "-89",
		[RDD_IPI_HIST_2] = "-86",
		[RDD_IPI_HIST_3] = "-83",
		[RDD_IPI_HIST_4] = "-80",
		[RDD_IPI_HIST_5] = "-75",
		[RDD_IPI_HIST_6] = "-70",
		[RDD_IPI_HIST_7] = "-65",
		[RDD_IPI_HIST_8] = "-60",
		[RDD_IPI_HIST_9] = "-55",
		[RDD_IPI_HIST_10] = "inf",
	};

	memset(&data, 0, sizeof(data));
	mt7996_tm_ipi_hist_ctrl(phy, &data, RDD_IPI_HIST_ALL_CNT);

	ipi_hist_data = data.ipi_hist_val;
	for (ipi_idx = 0; ipi_idx < POWER_INDICATE_HIST_MAX; ipi_idx++) {
		power_lower_bound = ipi_idx ? ipi_idx_to_power_bound[ipi_idx - 1] : "-inf";
		power_upper_bound = ipi_idx_to_power_bound[ipi_idx];

		dev_info(dev->mt76.dev, "IPI %d (power range: (%s, %s] dBm): ipi count = %d\n",
			 ipi_idx, power_lower_bound, power_upper_bound, ipi_hist_data[ipi_idx]);

		if (td->ipi_threshold <= ipi_idx && ipi_idx <= RDD_IPI_HIST_10)
			ipi_hist_count_th += ipi_hist_data[ipi_idx];

		ipi_hist_total_count += ipi_hist_data[ipi_idx];
	}

	ipi_free_count = ipi_hist_data[RDD_IPI_FREE_RUN_CNT];

	dev_info(dev->mt76.dev, "IPI threshold %d: ipi_hist_count_th = %d, ipi_free_count = %d\n",
		 td->ipi_threshold, ipi_hist_count_th, ipi_free_count);
	dev_info(dev->mt76.dev, "TX assert time =  %d [ms]\n", data.tx_assert_time / 1000);

	/* calculate channel load = (self idle ratio - idle ratio) / self idle ratio */
	if (ipi_hist_count_th >= UINT_MAX / (100 * PRECISION))
		ipi_percentage = 100 * PRECISION *
				 (ipi_hist_count_th / (100 * PRECISION)) /
				 (ipi_free_count / (100 * PRECISION));
	else
		ipi_percentage = PRECISION * 100 * ipi_hist_count_th / ipi_free_count;

	ipi_idle_ratio = ((100 * PRECISION) - ipi_percentage) / PRECISION;

	self_idle_ratio = PRECISION * 100 *
			  (td->ipi_period - (data.tx_assert_time / 1000)) /
			  td->ipi_period / PRECISION;

	if (self_idle_ratio < ipi_idle_ratio)
		channel_load = 0;
	else
		channel_load = self_idle_ratio - ipi_idle_ratio;

	if (self_idle_ratio <= td->ipi_threshold) {
		dev_info(dev->mt76.dev, "band[%d]: self idle ratio = %d%%, idle ratio = %d%%\n",
			 phy->mt76->band_idx, self_idle_ratio, ipi_idle_ratio);
		return;
	}

	channel_load = (100 * channel_load) / self_idle_ratio;
	dev_info(dev->mt76.dev,
		 "band[%d]: chan load = %d%%, self idle ratio = %d%%, idle ratio = %d%%\n",
		 phy->mt76->band_idx, channel_load, self_idle_ratio, ipi_idle_ratio);
}

static int
mt7996_tm_set_ipi(struct mt7996_phy *phy)
{
	struct mt76_testmode_data *td = &phy->mt76->test;

	/* reset IPI CR */
	mt7996_tm_ipi_hist_ctrl(phy, NULL, RDD_SET_IPI_HIST_RESET);

	cancel_delayed_work(&phy->ipi_work);
	ieee80211_queue_delayed_work(phy->mt76->hw, &phy->ipi_work,
				     msecs_to_jiffies(td->ipi_period));

	return 0;
}

static int
mt7996_tm_set_trx_mac(struct mt7996_phy *phy, u8 type, bool en)
{
#define UNI_TM_TRX_CTRL 0
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_trx_req req = {
		.param_num = 1,
		.tag = cpu_to_le16(UNI_TM_TRX_CTRL),
		.len = cpu_to_le16(sizeof(req) - 4),
		.param_idx = cpu_to_le16(TM_TRX_PARAM_SET_TRX),
		.band_idx = phy->mt76->band_idx,
		.testmode_en = 1,
		.action = TM_TRX_ACTION_SET,
		.set_trx = {
			.type = type,
			.enable = en,
			.band_idx = phy->mt76->band_idx,
		}
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(TESTMODE_TRX_PARAM),
				 &req, sizeof(req), false);
}

static int
mt7996_tm_txbf_init(struct mt7996_phy *phy, u16 *val)
{
#define EBF_BBP_RX_OFFSET	0x10280
#define EBF_BBP_RX_ENABLE	(BIT(0) | BIT(15))
	struct mt7996_dev *dev = phy->dev;
	struct mt76_testmode_data *td = &phy->mt76->test;
	bool enable = val[0];
	void *phase_cal, *pfmu_data, *pfmu_tag;
	u8 nss, band_idx = phy->mt76->band_idx;
	enum nl80211_chan_width width = NL80211_CHAN_WIDTH_20;
	u8 sub_addr = td->is_txbf_dut ? TXBF_DUT_MAC_SUBADDR : TXBF_GOLDEN_MAC_SUBADDR;
	u8 peer_addr = td->is_txbf_dut ? TXBF_GOLDEN_MAC_SUBADDR : TXBF_DUT_MAC_SUBADDR;
	u8 bss_addr = TXBF_DUT_MAC_SUBADDR;
	u8 addr[ETH_ALEN] = {0x00, sub_addr, sub_addr, sub_addr, sub_addr, sub_addr};
	u8 bssid[ETH_ALEN] = {0x00, bss_addr, bss_addr, bss_addr, bss_addr, bss_addr};
	u8 peer_addrs[ETH_ALEN] = {0x00, peer_addr, peer_addr, peer_addr, peer_addr, peer_addr};
	struct mt7996_vif *mvif = (struct mt7996_vif *)phy->monitor_vif->drv_priv;

	if (!enable) {
		td->bf_en = false;
		return 0;
	}

	if (!dev->test.txbf_phase_cal) {
		phase_cal = devm_kzalloc(dev->mt76.dev,
					 sizeof(struct mt7996_txbf_phase) *
					 MAX_PHASE_GROUP_NUM,
					 GFP_KERNEL);
		if (!phase_cal)
			return -ENOMEM;

		dev->test.txbf_phase_cal = phase_cal;
	}

	if (!dev->test.txbf_pfmu_data) {
		pfmu_data = devm_kzalloc(dev->mt76.dev,
					 sizeof(struct mt7996_pfmu_data) *
					 MT7996_TXBF_SUBCAR_NUM,
					 GFP_KERNEL);
		if (!pfmu_data)
			return -ENOMEM;

		dev->test.txbf_pfmu_data = pfmu_data;
	}

	if (!dev->test.txbf_pfmu_tag) {
		pfmu_tag = devm_kzalloc(dev->mt76.dev,
					sizeof(struct mt7996_pfmu_tag), GFP_KERNEL);
		if (!pfmu_tag)
			return -ENOMEM;

		dev->test.txbf_pfmu_tag = pfmu_tag;
	}

	td->bf_en = true;
	dev->ibf = td->ibf;
	memcpy(td->addr[0], peer_addrs, ETH_ALEN);
	memcpy(td->addr[1], addr, ETH_ALEN);
	memcpy(td->addr[2], bssid, ETH_ALEN);
	memcpy(phy->monitor_vif->addr, addr, ETH_ALEN);
	mt7996_tm_set_mac_addr(dev, td->addr[0], SET_ID(DA));
	mt7996_tm_set_mac_addr(dev, td->addr[1], SET_ID(SA));
	mt7996_tm_set_mac_addr(dev, td->addr[2], SET_ID(BSSID));

	/* bss idx & omac idx should be set to band idx for ibf cal */
	mvif->mt76.idx = band_idx;
	dev->mt76.vif_mask |= BIT_ULL(mvif->mt76.idx);
	mvif->mt76.omac_idx = band_idx;
	phy->omac_mask |= BIT_ULL(mvif->mt76.omac_idx);

	mt7996_mcu_add_dev_info(phy, phy->monitor_vif, true);
	mt7996_mcu_add_bss_info(phy, phy->monitor_vif, true);

	if (td->ibf) {
		if (td->is_txbf_dut) {
			/* Enable ITxBF Capability */
			mt7996_mcu_set_txbf(dev, BF_HW_EN_UPDATE);
			mt7996_tm_set_trx_mac(phy, TM_TRX_MAC_TX, true);

			td->tx_ipg = 999;
			td->tx_mpdu_len = 1024;
			td->tx_antenna_mask = phy->mt76->chainmask >> dev->chainshift[band_idx];
			nss = hweight8(td->tx_antenna_mask);
			if (nss > 1 && nss <= 4)
				td->tx_rate_idx = 15 + 8 * (nss - 2);
			else
				td->tx_rate_idx = 31;
		} else {
			td->tx_antenna_mask = 1;
			td->tx_mpdu_len = 1024;
			td->tx_rate_idx = 0;
			mt76_set(dev, EBF_BBP_RX_OFFSET, EBF_BBP_RX_ENABLE);
			dev_info(dev->mt76.dev, "Set BBP RX CR = %x\n",
				 mt76_rr(dev, EBF_BBP_RX_OFFSET));
		}

		td->tx_rate_mode = MT76_TM_TX_MODE_HT;
		td->tx_rate_sgi = 0;
	} else {
		if (td->is_txbf_dut) {
			/* Enable ETxBF Capability */
			mt7996_mcu_set_txbf(dev, BF_HW_EN_UPDATE);
			td->tx_antenna_mask = phy->mt76->chainmask >> dev->chainshift[band_idx];
			td->tx_spe_idx = 24 + phy->mt76->band_idx;
			if (td->tx_rate_mode == MT76_TM_TX_MODE_VHT ||
			    td->tx_rate_mode == MT76_TM_TX_MODE_HE_SU)
				mt7996_tm_set(dev, SET_ID(NSS), td->tx_rate_nss);

			mt7996_tm_set(dev, SET_ID(ENCODE_MODE), td->tx_rate_ldpc);
			mt7996_tm_set(dev, SET_ID(TX_COUNT), td->tx_count);
		} else {
			/* Turn On BBP CR for RX */
			mt76_set(dev, EBF_BBP_RX_OFFSET, EBF_BBP_RX_ENABLE);
			dev_info(dev->mt76.dev, "Set BBP RX CR = %x\n",
				 mt76_rr(dev, EBF_BBP_RX_OFFSET));

			td->tx_antenna_mask = 1;
		}
		width = phy->mt76->chandef.width;

		if (td->tx_rate_mode == MT76_TM_TX_MODE_EHT_MU)
			td->tx_rate_mode = MT76_TM_TX_MODE_EHT_SU;
	}
	mt76_testmode_param_set(td, MT76_TM_ATTR_TX_ANTENNA);

	mt7996_tm_set(dev, SET_ID(TX_MODE),
		      mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_PHY));
	mt7996_tm_set(dev, SET_ID(TX_RATE), td->tx_rate_idx);
	mt7996_tm_set(dev, SET_ID(GI), td->tx_rate_sgi);
	mt7996_tm_set(dev, SET_ID(CBW),
		      mt7996_tm_bw_mapping(width, BW_MAP_NL_TO_FW));
	mt7996_tm_set(dev, SET_ID(DBW),
		      mt7996_tm_bw_mapping(width, BW_MAP_NL_TO_FW));
	mt7996_tm_set_antenna(phy, SET_ID(TX_PATH));
	mt7996_tm_set_antenna(phy, SET_ID(RX_PATH));
	mt7996_tm_set(dev, SET_ID(IPG), td->tx_ipg);
	mt7996_tm_set(dev, SET_ID(TX_LEN), td->tx_mpdu_len);
	mt7996_tm_set(dev, SET_ID(TX_TIME), 0);
	mt7996_tm_set(dev, SET_ID(COMMAND), RF_CMD(TX_COMMIT));

	return 0;
}

static int
mt7996_tm_txbf_phase_comp(struct mt7996_phy *phy, u16 *val)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_bf_req req = {
		.phase_comp = {
			.tag = cpu_to_le16(BF_IBF_PHASE_COMP),
			.len = cpu_to_le16(sizeof(req.phase_comp)),
			.bw = val[0],
			.jp_band = (val[2] == 1) ? 1 : 0,
			.band_idx = phy->mt76->band_idx,
			.read_from_e2p = val[3],
			.disable = val[4],
			.group = val[2],
		}
	};
	struct mt7996_txbf_phase *phase = (struct mt7996_txbf_phase *)dev->test.txbf_phase_cal;

	wait_event_timeout(dev->mt76.tx_wait, phase[val[2]].status != 0, HZ);
	if (val[2])
		memcpy(req.phase_comp.buf, &phase[val[2]].phase_5g, sizeof(req.phase_comp.buf));
	else
		memcpy(req.phase_comp.buf, &phase[val[2]].phase_2g, sizeof(req.phase_comp.buf));

	pr_info("ibf cal process: phase comp info\n");
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1,
		       &req, sizeof(req), 0);

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &req,
				 sizeof(req), false);
}

static int
mt7996_tm_txbf_profile_tag_write(struct mt7996_phy *phy, u8 pfmu_idx, struct mt7996_pfmu_tag *tag)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_bf_req req = {
		.pfmu_tag = {
			.tag = cpu_to_le16(BF_PFMU_TAG_WRITE),
			.len = cpu_to_le16(sizeof(req.pfmu_tag)),
			.pfmu_id = pfmu_idx,
			.bfer = true,
			.band_idx = phy->mt76->band_idx,
		}
	};

	memcpy(req.pfmu_tag.buf, tag, sizeof(*tag));
	wait_event_timeout(dev->mt76.tx_wait, tag->t1.pfmu_idx != 0, HZ);

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &req,
				 sizeof(req), false);
}

static int
mt7996_tm_add_txbf_sta(struct mt7996_phy *phy, u8 pfmu_idx, u8 nr, u8 nc, bool ebf)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct {
		struct sta_req_hdr hdr;
		struct sta_rec_bf bf;
	} __packed req = {
		.hdr = {
			.bss_idx = phy->mt76->band_idx,
			.wlan_idx_lo = to_wcid_lo(phy->mt76->band_idx + 1),
			.tlv_num = 1,
			.is_tlv_append = 1,
			.muar_idx = 0,
			.wlan_idx_hi = to_wcid_hi(phy->mt76->band_idx + 1),
		},
		.bf = {
			.tag = cpu_to_le16(STA_REC_BF),
			.len = cpu_to_le16(sizeof(req.bf)),
			.pfmu = cpu_to_le16(pfmu_idx),
			.sounding_phy = 1,
			.bf_cap = ebf,
			.ncol = nc,
			.nrow = nr,
			.ibf_timeout = 0xff,
			.tx_mode = mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_PHY),
		},
	};
	u8 ndp_rate, ndpa_rate, rept_poll_rate, bf_bw;

	if (td->tx_rate_mode == MT76_TM_TX_MODE_HE_SU ||
	    td->tx_rate_mode == MT76_TM_TX_MODE_EHT_SU) {
		rept_poll_rate = 0x49;
		ndpa_rate = 0x49;
		ndp_rate = 0;
	} else if (td->tx_rate_mode == MT76_TM_TX_MODE_VHT) {
		rept_poll_rate = 0x9;
		ndpa_rate = 0x9;
		ndp_rate = 0;
	} else {
		rept_poll_rate = 0;
		ndpa_rate = 0;
		if (nr == 1)
			ndp_rate = 8;
		else if (nr == 2)
			ndp_rate = 16;
		else
			ndp_rate = 24;
	}

	bf_bw = mt7996_tm_bw_mapping(phy->mt76->chandef.width, BW_MAP_NL_TO_BF);
	req.bf.ndp_rate = ndp_rate;
	req.bf.ndpa_rate = ndpa_rate;
	req.bf.rept_poll_rate = rept_poll_rate;
	req.bf.bw = bf_bw;
	req.bf.tx_mode = (td->tx_rate_mode == MT76_TM_TX_MODE_EHT_SU) ? 0xf : req.bf.tx_mode;

	if (ebf) {
		req.bf.mem[0].row = 0;
		req.bf.mem[1].row = 1;
		req.bf.mem[2].row = 2;
		req.bf.mem[3].row = 3;
	} else {
		req.bf.mem[0].row = 4;
		req.bf.mem[1].row = 5;
		req.bf.mem[2].row = 6;
		req.bf.mem[3].row = 7;
	}

	return mt76_mcu_send_msg(&dev->mt76, MCU_WMWA_UNI_CMD(STA_REC_UPDATE), &req,
				 sizeof(req), true);
}

static int
mt7996_tm_txbf_profile_update(struct mt7996_phy *phy, u16 *val, bool ebf)
{
#define MT_ARB_IBF_ENABLE			(BIT(0) | GENMASK(9, 8))
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_pfmu_tag *tag = dev->test.txbf_pfmu_tag;
	u8 pfmu_idx = val[0], nc = val[2], nr;
	int ret;
	bool is_atenl = val[5];

	if (td->tx_antenna_mask == 3)
		nr = 1;
	else if (td->tx_antenna_mask == 7)
		nr = 2;
	else
		nr = 3;

	memset(tag, 0, sizeof(*tag));
	tag->t1.pfmu_idx = pfmu_idx;
	tag->t1.ebf = ebf;
	tag->t1.nr = nr;
	tag->t1.nc = nc;
	tag->t1.invalid_prof = true;
	tag->t1.data_bw = mt7996_tm_bw_mapping(phy->mt76->chandef.width, BW_MAP_NL_TO_BF);
	tag->t2.se_idx = td->tx_spe_idx;

	if (ebf) {
		tag->t1.row_id1 = 0;
		tag->t1.row_id2 = 1;
		tag->t1.row_id3 = 2;
		tag->t1.row_id4 = 3;
		tag->t1.lm = mt7996_tm_rate_mapping(td->tx_rate_mode, RATE_MODE_TO_LM);
	} else {
		tag->t1.row_id1 = 4;
		tag->t1.row_id2 = 5;
		tag->t1.row_id3 = 6;
		tag->t1.row_id4 = 7;
		tag->t1.lm = mt7996_tm_rate_mapping(MT76_TM_TX_MODE_OFDM, RATE_MODE_TO_LM);

		tag->t2.ibf_timeout = 0xff;
		tag->t2.ibf_nr = nr;
		tag->t2.ibf_nc = nc;
	}

	ret = mt7996_tm_txbf_profile_tag_write(phy, pfmu_idx, tag);
	if (ret)
		return ret;

	ret = mt7996_tm_add_txbf_sta(phy, pfmu_idx, nr, nc, ebf);
	if (ret)
		return ret;

	if (!is_atenl && !td->ibf) {
		mt76_set(dev, MT_ARB_TQSAXM0(phy->mt76->band_idx), MT_ARB_TQSAXM_ALTX_START_MASK);
		dev_info(dev->mt76.dev, "Set TX queue start CR for AX management (0x%x) = 0x%x\n",
			 MT_ARB_TQSAXM0(phy->mt76->band_idx),
			 mt76_rr(dev, MT_ARB_TQSAXM0(phy->mt76->band_idx)));
	} else if (!is_atenl && td->ibf && ebf) {
		/* iBF's ebf profile update */
		mt76_set(dev, MT_ARB_TQSAXM0(phy->mt76->band_idx), MT_ARB_IBF_ENABLE);
		dev_info(dev->mt76.dev, "Set TX queue start CR for AX management (0x%x) = 0x%x\n",
			 MT_ARB_TQSAXM0(phy->mt76->band_idx),
			 mt76_rr(dev, MT_ARB_TQSAXM0(phy->mt76->band_idx)));
	}

	if (!ebf && is_atenl)
		return mt7996_tm_txbf_apply_tx(phy, 1, false, true, true);

	return 0;
}

static int
mt7996_tm_txbf_phase_cal(struct mt7996_phy *phy, u16 *val)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_bf_req req = {
		.phase_cal = {
			.tag = cpu_to_le16(BF_PHASE_CALIBRATION),
			.len = cpu_to_le16(sizeof(req.phase_cal)),
			.group = val[0],
			.group_l_m_n = val[1],
			.sx2 = val[2],
			.cal_type = val[3],
			.lna_gain_level = val[4],
			.band_idx = phy->mt76->band_idx,
		},
	};
	struct mt7996_txbf_phase *phase = (struct mt7996_txbf_phase *)dev->test.txbf_phase_cal;

	/* reset phase status before update phase cal data */
	phase[req.phase_cal.group].status = 0;

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &req,
				 sizeof(req), false);
}

static int
mt7996_tm_txbf_profile_update_all(struct mt7996_phy *phy, u16 *val)
{
#define MT7996_TXBF_PFMU_DATA_LEN	(MT7996_TXBF_SUBCAR_NUM * sizeof(struct mt7996_pfmu_data))
	struct mt76_testmode_data *td = &phy->mt76->test;
	u8 nss = hweight8(td->tx_antenna_mask);
	u16 pfmu_idx = val[0];
	u16 subc_id = val[1];
	u16 angle11 = val[2];
	u16 angle21 = val[3];
	u16 angle31 = val[4];
	u16 angle41 = val[5];
	s16 phi11 = 0, phi21 = 0, phi31 = 0;
	struct mt7996_pfmu_data *pfmu_data;

	if (subc_id > MT7996_TXBF_SUBCAR_NUM - 1)
		return -EINVAL;

	if (nss == 2) {
		phi11 = (s16)(angle21 - angle11);
	} else if (nss == 3) {
		phi11 = (s16)(angle31 - angle11);
		phi21 = (s16)(angle31 - angle21);
	} else {
		phi11 = (s16)(angle41 - angle11);
		phi21 = (s16)(angle41 - angle21);
		phi31 = (s16)(angle41 - angle31);
	}

	pfmu_data = (struct mt7996_pfmu_data *)phy->dev->test.txbf_pfmu_data;
	pfmu_data = &pfmu_data[subc_id];

	if (subc_id < 32)
		pfmu_data->subc_idx = cpu_to_le16(subc_id + 224);
	else
		pfmu_data->subc_idx = cpu_to_le16(subc_id - 32);

	pfmu_data->phi11 = cpu_to_le16(phi11);
	pfmu_data->phi21 = cpu_to_le16(phi21);
	pfmu_data->phi31 = cpu_to_le16(phi31);
	if (subc_id == MT7996_TXBF_SUBCAR_NUM - 1) {
		struct mt7996_dev *dev = phy->dev;
		struct mt7996_tm_bf_req req = {
			.pfmu_data_all = {
				.tag = cpu_to_le16(BF_PROFILE_WRITE_20M_ALL),
				.len = cpu_to_le16(sizeof(req.pfmu_data_all)),
				.pfmu_id = pfmu_idx,
				.band_idx = phy->mt76->band_idx,
			},
		};

		memcpy(req.pfmu_data_all.buf, dev->test.txbf_pfmu_data, MT7996_TXBF_PFMU_DATA_LEN);

		return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF),
					 &req, sizeof(req), true);
	}

	return 0;
}

static int
mt7996_tm_txbf_e2p_update(struct mt7996_phy *phy)
{
#define TXBF_PHASE_EEPROM_START_OFFSET		0xc00
#define TXBF_PHASE_GROUP_EEPROM_OFFSET		0x2e
	struct mt7996_txbf_phase *phase, *p;
	struct mt7996_dev *dev = phy->dev;
	u8 *eeprom = dev->mt76.eeprom.data;
	u16 offset;
	int i;

	offset = TXBF_PHASE_EEPROM_START_OFFSET;
	phase = (struct mt7996_txbf_phase *)dev->test.txbf_phase_cal;
	for (i = 0; i < MAX_PHASE_GROUP_NUM; i++) {
		p = &phase[i];

		if (!p->status)
			continue;

		/* copy phase cal data to eeprom */
		if (i)
			memcpy(eeprom + offset, &p->phase_5g, sizeof(p->phase_5g));
		else
			memcpy(eeprom + offset, &p->phase_2g, sizeof(p->phase_2g));
		offset += TXBF_PHASE_GROUP_EEPROM_OFFSET;
	}

	return 0;
}

static int
mt7996_tm_txbf_apply_tx(struct mt7996_phy *phy, u16 wlan_idx, bool ebf,
			bool ibf, bool phase_cal)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_bf_req req = {
		.tx_apply = {
			.tag = cpu_to_le16(BF_DATA_PACKET_APPLY),
			.len = cpu_to_le16(sizeof(req.tx_apply)),
			.wlan_idx = cpu_to_le16(wlan_idx),
			.ebf = ebf,
			.ibf = ibf,
			.phase_cal = phase_cal,
		},
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &req, sizeof(req), false);
}

static int
mt7996_tm_txbf_set_tx(struct mt7996_phy *phy, u16 *val)
{
	bool bf_on = val[0], update = val[3];
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_pfmu_tag *tag = dev->test.txbf_pfmu_tag;
	struct mt76_testmode_data *td = &phy->mt76->test;

	if (bf_on) {
		mt7996_tm_set_rx_frames(phy, false);
		mt7996_tm_set_tx_frames(phy, false);
		mt7996_mcu_set_txbf_internal(phy, BF_PFMU_TAG_READ, 2, true);
		tag->t1.invalid_prof = false;
		mt7996_tm_txbf_profile_tag_write(phy, 2, tag);
		td->bf_ever_en = true;

		if (update)
			mt7996_tm_txbf_apply_tx(phy, 1, 0, 1, 1);
	} else {
		if (!td->bf_ever_en) {
			mt7996_tm_set_rx_frames(phy, false);
			mt7996_tm_set_tx_frames(phy, false);
			td->ibf = false;
			td->ebf = false;

			if (update)
				mt7996_tm_txbf_apply_tx(phy, 1, 0, 0, 0);
		} else {
			td->bf_ever_en = false;

			mt7996_mcu_set_txbf_internal(phy, BF_PFMU_TAG_READ, 2, true);
			tag->t1.invalid_prof = true;
			mt7996_tm_txbf_profile_tag_write(phy, 2, tag);
		}
	}

	return 0;
}

static int
mt7996_tm_trigger_sounding(struct mt7996_phy *phy, u16 *val, bool en)
{
	struct mt7996_dev *dev = phy->dev;
	u8 sounding_mode = val[0];
	u8 sta_num = val[1];
	u32 sounding_interval = (u32)val[2] << 2;	/* input unit: 4ms */
	u16 tag = en ? BF_SOUNDING_ON : BF_SOUNDING_OFF;
	struct mt7996_tm_bf_req req = {
		.sounding = {
			.tag = cpu_to_le16(tag),
			.len = cpu_to_le16(sizeof(req.sounding)),
			.snd_mode = sounding_mode,
			.sta_num = sta_num,
			.wlan_id = {
				cpu_to_le16(val[3]),
				cpu_to_le16(val[4]),
				cpu_to_le16(val[5]),
				cpu_to_le16(val[6])
			},
			.snd_period = cpu_to_le32(sounding_interval),
		},
	};

	if (sounding_mode > SOUNDING_MODE_MAX)
		return -EINVAL;

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF),
				 &req, sizeof(req), false);
}

static int
mt7996_tm_txbf_txcmd(struct mt7996_phy *phy, u16 *val)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_bf_req req = {
		.txcmd = {
			.tag = cpu_to_le16(BF_CMD_TXCMD),
			.len = cpu_to_le16(sizeof(req.txcmd)),
			.action = val[0],
			.bf_manual = val[1],
			.bf_bit = val[2],
		},
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &req, sizeof(req), false);
}

static int
mt7996_tm_set_txbf(struct mt7996_phy *phy)
{
#define TXBF_IS_DUT_MASK	BIT(0)
#define TXBF_IBF_MASK		BIT(1)
	struct mt76_testmode_data *td = &phy->mt76->test;
	u16 *val = td->txbf_param;

	dev_info(phy->dev->mt76.dev,
		 "ibf cal process: act = %u, val = %u, %u, %u, %u, %u, %u, %u, %u\n",
		 td->txbf_act, val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7]);

	switch (td->txbf_act) {
	case MT76_TM_TXBF_ACT_GOLDEN_INIT:
	case MT76_TM_TXBF_ACT_INIT:
	case MT76_TM_TX_EBF_ACT_GOLDEN_INIT:
	case MT76_TM_TX_EBF_ACT_INIT:
		td->ibf = !u32_get_bits(td->txbf_act, TXBF_IBF_MASK);
		td->ebf = true;
		td->is_txbf_dut = !!u32_get_bits(td->txbf_act, TXBF_IS_DUT_MASK);
		return mt7996_tm_txbf_init(phy, val);
	case MT76_TM_TXBF_ACT_UPDATE_CH:
		mt7996_tm_update_channel(phy);
		break;
	case MT76_TM_TXBF_ACT_PHASE_COMP:
		return mt7996_tm_txbf_phase_comp(phy, val);
	case MT76_TM_TXBF_ACT_TX_PREP:
		return mt7996_tm_txbf_set_tx(phy, val);
	case MT76_TM_TXBF_ACT_IBF_PROF_UPDATE:
		return mt7996_tm_txbf_profile_update(phy, val, false);
	case MT76_TM_TXBF_ACT_EBF_PROF_UPDATE:
		return mt7996_tm_txbf_profile_update(phy, val, true);
	case MT76_TM_TXBF_ACT_PHASE_CAL:
		return mt7996_tm_txbf_phase_cal(phy, val);
	case MT76_TM_TXBF_ACT_PROF_UPDATE_ALL_CMD:
	case MT76_TM_TXBF_ACT_PROF_UPDATE_ALL:
		return mt7996_tm_txbf_profile_update_all(phy, val);
	case MT76_TM_TXBF_ACT_E2P_UPDATE:
		return mt7996_tm_txbf_e2p_update(phy);
	case MT76_TM_TXBF_ACT_APPLY_TX: {
		u16 wlan_idx = val[0];
		bool ebf = !!val[1], ibf = !!val[2], phase_cal = !!val[4];

		return mt7996_tm_txbf_apply_tx(phy, wlan_idx, ebf, ibf, phase_cal);
	}
	case MT76_TM_TXBF_ACT_TRIGGER_SOUNDING:
		return mt7996_tm_trigger_sounding(phy, val, true);
	case MT76_TM_TXBF_ACT_STOP_SOUNDING:
		memset(val, 0, sizeof(td->txbf_param));
		return mt7996_tm_trigger_sounding(phy, val, false);
	case MT76_TM_TXBF_ACT_PROFILE_TAG_READ:
	case MT76_TM_TXBF_ACT_PROFILE_TAG_WRITE:
	case MT76_TM_TXBF_ACT_PROFILE_TAG_INVALID: {
		u8 pfmu_idx = val[0];
		bool bfer = !!val[1];
		struct mt7996_dev *dev = phy->dev;
		struct mt7996_pfmu_tag *tag = dev->test.txbf_pfmu_tag;

		if (!tag) {
			dev_err(dev->mt76.dev,
				"pfmu tag is not initialized!\n");
			return 0;
		}

		if (td->txbf_act == MT76_TM_TXBF_ACT_PROFILE_TAG_WRITE)
			return mt7996_tm_txbf_profile_tag_write(phy, pfmu_idx, tag);
		else if (td->txbf_act == MT76_TM_TXBF_ACT_PROFILE_TAG_READ)
			return mt7996_mcu_set_txbf_internal(phy, BF_PFMU_TAG_READ, pfmu_idx, bfer);

		tag->t1.invalid_prof = !!val[0];

		return 0;
	}
	case MT76_TM_TXBF_ACT_STA_REC_READ:
		return mt7996_mcu_set_txbf_internal(phy, BF_STA_REC_READ, val[0], 0);
	case MT76_TM_TXBF_ACT_TXCMD:
		return mt7996_tm_txbf_txcmd(phy, val);
	default:
		break;
	};

	return 0;
}

static void
mt7996_tm_update_params(struct mt7996_phy *phy, u32 changed)
{
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_dev *dev = phy->dev;

	if (changed & BIT(TM_CHANGED_FREQ_OFFSET)) {
		mt7996_tm_set(dev, SET_ID(FREQ_OFFSET), td->freq_offset);
		mt7996_tm_set(dev, SET_ID(FREQ_OFFSET_C2), td->freq_offset);
	}
	if (changed & BIT(TM_CHANGED_TXPOWER))
		mt7996_tm_set(dev, SET_ID(POWER), td->tx_power[0]);
	if (changed & BIT(TM_CHANGED_SKU_EN)) {
		mt7996_tm_update_channel(phy);
		mt7996_mcu_set_tx_power_ctrl(phy, POWER_CTRL(SKU_POWER_LIMIT), td->sku_en);
		mt7996_mcu_set_tx_power_ctrl(phy, POWER_CTRL(BACKOFF_POWER_LIMIT), td->sku_en);
		mt7996_mcu_set_txpower_sku(phy);
	}
	if (changed & BIT(TM_CHANGED_TX_LENGTH)) {
		mt7996_tm_set(dev, SET_ID(TX_LEN), td->tx_mpdu_len);
		mt7996_tm_set(dev, SET_ID(TX_TIME), 0);
	}
	if (changed & BIT(TM_CHANGED_TX_TIME)) {
		mt7996_tm_set(dev, SET_ID(TX_LEN), 0);
		mt7996_tm_set(dev, SET_ID(TX_TIME), td->tx_time);
	}
	if (changed & BIT(TM_CHANGED_CFG)) {
		u32 func_idx = td->cfg.enable ? SET_ID(CFG_ON) : SET_ID(CFG_OFF);

		mt7996_tm_set(dev, func_idx, td->cfg.type);
	}
	if ((changed & BIT(TM_CHANGED_OFF_CHAN_CH)) &&
	    (changed & BIT(TM_CHANGED_OFF_CHAN_BW)))
		mt7996_tm_set_offchan(phy, !(changed & BIT(TM_CHANGED_OFF_CHAN_CENTER_CH)));
	if ((changed & BIT(TM_CHANGED_IPI_THRESHOLD)) &&
	    (changed & BIT(TM_CHANGED_IPI_PERIOD)))
		mt7996_tm_set_ipi(phy);
	if (changed & BIT(TM_CHANGED_IPI_RESET))
		mt7996_tm_ipi_hist_ctrl(phy, NULL, RDD_SET_IPI_HIST_RESET);
	if (changed & BIT(TM_CHANGED_TXBF_ACT))
		mt7996_tm_set_txbf(phy);
}

static int
mt7996_tm_set_state(struct mt76_phy *mphy, enum mt76_testmode_state state)
{
	struct mt76_testmode_data *td = &mphy->test;
	struct mt7996_phy *phy = mphy->priv;
	enum mt76_testmode_state prev_state = td->state;

	mphy->test.state = state;

	if (prev_state != MT76_TM_STATE_OFF)
		mt7996_tm_set(phy->dev, SET_ID(BAND_IDX), mphy->band_idx);

	if (prev_state == MT76_TM_STATE_TX_FRAMES ||
	    state == MT76_TM_STATE_TX_FRAMES)
		mt7996_tm_set_tx_frames(phy, state == MT76_TM_STATE_TX_FRAMES);
	else if (prev_state == MT76_TM_STATE_RX_FRAMES ||
		 state == MT76_TM_STATE_RX_FRAMES)
		mt7996_tm_set_rx_frames(phy, state == MT76_TM_STATE_RX_FRAMES);
	else if (prev_state == MT76_TM_STATE_TX_CONT ||
		 state == MT76_TM_STATE_TX_CONT)
		mt7996_tm_set_tx_cont(phy, state == MT76_TM_STATE_TX_CONT);
	else if (prev_state == MT76_TM_STATE_OFF ||
		 state == MT76_TM_STATE_OFF)
		mt7996_tm_init(phy, !(state == MT76_TM_STATE_OFF));
	else if (state >= MT76_TM_STATE_GROUP_PREK && state <= MT76_TM_STATE_GROUP_PREK_CLEAN)
		return mt7996_tm_group_prek(phy, state);
	else if (state >= MT76_TM_STATE_DPD_2G && state <= MT76_TM_STATE_DPD_CLEAN)
		return mt7996_tm_dpd_prek(phy, state);

	if ((state == MT76_TM_STATE_IDLE &&
	     prev_state == MT76_TM_STATE_OFF) ||
	    (state == MT76_TM_STATE_OFF &&
	     prev_state == MT76_TM_STATE_IDLE)) {
		u32 changed = 0;
		int i, ret;

		for (i = 0; i < ARRAY_SIZE(tm_change_map); i++) {
			u16 cur = tm_change_map[i];

			if (mt76_testmode_param_present(td, cur))
				changed |= BIT(i);
		}

		ret = mt7996_tm_check_antenna(phy);
		if (ret)
			return ret;

		mt7996_tm_update_params(phy, changed);
	}

	return 0;
}

static int
mt7996_tm_set_params(struct mt76_phy *mphy, struct nlattr **tb,
		     enum mt76_testmode_state new_state)
{
	struct mt76_testmode_data *td = &mphy->test;
	struct mt7996_phy *phy = mphy->priv;
	struct mt7996_dev *dev = phy->dev;
	u32 changed = 0;
	int i, ret;

	BUILD_BUG_ON(NUM_TM_CHANGED >= 32);

	if (new_state == MT76_TM_STATE_OFF ||
	    td->state == MT76_TM_STATE_OFF)
		return 0;

	ret = mt7996_tm_check_antenna(phy);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(tm_change_map); i++) {
		if (tb[tm_change_map[i]])
			changed |= BIT(i);
	}

	mt7996_tm_set(dev, SET_ID(BAND_IDX), mphy->band_idx);
	mt7996_tm_update_params(phy, changed);

	return 0;
}

static int
mt7996_tm_get_rx_stats(struct mt7996_phy *phy)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_tm_rx_req req = {
		.band = phy->mt76->band_idx,
		.rx_stat_all = {
			.tag = cpu_to_le16(UNI_TM_RX_STAT_GET_ALL_V2),
			.len = cpu_to_le16(sizeof(req.rx_stat_all)),
			.band_idx = phy->mt76->band_idx,
		},
	};
	struct mt76_testmode_data *td = &phy->mt76->test;
	struct mt7996_tm_rx_event *rx_stats;
	struct mt7996_tm_rx_event_stat_all *rx_stats_all;
	struct sk_buff *skb;
	enum mt76_rxq_id qid;
	int i, ret = 0;
	u32 mac_rx_mdrdy_cnt;
	u16 mac_rx_len_mismatch, fcs_err_count;

	if (td->state != MT76_TM_STATE_RX_FRAMES)
		return 0;

	ret = mt76_mcu_send_and_get_msg(&dev->mt76, MCU_WM_UNI_CMD_QUERY(TESTMODE_RX_STAT),
					&req, sizeof(req), true, &skb);

	if (ret)
		return ret;

	rx_stats = (struct mt7996_tm_rx_event *)skb->data;
	rx_stats_all = &rx_stats->rx_stat_all;

	phy->test.last_freq_offset = le32_to_cpu(rx_stats_all->user_info[0].freq_offset);
	phy->test.last_snr = le32_to_cpu(rx_stats_all->user_info[0].snr);
	for (i = 0; i < ARRAY_SIZE(phy->test.last_rcpi); i++) {
		phy->test.last_rcpi[i] = le16_to_cpu(rx_stats_all->rxv_info[i].rcpi);
		phy->test.last_rssi[i] = le16_to_cpu(rx_stats_all->rxv_info[i].rssi);
		phy->test.last_ib_rssi[i] = rx_stats_all->fagc[i].ib_rssi;
		phy->test.last_wb_rssi[i] = rx_stats_all->fagc[i].wb_rssi;
	}

	if (phy->mt76->band_idx == 2)
		qid = MT_RXQ_BAND2;
	else if (phy->mt76->band_idx == 1)
		qid = MT_RXQ_BAND1;
	else
		qid = MT_RXQ_MAIN;

	fcs_err_count = le16_to_cpu(rx_stats_all->band_info.mac_rx_fcs_err_cnt);
	mac_rx_len_mismatch = le16_to_cpu(rx_stats_all->band_info.mac_rx_len_mismatch);
	mac_rx_mdrdy_cnt = le32_to_cpu(rx_stats_all->band_info.mac_rx_mdrdy_cnt);
	td->rx_stats.packets[qid] += mac_rx_mdrdy_cnt;
	td->rx_stats.packets[qid] += fcs_err_count;
	td->rx_stats.fcs_error[qid] += fcs_err_count;
	td->rx_stats.len_mismatch += mac_rx_len_mismatch;

	dev_kfree_skb(skb);

	return ret;
}

static void
mt7996_tm_reset_trx_stats(struct mt76_phy *mphy)
{
	struct mt7996_phy *phy = mphy->priv;
	struct mt7996_dev *dev = phy->dev;

	memset(&mphy->test.rx_stats, 0, sizeof(mphy->test.rx_stats));
	mt7996_tm_set(dev, SET_ID(TRX_COUNTER_RESET), 0);
}

static int
mt7996_tm_get_tx_stats(struct mt7996_phy *phy)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt76_testmode_data *td = &phy->mt76->test;
	int ret;

	if (td->state != MT76_TM_STATE_TX_FRAMES)
		return 0;

	ret = mt7996_tm_get(dev, GET_ID(TXED_COUNT), 0, &td->tx_done);
	if (ret)
		return ret;

	td->tx_pending = td->tx_count - td->tx_done;

	return ret;
}

static int
mt7996_tm_dump_stats(struct mt76_phy *mphy, struct sk_buff *msg)
{
	struct mt7996_phy *phy = mphy->priv;
	void *rx, *rssi;
	int i;

	mt7996_tm_set(phy->dev, SET_ID(BAND_IDX), mphy->band_idx);
	mt7996_tm_get_rx_stats(phy);
	mt7996_tm_get_tx_stats(phy);

	rx = nla_nest_start(msg, MT76_TM_STATS_ATTR_LAST_RX);
	if (!rx)
		return -ENOMEM;

	if (nla_put_s32(msg, MT76_TM_RX_ATTR_FREQ_OFFSET, phy->test.last_freq_offset))
		return -ENOMEM;

	rssi = nla_nest_start(msg, MT76_TM_RX_ATTR_RCPI);
	if (!rssi)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(phy->test.last_rcpi); i++)
		if (nla_put_u8(msg, i, phy->test.last_rcpi[i]))
			return -ENOMEM;

	nla_nest_end(msg, rssi);

	rssi = nla_nest_start(msg, MT76_TM_RX_ATTR_RSSI);
	if (!rssi)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(phy->test.last_rssi); i++)
		if (nla_put_s8(msg, i, phy->test.last_rssi[i]))
			return -ENOMEM;

	nla_nest_end(msg, rssi);

	rssi = nla_nest_start(msg, MT76_TM_RX_ATTR_IB_RSSI);
	if (!rssi)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(phy->test.last_ib_rssi); i++)
		if (nla_put_s8(msg, i, phy->test.last_ib_rssi[i]))
			return -ENOMEM;

	nla_nest_end(msg, rssi);

	rssi = nla_nest_start(msg, MT76_TM_RX_ATTR_WB_RSSI);
	if (!rssi)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(phy->test.last_wb_rssi); i++)
		if (nla_put_s8(msg, i, phy->test.last_wb_rssi[i]))
			return -ENOMEM;

	nla_nest_end(msg, rssi);

	if (nla_put_u8(msg, MT76_TM_RX_ATTR_SNR, phy->test.last_snr))
		return -ENOMEM;

	nla_nest_end(msg, rx);

	return 0;
}

static int
mt7996_tm_write_back_to_efuse(struct mt7996_dev *dev)
{
	struct mt7996_mcu_eeprom_info req = {
		.tag = cpu_to_le16(UNI_EFUSE_ACCESS),
		.len = cpu_to_le16(sizeof(req) - 4),
	};
	u8 read_buf[MT76_TM_EEPROM_BLOCK_SIZE], *eeprom = dev->mt76.eeprom.data;
	int i, ret = -EINVAL;

	/* prevent from damaging chip id in efuse */
	if (mt76_chip(&dev->mt76) != get_unaligned_le16(eeprom))
		goto out;

	for (i = 0; i < MT7996_EEPROM_SIZE; i += MT76_TM_EEPROM_BLOCK_SIZE) {
		req.addr = cpu_to_le32(i);
		memcpy(req.data, eeprom + i, MT76_TM_EEPROM_BLOCK_SIZE);

		ret = mt7996_mcu_get_eeprom(dev, i, read_buf);
		if (ret < 0)
			return ret;

		if (!memcmp(req.data, read_buf, MT76_TM_EEPROM_BLOCK_SIZE))
			continue;

		ret = mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(EFUSE_CTRL),
					&req, sizeof(req), true);
		if (ret)
			return ret;
	}

out:
	return ret;
}

static int
mt7996_tm_set_eeprom(struct mt76_phy *mphy, u32 offset, u8 *val, u8 action)
{
	struct mt7996_phy *phy = mphy->priv;
	struct mt7996_dev *dev = phy->dev;
	u8 *eeprom = dev->mt76.eeprom.data;
	int ret = 0;

	if (offset >= MT7996_EEPROM_SIZE)
		return -EINVAL;

	switch (action) {
	case MT76_TM_EEPROM_ACTION_UPDATE_DATA:
		memcpy(eeprom + offset, val, MT76_TM_EEPROM_BLOCK_SIZE);
		break;
	case MT76_TM_EEPROM_ACTION_UPDATE_BUFFER_MODE:
		ret = mt7996_mcu_set_eeprom(dev);
		break;
	case MT76_TM_EEPROM_ACTION_WRITE_TO_EFUSE:
		ret = mt7996_tm_write_back_to_efuse(dev);
		break;
	default:
		break;
	}

	return ret;
}

const struct mt76_testmode_ops mt7996_testmode_ops = {
	.set_state = mt7996_tm_set_state,
	.set_params = mt7996_tm_set_params,
	.dump_stats = mt7996_tm_dump_stats,
	.reset_rx_stats = mt7996_tm_reset_trx_stats,
	.tx_stop = mt7996_tm_tx_stop,
	.set_eeprom = mt7996_tm_set_eeprom,
	.dump_precal = mt7996_tm_dump_precal,
};
