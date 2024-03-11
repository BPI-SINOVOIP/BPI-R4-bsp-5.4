// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2020, MediaTek Inc. All rights reserved.
 */

#include <net/netlink.h>

#include "mt7996.h"
#include "mcu.h"
#include "vendor.h"
#include "mtk_mcu.h"

#ifdef CONFIG_MTK_VENDOR
static const struct nla_policy
mu_ctrl_policy[NUM_MTK_VENDOR_ATTRS_MU_CTRL] = {
	[MTK_VENDOR_ATTR_MU_CTRL_ONOFF] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_MU_CTRL_DUMP] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_MU_CTRL_STRUCT] = {.type = NLA_BINARY },
};

static const struct nla_policy
wireless_ctrl_policy[NUM_MTK_VENDOR_ATTRS_WIRELESS_CTRL] = {
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_AMSDU] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_AMPDU] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_CERT] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_RTS_SIGTA] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_FIXED_MCS] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_OFDMA] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_PPDU_TX_TYPE] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_NUSERS_OFDMA] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_MIMO] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_WIRELESS_CTRL_BA_BUFFER_SIZE] = {.type = NLA_U16 },
};

static const struct nla_policy
wireless_dump_policy[NUM_MTK_VENDOR_ATTRS_WIRELESS_DUMP] = {
	[MTK_VENDOR_ATTR_WIRELESS_DUMP_AMSDU] = { .type = NLA_U8 },
};

static const struct nla_policy
amnt_ctrl_policy[NUM_MTK_VENDOR_ATTRS_AMNT_CTRL] = {
	[MTK_VENDOR_ATTR_AMNT_CTRL_SET] = {.type = NLA_NESTED },
	[MTK_VENDOR_ATTR_AMNT_CTRL_DUMP] = { .type = NLA_NESTED },
};

static const struct nla_policy
amnt_set_policy[NUM_MTK_VENDOR_ATTRS_AMNT_SET] = {
	[MTK_VENDOR_ATTR_AMNT_SET_INDEX] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_AMNT_SET_MACADDR] = NLA_POLICY_EXACT_LEN_WARN(ETH_ALEN),
};

static const struct nla_policy
amnt_dump_policy[NUM_MTK_VENDOR_ATTRS_AMNT_DUMP] = {
	[MTK_VENDOR_ATTR_AMNT_DUMP_INDEX] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_AMNT_DUMP_LEN] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_AMNT_DUMP_RESULT] = { .type = NLA_NESTED },
};

static struct nla_policy
bss_color_ctrl_policy[NUM_MTK_VENDOR_ATTRS_BSS_COLOR_CTRL] = {
	[MTK_VENDOR_ATTR_AVAL_BSS_COLOR_BMP] = { .type = NLA_U64 },
};

static const struct nla_policy
edcca_ctrl_policy[NUM_MTK_VENDOR_ATTRS_EDCCA_CTRL] = {
	[MTK_VENDOR_ATTR_EDCCA_CTRL_MODE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_PRI20_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC20_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC40_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC80_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_COMPENSATE] = { .type = NLA_S8 },
	[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC160_VAL] = { .type = NLA_U8 },
};

static const struct nla_policy
edcca_dump_policy[NUM_MTK_VENDOR_ATTRS_EDCCA_DUMP] = {
	[MTK_VENDOR_ATTR_EDCCA_DUMP_MODE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_DUMP_PRI20_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_DUMP_SEC40_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_DUMP_SEC80_VAL] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_EDCCA_DUMP_SEC160_VAL] = { .type = NLA_U8 },
};

static const struct nla_policy
three_wire_ctrl_policy[NUM_MTK_VENDOR_ATTRS_3WIRE_CTRL] = {
	[MTK_VENDOR_ATTR_3WIRE_CTRL_MODE] = {.type = NLA_U8 },
};

static const struct nla_policy
ibf_ctrl_policy[NUM_MTK_VENDOR_ATTRS_IBF_CTRL] = {
	[MTK_VENDOR_ATTR_IBF_CTRL_ENABLE] = { .type = NLA_U8 },
};

static const struct nla_policy
rfeature_ctrl_policy[NUM_MTK_VENDOR_ATTRS_RFEATURE_CTRL] = {
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_GI] = {.type = NLA_U8 },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_LTF] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_CFG] = { .type = NLA_NESTED },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_EN] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_ACK_PLCY] = { .type = NLA_U8 },
	[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TXBF] = { .type = NLA_U8 },
};

static const struct nla_policy
background_radar_ctrl_policy[NUM_MTK_VENDOR_ATTRS_BACKGROUND_RADAR_CTRL] = {
	[MTK_VENDOR_ATTR_BACKGROUND_RADAR_CTRL_MODE] = {.type = NLA_U8 },
};

static struct nla_policy
pp_ctrl_policy[NUM_MTK_VENDOR_ATTRS_PP_CTRL] = {
	[MTK_VENDOR_ATTR_PP_MODE] = { .type = NLA_U8 },
};

static const struct nla_policy
beacon_ctrl_policy[NUM_MTK_VENDOR_ATTRS_BEACON_CTRL] = {
	[MTK_VENDOR_ATTR_BEACON_CTRL_MODE] = { .type = NLA_U8 },
};

struct mt7996_amnt_data {
	u8 idx;
	u8 addr[ETH_ALEN];
	s8 rssi[4];
	u32 last_seen;
};

static int mt7996_vendor_mu_ctrl(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data,
				 int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_MU_CTRL];
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct mt7996_muru *muru;
	int err;
	u8 val8;
	u32 val32 = 0;

	err = nla_parse(tb, MTK_VENDOR_ATTR_MU_CTRL_MAX, data, data_len,
			mu_ctrl_policy, NULL);
	if (err)
		return err;

	if (tb[MTK_VENDOR_ATTR_MU_CTRL_ONOFF]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_MU_CTRL_ONOFF]);
		val32 |= FIELD_PREP(RATE_CFG_MODE, RATE_PARAM_AUTO_MU) |
			 FIELD_PREP(RATE_CFG_VAL, val8);
		ieee80211_iterate_active_interfaces_atomic(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
							   mt7996_set_wireless_vif, &val32);
	} else if (tb[MTK_VENDOR_ATTR_MU_CTRL_STRUCT]) {
		muru = kzalloc(sizeof(struct mt7996_muru), GFP_KERNEL);

		nla_memcpy(muru, tb[MTK_VENDOR_ATTR_MU_CTRL_STRUCT],
			   sizeof(struct mt7996_muru));

		err = mt7996_mcu_set_muru_cfg(phy, muru);
		kfree(muru);
	}

	return err;
}

static int
mt7996_vendor_mu_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
			   struct sk_buff *skb, const void *data, int data_len,
			   unsigned long *storage)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	int len = 0;

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	if (nla_put_u8(skb, MTK_VENDOR_ATTR_MU_CTRL_DUMP, phy->muru_onoff))
		return -ENOMEM;
	len += 1;

	return len;
}

void mt7996_set_wireless_rts_sigta(struct ieee80211_hw *hw, u8 value) {
	struct mt7996_phy *phy = mt7996_hw_phy(hw);

	switch (value) {
	case BW_SIGNALING_STATIC:
	case BW_SIGNALING_DYNAMIC:
		mt7996_mcu_set_band_confg(phy, UNI_BAND_CONFIG_RTS_SIGTA_EN, true);
		mt7996_mcu_set_band_confg(phy, UNI_BAND_CONFIG_DIS_SECCH_CCA_DET, false);
		break;
	default:
		value = BW_SIGNALING_DISABLE;
		mt7996_mcu_set_band_confg(phy, UNI_BAND_CONFIG_RTS_SIGTA_EN, false);
		mt7996_mcu_set_band_confg(phy, UNI_BAND_CONFIG_DIS_SECCH_CCA_DET, true);
		break;
      }

	phy->rts_bw_sig = value;

	/* Set RTS Threshold to a lower Value */
	mt7996_mcu_set_rts_thresh(phy, 500);
}

static int
mt7996_vendor_wireless_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
				 struct sk_buff *skb, const void *data, int data_len,
				 unsigned long *storage)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	int len = 0;

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	if (nla_put_u8(skb, MTK_VENDOR_ATTR_WIRELESS_DUMP_AMSDU,
		       ieee80211_hw_check(hw, SUPPORTS_AMSDU_IN_AMPDU)))
	return -ENOMEM;
	len += 1;

	return len;
 }

void mt7996_vendor_amnt_fill_rx(struct mt7996_phy *phy, struct sk_buff *skb)
{
	struct mt76_rx_status *status = (struct mt76_rx_status *)skb->cb;
	struct mt7996_air_monitor_ctrl *ctrl = &phy->amnt_ctrl;
	struct ieee80211_hdr *hdr = mt76_skb_get_hdr(skb);
	__le16 fc = hdr->frame_control;
	u8 addr[ETH_ALEN];
	int i;

	if (!ieee80211_has_fromds(fc))
		ether_addr_copy(addr, hdr->addr2);
	else if (ieee80211_has_tods(fc))
		ether_addr_copy(addr, hdr->addr4);
	else
		ether_addr_copy(addr, hdr->addr3);

	spin_lock_bh(&phy->amnt_lock);
	for (i = 0; i < MT7996_AIR_MONITOR_MAX_ENTRY; i++) {
		struct mt7996_air_monitor_entry *entry;

		if (ether_addr_equal(addr, ctrl->entry[i].addr)) {
			entry = &ctrl->entry[i];
			entry->rssi[0] = status->chain_signal[0];
			entry->rssi[1] = status->chain_signal[1];
			entry->rssi[2] = status->chain_signal[2];
			entry->rssi[3] = status->chain_signal[3];
			entry->last_seen = jiffies;
			break;
		}
	}
	spin_unlock_bh(&phy->amnt_lock);
}

static int
mt7996_vendor_smesh_ctrl(struct mt7996_phy *phy, u8 write,
			 u8 enable, u8 *value)
{
#define UNI_CMD_SMESH_PARAM  0
	struct mt7996_dev *dev = phy->dev;
	struct smesh_param {
		__le16 tag;
		__le16 length;

		u8 enable;
		bool a2;
		bool a1;
		bool data;
		bool mgnt;
		bool ctrl;
		u8 padding[2];
	} req = {
		.tag = cpu_to_le16(UNI_CMD_SMESH_PARAM),
		.length = cpu_to_le16(sizeof(req) - 4),

		.enable = enable,
		.a2 = true,
		.a1 = true,
		.data = true,
		.mgnt = false,
		.ctrl = false,
	};
	struct smesh_param *res;
	struct sk_buff *skb;
	int ret = 0;

	if (!value)
		return -EINVAL;

	ret = mt76_mcu_send_and_get_msg(&dev->mt76, MCU_WM_UNI_CMD(CFG_SMESH),
					&req, sizeof(req), !write, &skb);

	if (ret || write)
		return ret;

	res = (struct smesh_param *) skb->data;

	*value = res->enable;

	dev_kfree_skb(skb);

	return 0;
}

static int
mt7996_vendor_amnt_muar(struct mt7996_phy *phy, u8 muar_idx, u8 *addr)
{
#define UNI_CMD_MUAR_ENTRY  2
	struct mt7996_dev *dev = phy->dev;
	struct muar_entry {
		__le16 tag;
		__le16 length;

		bool smesh;
		u8 hw_bss_index;
		u8 muar_idx;
		u8 entry_add;
		u8 mac_addr[6];
		u8 padding[2];
	} __packed req = {
		.tag = cpu_to_le16(UNI_CMD_MUAR_ENTRY),
		.length = cpu_to_le16(sizeof(req) - 4),

		.smesh = true,
		.hw_bss_index = phy != &dev->phy,
		.muar_idx = muar_idx,
		.entry_add = 1,
	};

	ether_addr_copy(req.mac_addr, addr);
	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(REPT_MUAR), &req,
				 sizeof(req), true);
}

static int
mt7996_vendor_amnt_set_en(struct mt7996_phy *phy, u8 enable)
{
	u8 status;
	int ret;

	ret = mt7996_vendor_smesh_ctrl(phy, 0, enable, &status);
	if (ret)
		return ret;

	if (status == enable)
		return 0;

	ret = mt7996_vendor_smesh_ctrl(phy, 1, enable, &status);
	if (ret)
		return ret;

	return 0;
}

static int
mt7996_vendor_amnt_set_addr(struct mt7996_phy *phy, u8 index, u8 *addr)
{
	struct mt7996_air_monitor_ctrl *amnt_ctrl = &phy->amnt_ctrl;
	struct mt7996_air_monitor_group *group;
	struct mt7996_air_monitor_entry *entry;
	int ret, i, j;

	if (index >= MT7996_AIR_MONITOR_MAX_ENTRY)
		return -1;

	spin_lock_bh(&phy->amnt_lock);
	entry = &amnt_ctrl->entry[index];
	if (!is_zero_ether_addr(addr)) {
		if (entry->enable == false) {
			for (i = 0; i < MT7996_AIR_MONITOR_MAX_GROUP; i++) {
				group = &(amnt_ctrl->group[i]);
				if (group->used[0] == false)
					j = 0;
				else if (group->used[1] == false)
					j = 1;
				else
					continue;

				group->enable = true;
				group->used[j] = true;
				entry->enable = true;
				entry->group_idx = i;
				entry->group_used_idx = j;
				entry->muar_idx = 32 + 4 * i + 2 * j;
				break;
			}
		}
	} else {
		group = &(amnt_ctrl->group[entry->group_idx]);

		group->used[entry->group_used_idx] = false;
		if (group->used[0] == false && group->used[1] == false)
			group->enable = false;

		entry->enable = false;
	}
	ether_addr_copy(entry->addr, addr);
	amnt_ctrl->enable &= ~(1 << entry->group_idx);
	amnt_ctrl->enable |= entry->enable << entry->group_idx;
	spin_unlock_bh(&phy->amnt_lock);

	ret = mt7996_vendor_amnt_muar(phy, entry->muar_idx, addr);
	if (ret)
		return ret;

	return mt7996_vendor_amnt_set_en(phy, amnt_ctrl->enable);
}

static int
mt7966_vendor_amnt_ctrl(struct wiphy *wiphy, struct wireless_dev *wdev,
			const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct nlattr *tb1[NUM_MTK_VENDOR_ATTRS_AMNT_CTRL];
	struct nlattr *tb2[NUM_MTK_VENDOR_ATTRS_AMNT_SET];
	u8 index = 0;
	u8 mac_addr[ETH_ALEN];
	int err;

	err = nla_parse(tb1, MTK_VENDOR_ATTR_AMNT_CTRL_MAX, data, data_len,
			amnt_ctrl_policy, NULL);
	if (err)
		return err;

	if (!tb1[MTK_VENDOR_ATTR_AMNT_CTRL_SET])
		return -EINVAL;

	err = nla_parse_nested(tb2, MTK_VENDOR_ATTR_AMNT_SET_MAX,
		tb1[MTK_VENDOR_ATTR_AMNT_CTRL_SET], amnt_set_policy, NULL);

	if (!tb2[MTK_VENDOR_ATTR_AMNT_SET_INDEX] ||
		!tb2[MTK_VENDOR_ATTR_AMNT_SET_MACADDR])
		return -EINVAL;

	index = nla_get_u8(tb2[MTK_VENDOR_ATTR_AMNT_SET_INDEX]);
	memcpy(mac_addr, nla_data(tb2[MTK_VENDOR_ATTR_AMNT_SET_MACADDR]), ETH_ALEN);

	return mt7996_vendor_amnt_set_addr(phy, index, mac_addr);
}

int mt7996_vendor_amnt_sta_remove(struct mt7996_phy *phy,
				  struct ieee80211_sta *sta)
{
	u8 zero[ETH_ALEN] = {};
	int i;

	if (!phy->amnt_ctrl.enable)
		return 0;

	for (i = 0; i < MT7996_AIR_MONITOR_MAX_ENTRY; i++)
		if (ether_addr_equal(sta->addr, phy->amnt_ctrl.entry[i].addr))
			return mt7996_vendor_amnt_set_addr(phy, i, zero);
	return 0;
}

static int
mt7996_amnt_dump(struct mt7996_phy *phy, struct sk_buff *skb,
		 u8 amnt_idx, int *attrtype)
{
	struct mt7996_air_monitor_entry *entry;
	struct mt7996_amnt_data data;
	u32 last_seen = 0;

	spin_lock_bh(&phy->amnt_lock);
	entry = &phy->amnt_ctrl.entry[amnt_idx];
	if (entry->enable == 0) {
		spin_unlock_bh(&phy->amnt_lock);
		return 0;
	}

	last_seen = jiffies_to_msecs(jiffies - entry->last_seen);
	ether_addr_copy(data.addr, entry->addr);
	data.rssi[0] = entry->rssi[0];
	data.rssi[1] = entry->rssi[1];
	data.rssi[2] = entry->rssi[2];
	data.rssi[3] = entry->rssi[3];
	spin_unlock_bh(&phy->amnt_lock);

	data.idx = amnt_idx;
	data.last_seen = last_seen;

	nla_put(skb, (*attrtype)++, sizeof(struct mt7996_amnt_data), &data);

	return 1;
}

static int
mt7966_vendor_amnt_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
			     struct sk_buff *skb, const void *data, int data_len,
			     unsigned long *storage)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct nlattr *tb1[NUM_MTK_VENDOR_ATTRS_AMNT_CTRL];
	struct nlattr *tb2[NUM_MTK_VENDOR_ATTRS_AMNT_DUMP];
	void *a, *b;
	int err = 0, attrtype = 0, i, len = 0;
	u8 amnt_idx;

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	err = nla_parse(tb1, MTK_VENDOR_ATTR_AMNT_CTRL_MAX, data, data_len,
			amnt_ctrl_policy, NULL);
	if (err)
		return err;

	if (!tb1[MTK_VENDOR_ATTR_AMNT_CTRL_DUMP])
		return -EINVAL;

	err = nla_parse_nested(tb2, MTK_VENDOR_ATTR_AMNT_DUMP_MAX,
			       tb1[MTK_VENDOR_ATTR_AMNT_CTRL_DUMP],
			       amnt_dump_policy, NULL);
	if (err)
		return err;

	if (!tb2[MTK_VENDOR_ATTR_AMNT_DUMP_INDEX])
		return -EINVAL;

	amnt_idx = nla_get_u8(tb2[MTK_VENDOR_ATTR_AMNT_DUMP_INDEX]);

	a = nla_nest_start(skb, MTK_VENDOR_ATTR_AMNT_CTRL_DUMP);
	b = nla_nest_start(skb, MTK_VENDOR_ATTR_AMNT_DUMP_RESULT);

	if (amnt_idx != 0xff) {
		len += mt7996_amnt_dump(phy, skb, amnt_idx, &attrtype);
	} else {
		for (i = 0; i < MT7996_AIR_MONITOR_MAX_ENTRY; i++)
			len += mt7996_amnt_dump(phy, skb, i, &attrtype);
	}

	nla_nest_end(skb, b);

	nla_put_u8(skb, MTK_VENDOR_ATTR_AMNT_DUMP_LEN, len);

	nla_nest_end(skb, a);

	return len + 1;
}

static int
mt7996_vendor_bss_color_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
				  struct sk_buff *skb, const void *data, int data_len,
				  unsigned long *storage)
{
	struct ieee80211_vif *vif = wdev_to_ieee80211_vif(wdev);
	struct ieee80211_bss_conf *bss_conf = &vif->bss_conf;
	int len = 0;

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	if (nla_put_u64_64bit(skb, MTK_VENDOR_ATTR_AVAL_BSS_COLOR_BMP,
			      ~bss_conf->used_color_bitmap, NL80211_ATTR_PAD))
		return -ENOMEM;
	len += 1;

	return len;
}

static int mt7996_vendor_edcca_ctrl(struct wiphy *wiphy, struct wireless_dev *wdev,
				    const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_EDCCA_CTRL];
	int err;
	u8 edcca_mode;
	u8 edcca_value[EDCCA_MAX_BW_NUM];

	err = nla_parse(tb, MTK_VENDOR_ATTR_EDCCA_CTRL_MAX, data, data_len,
			edcca_ctrl_policy, NULL);
	if (err)
		return err;

	if (!tb[MTK_VENDOR_ATTR_EDCCA_CTRL_MODE])
		return -EINVAL;

	edcca_mode = nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_MODE]);
	if (edcca_mode == EDCCA_CTRL_SET_EN) {
		if (!tb[MTK_VENDOR_ATTR_EDCCA_CTRL_PRI20_VAL])
			return -EINVAL;

		edcca_value[0] =
			nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_PRI20_VAL]);

		err = mt7996_mcu_edcca_enable(phy, !!edcca_value[0]);
		if (err)
			return err;
	} else if (edcca_mode == EDCCA_CTRL_SET_THRES) {
		if (!tb[MTK_VENDOR_ATTR_EDCCA_CTRL_PRI20_VAL] ||
		    !tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC40_VAL] ||
		    !tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC80_VAL] ||
		    !tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC160_VAL]) {
			return -EINVAL;
		}
		edcca_value[EDCCA_BW_20] =
			nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_PRI20_VAL]);
		edcca_value[EDCCA_BW_40] =
			nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC40_VAL]);
		edcca_value[EDCCA_BW_80] =
			nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC80_VAL]);
		edcca_value[EDCCA_BW_160] =
			nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_SEC160_VAL]);

		err = mt7996_mcu_edcca_threshold_ctrl(phy, edcca_value, true);

		if (err)
			return err;
	} else {
		return -EINVAL;
	}

	return 0;
}


static int
mt7996_vendor_edcca_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
			     struct sk_buff *skb, const void *data, int data_len,
			     unsigned long *storage)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_EDCCA_CTRL];
	int err;
	u8 edcca_mode;
	u8 value[EDCCA_MAX_BW_NUM];

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	err = nla_parse(tb, MTK_VENDOR_ATTR_EDCCA_CTRL_MAX, data, data_len,
			edcca_ctrl_policy, NULL);
	if (err)
		return err;

	if (!tb[MTK_VENDOR_ATTR_EDCCA_CTRL_MODE])
		return -EINVAL;

	edcca_mode = nla_get_u8(tb[MTK_VENDOR_ATTR_EDCCA_CTRL_MODE]);

	if (edcca_mode != EDCCA_CTRL_GET_THRES)
		return -EINVAL;

	err = mt7996_mcu_edcca_threshold_ctrl(phy, value, false);

	if (err)
		return err;

	if (nla_put_u8(skb, MTK_VENDOR_ATTR_EDCCA_DUMP_PRI20_VAL, value[EDCCA_BW_20]) ||
	    nla_put_u8(skb, MTK_VENDOR_ATTR_EDCCA_DUMP_SEC40_VAL, value[EDCCA_BW_40]) ||
	    nla_put_u8(skb, MTK_VENDOR_ATTR_EDCCA_DUMP_SEC80_VAL, value[EDCCA_BW_80]) ||
	    nla_put_u8(skb, MTK_VENDOR_ATTR_EDCCA_DUMP_SEC160_VAL, value[EDCCA_BW_160]))
		return -ENOMEM;

	return EDCCA_MAX_BW_NUM;
}

static int mt7996_vendor_3wire_ctrl(struct wiphy *wiphy, struct wireless_dev *wdev,
				    const void *data, int data_len)
{
#define UNI_3WIRE_EXT_EN	0
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_dev *dev = mt7996_hw_dev(hw);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_3WIRE_CTRL];
	struct {
		u8 __rsv1[4];

		__le16 tag;
		__le16 len;
		u8 three_wire_mode;
	} __packed req = {
		.tag = cpu_to_le16(UNI_3WIRE_EXT_EN),
		.len = cpu_to_le16(sizeof(req) - 4),
	};
	int err;

	err = nla_parse(tb, MTK_VENDOR_ATTR_3WIRE_CTRL_MAX, data, data_len,
			three_wire_ctrl_policy, NULL);
	if (err)
		return err;

	if (!tb[MTK_VENDOR_ATTR_3WIRE_CTRL_MODE])
		return -EINVAL;

	req.three_wire_mode = nla_get_u8(tb[MTK_VENDOR_ATTR_3WIRE_CTRL_MODE]);

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(PTA_3WIRE_CTRL), &req,
				 sizeof(req), false);
}

static int mt7996_vendor_ibf_ctrl(struct wiphy *wiphy,
				  struct wireless_dev *wdev,
				  const void *data,
				  int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct mt7996_dev *dev = phy->dev;
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_IBF_CTRL];
	int err;
	u8 val;

	err = nla_parse(tb, MTK_VENDOR_ATTR_IBF_CTRL_MAX, data, data_len,
			ibf_ctrl_policy, NULL);
	if (err)
		return err;

	if (tb[MTK_VENDOR_ATTR_IBF_CTRL_ENABLE]) {
		val = nla_get_u8(tb[MTK_VENDOR_ATTR_IBF_CTRL_ENABLE]);

		dev->ibf = !!val;

		err = mt7996_mcu_set_txbf(dev, BF_HW_EN_UPDATE);
		if (err)
			return err;
	}
	return 0;
}

static int
mt7996_vendor_ibf_ctrl_dump(struct wiphy *wiphy, struct wireless_dev *wdev,
			    struct sk_buff *skb, const void *data, int data_len,
			    unsigned long *storage)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct mt7996_dev *dev = phy->dev;

	if (*storage == 1)
		return -ENOENT;
	*storage = 1;

	if (nla_put_u8(skb, MTK_VENDOR_ATTR_IBF_DUMP_ENABLE, dev->ibf))
		return -ENOMEM;

	return 1;
}

static int mt7996_vendor_rfeature_ctrl(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       const void *data,
				       int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct mt7996_dev *dev = phy->dev;
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_RFEATURE_CTRL];
	int err;
	u32 val;

	err = nla_parse(tb, MTK_VENDOR_ATTR_RFEATURE_CTRL_MAX, data, data_len,
			rfeature_ctrl_policy, NULL);
	if (err)
		return err;

	val = CAPI_RFEATURE_CHANGED;

	if (tb[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_CFG]) {
		u8 enable, trig_type;
		int rem;
		struct nlattr *cur;

		nla_for_each_nested(cur, tb[MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_CFG], rem) {
			switch (nla_type(cur)) {
			case MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_EN:
				enable = nla_get_u8(cur);
				break;
			case MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE:
				trig_type = nla_get_u8(cur);
				break;
			default:
				return -EINVAL;
			};
		}

		err = mt7996_mcu_set_rfeature_trig_type(phy, enable, trig_type);
		if (err)
			return err;
	} else if (tb[MTK_VENDOR_ATTR_RFEATURE_CTRL_ACK_PLCY]) {
		u8 ack_policy;

		ack_policy = nla_get_u8(tb[MTK_VENDOR_ATTR_RFEATURE_CTRL_ACK_PLCY]);
		switch (ack_policy) {
		case MU_DL_ACK_POLICY_TF_FOR_ACK:
			return mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_MUDL_ACK_POLICY,
						       ack_policy);
		default:
			return 0;
		}
	}

	return 0;
}

static int mt7996_vendor_wireless_ctrl(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       const void *data,
				       int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	struct mt7996_dev *dev = phy->dev;
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_WIRELESS_CTRL];
	int err;
	u8 val8;
	u16 val16;
	u32 val32;

	err = nla_parse(tb, MTK_VENDOR_ATTR_WIRELESS_CTRL_MAX, data, data_len,
			wireless_ctrl_policy, NULL);
	if (err)
		return err;

	val32 = CAPI_WIRELESS_CHANGED;

	if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_OFDMA]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_OFDMA]);
		val32 |= FIELD_PREP(RATE_CFG_MODE, RATE_PARAM_FIXED_OFDMA) |
			 FIELD_PREP(RATE_CFG_VAL, val8);
		ieee80211_iterate_active_interfaces_atomic(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
			mt7996_set_wireless_vif, &val32);
		if (val8 == 3) /* DL20and80 */
			mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_20M_DYN_ALGO, 1);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_BA_BUFFER_SIZE]) {
		val16 = nla_get_u16(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_BA_BUFFER_SIZE]);
		hw->max_tx_aggregation_subframes = val16;
		hw->max_rx_aggregation_subframes = val16;
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_PPDU_TX_TYPE]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_PPDU_TX_TYPE]);
		mt7996_mcu_set_ppdu_tx_type(phy, val8);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_NUSERS_OFDMA]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_NUSERS_OFDMA]);
		if (phy->muru_onoff & OFDMA_UL)
			mt7996_mcu_set_nusers_ofdma(phy, MU_CTRL_UL_USER_CNT, val8);
		else
			mt7996_mcu_set_nusers_ofdma(phy, MU_CTRL_DL_USER_CNT, val8);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_MIMO]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_MIMO]);
		val32 |= FIELD_PREP(RATE_CFG_MODE, RATE_PARAM_FIXED_MIMO) |
			 FIELD_PREP(RATE_CFG_VAL, val8);
		ieee80211_iterate_active_interfaces_atomic(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
			mt7996_set_wireless_vif, &val32);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_CERT]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_CERT]);
		dev->cert_mode = val8;
		mt7996_mcu_set_cert(phy, val8);
		mt7996_mcu_set_bypass_smthint(phy, val8);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_AMSDU]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_AMSDU]);
		mt7996_set_wireless_amsdu(hw, val8);
	} else if (tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_RTS_SIGTA]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_WIRELESS_CTRL_RTS_SIGTA]);
		mt7996_set_wireless_rts_sigta(hw, val8);
	}

	return 0;
}

static int mt7996_vendor_background_radar_mode_ctrl(struct wiphy *wiphy,
						    struct wireless_dev *wdev,
						    const void *data,
						    int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct mt7996_dev *dev = mt7996_hw_dev(hw);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_BACKGROUND_RADAR_CTRL];
	int err;
	u8 background_radar_mode;

	err = nla_parse(tb, MTK_VENDOR_ATTR_BACKGROUND_RADAR_CTRL_MAX, data, data_len,
			background_radar_ctrl_policy, NULL);
	if (err)
		return err;

	background_radar_mode = nla_get_u8(tb[MTK_VENDOR_ATTR_BACKGROUND_RADAR_CTRL_MODE]);

	return mt7996_mcu_rdd_background_disable_timer(dev, !!background_radar_mode);
}

static int mt7996_vendor_pp_ctrl(struct wiphy *wiphy, struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_PP_CTRL];
	struct mt7996_phy *phy = mt7996_hw_phy(hw);
	int err;
	u8 val8;

	err = nla_parse(tb, MTK_VENDOR_ATTR_PP_CTRL_MAX, data, data_len,
			pp_ctrl_policy, NULL);
	if (err)
		return err;

	if (tb[MTK_VENDOR_ATTR_PP_MODE]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_PP_MODE]);
		err = mt7996_mcu_set_pp_en(phy, !!val8, 0, 0);
	}

	return err;
}

static int mt7996_vendor_beacon_ctrl(struct wiphy *wiphy,
				     struct wireless_dev *wdev,
				     const void *data,
				     int data_len)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct nlattr *tb[NUM_MTK_VENDOR_ATTRS_BEACON_CTRL];
	int err;
	u8 val8;

	err = nla_parse(tb, MTK_VENDOR_ATTR_BEACON_CTRL_MAX, data, data_len,
			beacon_ctrl_policy, NULL);
	if (err)
		return err;

	if (tb[MTK_VENDOR_ATTR_BEACON_CTRL_MODE]) {
		val8 = nla_get_u8(tb[MTK_VENDOR_ATTR_BEACON_CTRL_MODE]);
		ieee80211_iterate_active_interfaces_atomic(hw, IEEE80211_IFACE_ITER_RESUME_ALL,
				mt7996_set_beacon_vif, &val8);
	}

	return 0;
}


static const struct wiphy_vendor_command mt7996_vendor_commands[] = {
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_MU_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_mu_ctrl,
		.dumpit = mt7996_vendor_mu_ctrl_dump,
		.policy = mu_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_MU_CTRL_MAX,
	},
	{
		.info = {
		        .vendor_id = MTK_NL80211_VENDOR_ID,
		        .subcmd = MTK_NL80211_VENDOR_SUBCMD_WIRELESS_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
		        WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_wireless_ctrl,
		.dumpit = mt7996_vendor_wireless_ctrl_dump,
		.policy = wireless_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_WIRELESS_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_AMNT_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7966_vendor_amnt_ctrl,
		.dumpit = mt7966_vendor_amnt_ctrl_dump,
		.policy = amnt_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_AMNT_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_BSS_COLOR_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.dumpit = mt7996_vendor_bss_color_ctrl_dump,
		.policy = bss_color_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_BSS_COLOR_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_EDCCA_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_edcca_ctrl,
		.dumpit = mt7996_vendor_edcca_ctrl_dump,
		.policy = edcca_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_EDCCA_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_3WIRE_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_3wire_ctrl,
		.policy = three_wire_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_3WIRE_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_IBF_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_ibf_ctrl,
		.dumpit = mt7996_vendor_ibf_ctrl_dump,
		.policy = ibf_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_IBF_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_RFEATURE_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_rfeature_ctrl,
		.policy = rfeature_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_RFEATURE_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_BACKGROUND_RADAR_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_background_radar_mode_ctrl,
		.policy = background_radar_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_BACKGROUND_RADAR_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_PP_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_pp_ctrl,
		.policy = pp_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_PP_CTRL_MAX,
	},
	{
		.info = {
			.vendor_id = MTK_NL80211_VENDOR_ID,
			.subcmd = MTK_NL80211_VENDOR_SUBCMD_BEACON_CTRL,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mt7996_vendor_beacon_ctrl,
		.policy = beacon_ctrl_policy,
		.maxattr = MTK_VENDOR_ATTR_BEACON_CTRL_MAX,
	},
};

void mt7996_vendor_register(struct mt7996_phy *phy)
{
	phy->mt76->hw->wiphy->vendor_commands = mt7996_vendor_commands;
	phy->mt76->hw->wiphy->n_vendor_commands = ARRAY_SIZE(mt7996_vendor_commands);

	spin_lock_init(&phy->amnt_lock);
}
#endif
