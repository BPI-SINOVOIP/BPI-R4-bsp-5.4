// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2023 MediaTek Inc.
 */

#include <linux/firmware.h>
#include <linux/fs.h>
#include "mt7996.h"
#include "mcu.h"
#include "mac.h"
#include "mtk_mcu.h"

#ifdef CONFIG_MTK_DEBUG

int mt7996_mcu_get_tx_power_info(struct mt7996_phy *phy, u8 category, void *event)
{
	struct mt7996_dev *dev = phy->dev;
	struct tx_power_ctrl req = {
		.tag = cpu_to_le16(UNI_TXPOWER_SHOW_INFO),
		.len = cpu_to_le16(sizeof(req) - 4),
		.power_ctrl_id = UNI_TXPOWER_SHOW_INFO,
		.show_info_category = category,
		.band_idx = phy->mt76->band_idx,
	};
	struct sk_buff *skb;
	int ret;

	ret = mt76_mcu_send_and_get_msg(&dev->mt76,
					MCU_WM_UNI_CMD_QUERY(TXPOWER),
					&req, sizeof(req), true, &skb);
	if (ret)
		return ret;

	memcpy(event, skb->data, sizeof(struct mt7996_mcu_txpower_event));

	dev_kfree_skb(skb);

	return 0;
}

int mt7996_mcu_muru_dbg_info(struct mt7996_dev *dev, u16 item, u8 val)
{
	struct {
		u8 __rsv1[4];

		__le16 tag;
		__le16 len;

		__le16 item;
		u8 __rsv2[2];
		__le32 value;
	} __packed req = {
		.tag = cpu_to_le16(UNI_CMD_MURU_DBG_INFO),
		.len = cpu_to_le16(sizeof(req) - 4),
		.item = cpu_to_le16(item),
		.value = cpu_to_le32(val),
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &req,
				 sizeof(req), true);
}

int mt7996_mcu_edcca_enable(struct mt7996_phy *phy, bool enable)
{
	struct mt7996_dev *dev = phy->dev;
	struct cfg80211_chan_def *chandef = &phy->mt76->chandef;
	enum nl80211_band band = chandef->chan->band;
	struct {
		u8 band_idx;
		u8 _rsv[3];

		__le16 tag;
		__le16 len;
		u8 enable;
		u8 std;
		u8 _rsv2[2];
	} __packed req = {
		.band_idx = phy->mt76->band_idx,
		.tag = cpu_to_le16(UNI_BAND_CONFIG_EDCCA_ENABLE),
		.len = cpu_to_le16(sizeof(req) - 4),
		.enable = enable,
		.std = EDCCA_DEFAULT,
	};

	switch (dev->mt76.region) {
	case NL80211_DFS_JP:
		req.std = EDCCA_JAPAN;
		break;
	case NL80211_DFS_FCC:
		if (band == NL80211_BAND_6GHZ)
			req.std = EDCCA_FCC;
		break;
	case NL80211_DFS_ETSI:
		if (band == NL80211_BAND_6GHZ)
			req.std = EDCCA_ETSI;
		break;
	default:
		break;
	}

	return mt76_mcu_send_msg(&phy->dev->mt76, MCU_WM_UNI_CMD(BAND_CONFIG),
				 &req, sizeof(req), true);
}

int mt7996_mcu_edcca_threshold_ctrl(struct mt7996_phy *phy, u8 *value, bool set)
{
	struct {
		u8 band_idx;
		u8 _rsv[3];

		__le16 tag;
		__le16 len;
		u8 threshold[4];
		bool init;
	} __packed *res, req = {
		.band_idx = phy->mt76->band_idx,
		.tag = cpu_to_le16(UNI_BAND_CONFIG_EDCCA_THRESHOLD),
		.len = cpu_to_le16(sizeof(req) - 4),
		.init = false,
	};
	struct sk_buff *skb;
	int ret;
	int i;

	for (i = 0; i < EDCCA_MAX_BW_NUM; i++)
		req.threshold[i] = value[i];

	if (set)
		return mt76_mcu_send_msg(&phy->dev->mt76, MCU_WM_UNI_CMD(BAND_CONFIG),
					 &req, sizeof(req), true);

	ret = mt76_mcu_send_and_get_msg(&phy->dev->mt76,
					MCU_WM_UNI_CMD_QUERY(BAND_CONFIG),
					&req, sizeof(req), true, &skb);

	if (ret)
		return ret;

	res = (void *)skb->data;

	for (i = 0; i < EDCCA_MAX_BW_NUM; i++)
		value[i] = res->threshold[i];

	dev_kfree_skb(skb);

	return 0;
}

int mt7996_mcu_set_sr_enable(struct mt7996_phy *phy, u8 action, u64 val, bool set)
{
	struct {
		u8 band_idx;
		u8 _rsv[3];

		__le16 tag;
		__le16 len;

		__le32 val;

	} __packed req = {
		.band_idx = phy->mt76->band_idx,

		.tag = cpu_to_le16(action),
		.len = cpu_to_le16(sizeof(req) - 4),

		.val = cpu_to_le32((u32) val),
	};

	if (set)
		return mt76_mcu_send_msg(&phy->dev->mt76, MCU_WM_UNI_CMD(SR), &req,
					 sizeof(req), false);
	else
		return mt76_mcu_send_msg(&phy->dev->mt76, MCU_WM_UNI_CMD_QUERY(SR), &req,
					 sizeof(req), false);
}

void mt7996_mcu_rx_sr_swsd(struct mt7996_dev *dev, struct sk_buff *skb)
{
#define SR_SCENE_DETECTION_TIMER_PERIOD_MS 500
	struct mt7996_mcu_sr_swsd_event *event;
	static const char * const rules[] = {"1 - NO CONNECTED", "2 - NO CONGESTION",
					     "3 - NO INTERFERENCE", "4 - SR ON"};
	u8 idx;

	event = (struct mt7996_mcu_sr_swsd_event *)skb->data;
	idx = event->basic.band_idx;

	dev_info(dev->mt76.dev, "Band index = %u\n", le16_to_cpu(event->basic.band_idx));
	dev_info(dev->mt76.dev, "Hit Rule = %s\n", rules[event->tlv[idx].rule]);
	dev_info(dev->mt76.dev, "Timer Period = %d(us)\n"
		 "Congestion Ratio  = %d.%1d%%\n",
		 SR_SCENE_DETECTION_TIMER_PERIOD_MS * 1000,
		 le32_to_cpu(event->tlv[idx].total_airtime_ratio) / 10,
		 le32_to_cpu(event->tlv[idx].total_airtime_ratio) % 10);
	dev_info(dev->mt76.dev,
		 "Total Airtime = %d(us)\n"
		 "ChBusy = %d\n"
		 "SrTx = %d\n"
		 "OBSS = %d\n"
		 "MyTx = %d\n"
		 "MyRx = %d\n"
		 "Interference Ratio = %d.%1d%%\n",
		 le32_to_cpu(event->tlv[idx].total_airtime),
		 le32_to_cpu(event->tlv[idx].channel_busy_time),
		 le32_to_cpu(event->tlv[idx].sr_tx_airtime),
		 le32_to_cpu(event->tlv[idx].obss_airtime),
		 le32_to_cpu(event->tlv[idx].my_tx_airtime),
		 le32_to_cpu(event->tlv[idx].my_rx_airtime),
		 le32_to_cpu(event->tlv[idx].obss_airtime_ratio) / 10,
		 le32_to_cpu(event->tlv[idx].obss_airtime_ratio) % 10);
}

void mt7996_mcu_rx_sr_hw_indicator(struct mt7996_dev *dev, struct sk_buff *skb)
{
	struct mt7996_mcu_sr_hw_ind_event *event;

	event = (struct mt7996_mcu_sr_hw_ind_event *)skb->data;

	dev_info(dev->mt76.dev, "Inter PPDU Count = %u\n",
		 le16_to_cpu(event->inter_bss_ppdu_cnt));
	dev_info(dev->mt76.dev, "SR Valid Count = %u\n",
		 le16_to_cpu(event->non_srg_valid_cnt));
	dev_info(dev->mt76.dev, "SR Tx Count = %u\n",
		 le32_to_cpu(event->sr_ampdu_mpdu_cnt));
	dev_info(dev->mt76.dev, "SR Tx Acked Count = %u\n",
		 le32_to_cpu(event->sr_ampdu_mpdu_acked_cnt));
}

void mt7996_mcu_rx_sr_event(struct mt7996_dev *dev, struct sk_buff *skb)
{
	struct mt76_phy *mphy = &dev->mt76.phy;
	struct mt7996_phy *phy;
	struct mt7996_mcu_sr_common_event *event;

	event = (struct mt7996_mcu_sr_common_event *)skb->data;
	mphy = dev->mt76.phys[event->basic.band_idx];
	if (!mphy)
		return;

	phy = (struct mt7996_phy *)mphy->priv;

	switch (le16_to_cpu(event->basic.tag)) {
	case UNI_EVENT_SR_CFG_SR_ENABLE:
		phy->sr_enable = le32_to_cpu(event->value) ? true : false;
		break;
	case UNI_EVENT_SR_HW_ESR_ENABLE:
		phy->enhanced_sr_enable = le32_to_cpu(event->value) ? true : false;
		break;
	case UNI_EVENT_SR_SW_SD:
		mt7996_mcu_rx_sr_swsd(dev, skb);
		break;
	case UNI_EVENT_SR_HW_IND:
		mt7996_mcu_rx_sr_hw_indicator(dev, skb);
		break;
	default:
		dev_info(dev->mt76.dev, "Unknown SR event tag %d\n",
			 le16_to_cpu(event->basic.tag));
	}
}

int mt7996_mcu_set_dup_wtbl(struct mt7996_dev *dev)
{
#define CHIP_CONFIG_DUP_WTBL	4
#define DUP_WTBL_NUM	80
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;
		__le16 base;
		__le16 num;
		u8 _rsv2[4];
	} __packed req = {
		.tag = cpu_to_le16(CHIP_CONFIG_DUP_WTBL),
		.len = cpu_to_le16(sizeof(req) - 4),
		.base = cpu_to_le16(MT7996_WTBL_STA - DUP_WTBL_NUM + 1),
		.num = cpu_to_le16(DUP_WTBL_NUM),
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(CHIP_CONFIG), &req,
				 sizeof(req), true);
}

static struct tlv *
__mt7996_mcu_add_uni_tlv(struct sk_buff *skb, u16 tag, u16 len)
{
	struct tlv *ptlv, tlv = {
		.tag = cpu_to_le16(tag),
		.len = cpu_to_le16(len),
	};

	ptlv = skb_put(skb, len);
	memcpy(ptlv, &tlv, sizeof(tlv));

	return ptlv;
}

int mt7996_mcu_set_txbf_internal(struct mt7996_phy *phy, u8 action, int idx, bool bfer)
{
	struct mt7996_dev *dev = phy->dev;
#define MT7996_MTK_BF_MAX_SIZE	sizeof(struct bf_starec_read)
	struct uni_header hdr;
	struct sk_buff *skb;
	struct tlv *tlv;
	int len = sizeof(hdr) + MT7996_MTK_BF_MAX_SIZE;

	memset(&hdr, 0, sizeof(hdr));

	skb = mt76_mcu_msg_alloc(&dev->mt76, NULL, len);
	if (!skb)
		return -ENOMEM;

	skb_put_data(skb, &hdr, sizeof(hdr));

	switch (action) {
	case BF_PFMU_TAG_READ: {
		struct bf_pfmu_tag *req;

		tlv = __mt7996_mcu_add_uni_tlv(skb, action, sizeof(*req));
		req = (struct bf_pfmu_tag *)tlv;
		req->pfmu_id = idx;
		req->bfer = bfer;
		req->band_idx = phy->mt76->band_idx;
		break;
	}
	case BF_STA_REC_READ: {
		struct bf_starec_read *req;

		tlv = __mt7996_mcu_add_uni_tlv(skb, action, sizeof(*req));
		req = (struct bf_starec_read *)tlv;
		req->wlan_idx = idx;
		break;
	}
	case BF_FBRPT_DBG_INFO_READ: {
		struct bf_fbk_rpt_info *req;

		if (idx != 0) {
			dev_info(dev->mt76.dev, "Invalid input");
			return 0;
		}

		tlv = __mt7996_mcu_add_uni_tlv(skb, action, sizeof(*req));
		req = (struct bf_fbk_rpt_info *)tlv;
		req->action = idx;
		req->band_idx = phy->mt76->band_idx;
		break;
	}
	default:
		return -EINVAL;
	}

	return mt76_mcu_skb_send_msg(&phy->dev->mt76, skb, MCU_WM_UNI_CMD(BF), false);
}

int mt7996_mcu_set_txbf_snd_info(struct mt7996_phy *phy, void *para)
{
	char *buf = (char *)para;
	__le16 input[5] = {0};
	u8 recv_arg = 0;
	struct bf_txsnd_info *req;
	struct uni_header hdr;
	struct sk_buff *skb;
	struct tlv *tlv;
	int len = sizeof(hdr) + MT7996_MTK_BF_MAX_SIZE;

	memset(&hdr, 0, sizeof(hdr));

	skb = mt76_mcu_msg_alloc(&phy->dev->mt76, NULL, len);
	if (!skb)
		return -ENOMEM;

	skb_put_data(skb, &hdr, sizeof(hdr));

	recv_arg = sscanf(buf, "%hx:%hx:%hx:%hx:%hx", &input[0], &input[1], &input[2],
						      &input[3], &input[4]);

	if (!recv_arg)
		return -EINVAL;

	tlv = __mt7996_mcu_add_uni_tlv(skb, BF_TXSND_INFO, sizeof(*req));
	req = (struct bf_txsnd_info *)tlv;
	req->action = input[0];

	switch (req->action) {
	case BF_SND_READ_INFO: {
		req->read_clr = input[1];
		break;
	}
	case BF_SND_CFG_OPT: {
		req->vht_opt = input[1];
		req->he_opt = input[2];
		req->glo_opt = input[3];
		break;
	}
	case BF_SND_CFG_INTV: {
		req->wlan_idx = input[1];
		req->snd_intv = input[2];
		break;
	}
	case BF_SND_STA_STOP: {
		req->wlan_idx = input[1];
		req->snd_stop = input[2];
		break;
	}
	case BF_SND_CFG_MAX_STA: {
		req->max_snd_stas = input[1];
		break;
	}
	case BF_SND_CFG_BFRP: {
		req->man = input[1];
		req->tx_time = input[2];
		req->mcs = input[3];
		req->ldpc = input[4];
		break;
	}
	case BF_SND_CFG_INF: {
		req->inf = input[1];
		break;
	}
	case BF_SND_CFG_TXOP_SND: {
		req->man = input[1];
		req->ac_queue = input[2];
		req->sxn_protect = input[3];
		req->direct_fbk = input[4];
		break;
	}
	default:
		return -EINVAL;
	}

	return mt76_mcu_skb_send_msg(&phy->dev->mt76, skb, MCU_WM_UNI_CMD(BF), false);
}

static inline void
mt7996_ibf_phase_assign(struct mt7996_dev *dev,
			struct mt7996_ibf_cal_info *cal,
			struct mt7996_txbf_phase *phase)
{
	/* fw return ibf calibrated data with
	 * the mt7996_txbf_phase_info_5g struct for both 2G and 5G.
	 * Therefore, memcpy cannot be used here.
	 */
	phase_assign(cal->group, m_t0_h, true);
	phase_assign(cal->group, m_t1_h, true);
	phase_assign(cal->group, m_t2_h, true);
	phase_assign(cal->group, m_t2_h_sx2, false);
	phase_assign_rx(cal->group, r0);
	phase_assign_rx(cal->group, r1);
	phase_assign_rx(cal->group, r2);
	phase_assign_rx(cal->group, r3);
	phase_assign_rx_g0(cal->group, r2_sx2);
	phase_assign_rx_g0(cal->group, r3_sx2);
	phase_assign(cal->group, r0_reserved, false);
	phase_assign(cal->group, r1_reserved, false);
	phase_assign(cal->group, r2_reserved, false);
	phase_assign(cal->group, r3_reserved, false);
	phase_assign(cal->group, r2_sx2_reserved, false);
	phase_assign(cal->group, r3_sx2_reserved, false);
}

void
mt7996_mcu_rx_bf_event(struct mt7996_dev *dev, struct sk_buff *skb)
{
	struct mt7996_mcu_bf_basic_event *event;

	event = (struct mt7996_mcu_bf_basic_event *)skb->data;

	dev_info(dev->mt76.dev, " bf_event tag = %d\n", event->tag);

	switch (event->tag) {
	case UNI_EVENT_BF_PFMU_TAG: {

		struct mt7996_pfmu_tag_event *tag;
		u32 *raw_t1, *raw_t2;

		tag = (struct mt7996_pfmu_tag_event *) skb->data;

		raw_t1 = (u32 *)&tag->t1;
		raw_t2 = (u32 *)&tag->t2;

		dev_info(dev->mt76.dev, "=================== TXBf Profile Tag1 Info ==================\n");
		dev_info(dev->mt76.dev,
			 "DW0 = 0x%08x, DW1 = 0x%08x, DW2 = 0x%08x\n",
			 raw_t1[0], raw_t1[1], raw_t1[2]);
		dev_info(dev->mt76.dev,
			 "DW4 = 0x%08x, DW5 = 0x%08x, DW6 = 0x%08x\n\n",
			 raw_t1[3], raw_t1[4], raw_t1[5]);
		dev_info(dev->mt76.dev, "PFMU ID = %d              Invalid status = %d\n",
			 tag->t1.pfmu_idx, tag->t1.invalid_prof);
		dev_info(dev->mt76.dev, "iBf/eBf = %d\n\n", tag->t1.ebf);
		dev_info(dev->mt76.dev, "DBW   = %d\n", tag->t1.data_bw);
		dev_info(dev->mt76.dev, "SU/MU = %d\n", tag->t1.is_mu);
		dev_info(dev->mt76.dev,
			 "nrow = %d, ncol = %d, ng = %d, LM = %d, CodeBook = %d MobCalEn = %d\n",
			 tag->t1.nr, tag->t1.nc, tag->t1.ngroup, tag->t1.lm, tag->t1.codebook,
			 tag->t1.mob_cal_en);

		if (tag->t1.lm <= BF_LM_HE)
			dev_info(dev->mt76.dev, "RU start = %d, RU end = %d\n",
				 tag->t1.field.ru_start_id, tag->t1.field.ru_end_id);
		else
			dev_info(dev->mt76.dev, "PartialBW = %d\n",
				 tag->t1.bw_info.partial_bw_info);

		dev_info(dev->mt76.dev, "Mem Col1 = %d, Mem Row1 = %d, Mem Col2 = %d, Mem Row2 = %d\n",
			 tag->t1.col_id1, tag->t1.row_id1, tag->t1.col_id2, tag->t1.row_id2);
		dev_info(dev->mt76.dev, "Mem Col3 = %d, Mem Row3 = %d, Mem Col4 = %d, Mem Row4 = %d\n\n",
			 tag->t1.col_id3, tag->t1.row_id3, tag->t1.col_id4, tag->t1.row_id4);
		dev_info(dev->mt76.dev,
			 "STS0_SNR = 0x%02x, STS1_SNR = 0x%02x, STS2_SNR = 0x%02x, STS3_SNR = 0x%02x\n",
			 tag->t1.snr_sts0, tag->t1.snr_sts1, tag->t1.snr_sts2, tag->t1.snr_sts3);
		dev_info(dev->mt76.dev,
			 "STS4_SNR = 0x%02x, STS5_SNR = 0x%02x, STS6_SNR = 0x%02x, STS7_SNR = 0x%02x\n",
			 tag->t1.snr_sts4, tag->t1.snr_sts5, tag->t1.snr_sts6, tag->t1.snr_sts7);
		dev_info(dev->mt76.dev, "=============================================================\n");

		dev_info(dev->mt76.dev, "=================== TXBf Profile Tag2 Info ==================\n");
		dev_info(dev->mt76.dev,
			 "DW0 = 0x%08x, DW1 = 0x%08x, DW2 = 0x%08x\n",
			 raw_t2[0], raw_t2[1], raw_t2[2]);
		dev_info(dev->mt76.dev,
			 "DW3 = 0x%08x, DW4 = 0x%08x, DW5 = 0x%08x\n\n",
			 raw_t2[3], raw_t2[4], raw_t2[5]);
		dev_info(dev->mt76.dev, "Smart antenna ID = 0x%x,  SE index = %d\n",
			 tag->t2.smart_ant, tag->t2.se_idx);
		dev_info(dev->mt76.dev, "Timeout = 0x%x\n", tag->t2.ibf_timeout);
		dev_info(dev->mt76.dev, "Desired BW = %d, Desired Ncol = %d, Desired Nrow = %d\n",
			 tag->t2.ibf_data_bw, tag->t2.ibf_nc, tag->t2.ibf_nr);
		dev_info(dev->mt76.dev, "Desired RU Allocation = %d\n", tag->t2.ibf_ru);
		dev_info(dev->mt76.dev, "Mobility DeltaT = %d, Mobility LQ = %d\n",
			 tag->t2.mob_delta_t, tag->t2.mob_lq_result);
		dev_info(dev->mt76.dev, "=============================================================\n");
		break;
	}
	case UNI_EVENT_BF_STAREC: {

		struct mt7996_mcu_bf_starec_read *r;

		r = (struct mt7996_mcu_bf_starec_read *)skb->data;
		dev_info(dev->mt76.dev, "=================== BF StaRec ===================\n"
					"rStaRecBf.u2PfmuId      = %d\n"
					"rStaRecBf.fgSU_MU       = %d\n"
					"rStaRecBf.u1TxBfCap     = %d\n"
					"rStaRecBf.ucSoundingPhy = %d\n"
					"rStaRecBf.ucNdpaRate    = %d\n"
					"rStaRecBf.ucNdpRate     = %d\n"
					"rStaRecBf.ucReptPollRate= %d\n"
					"rStaRecBf.ucTxMode      = %d\n"
					"rStaRecBf.ucNc          = %d\n"
					"rStaRecBf.ucNr          = %d\n"
					"rStaRecBf.ucCBW         = %d\n"
					"rStaRecBf.ucMemRequire20M = %d\n"
					"rStaRecBf.ucMemRow0     = %d\n"
					"rStaRecBf.ucMemCol0     = %d\n"
					"rStaRecBf.ucMemRow1     = %d\n"
					"rStaRecBf.ucMemCol1     = %d\n"
					"rStaRecBf.ucMemRow2     = %d\n"
					"rStaRecBf.ucMemCol2     = %d\n"
					"rStaRecBf.ucMemRow3     = %d\n"
					"rStaRecBf.ucMemCol3     = %d\n",
					r->pfmu_id,
					r->is_su_mu,
					r->txbf_cap,
					r->sounding_phy,
					r->ndpa_rate,
					r->ndp_rate,
					r->rpt_poll_rate,
					r->tx_mode,
					r->nc,
					r->nr,
					r->bw,
					r->mem_require_20m,
					r->mem_row0,
					r->mem_col0,
					r->mem_row1,
					r->mem_col1,
					r->mem_row2,
					r->mem_col2,
					r->mem_row3,
					r->mem_col3);

		dev_info(dev->mt76.dev, "rStaRecBf.u2SmartAnt    = 0x%x\n"
					"rStaRecBf.ucSEIdx       = %d\n"
					"rStaRecBf.uciBfTimeOut  = 0x%x\n"
					"rStaRecBf.uciBfDBW      = %d\n"
					"rStaRecBf.uciBfNcol     = %d\n"
					"rStaRecBf.uciBfNrow     = %d\n"
					"rStaRecBf.nr_bw160      = %d\n"
					"rStaRecBf.nc_bw160 	  = %d\n"
					"rStaRecBf.ru_start_idx  = %d\n"
					"rStaRecBf.ru_end_idx 	  = %d\n"
					"rStaRecBf.trigger_su 	  = %d\n"
					"rStaRecBf.trigger_mu 	  = %d\n"
					"rStaRecBf.ng16_su 	  = %d\n"
					"rStaRecBf.ng16_mu 	  = %d\n"
					"rStaRecBf.codebook42_su = %d\n"
					"rStaRecBf.codebook75_mu = %d\n"
					"rStaRecBf.he_ltf 	      = %d\n"
					"rStaRecBf.pp_fd_val 	  = %d\n"
					"======================================\n",
					r->smart_ant,
					r->se_idx,
					r->bf_timeout,
					r->bf_dbw,
					r->bf_ncol,
					r->bf_nrow,
					r->nr_lt_bw80,
					r->nc_lt_bw80,
					r->ru_start_idx,
					r->ru_end_idx,
					r->trigger_su,
					r->trigger_mu,
					r->ng16_su,
					r->ng16_mu,
					r->codebook42_su,
					r->codebook75_mu,
					r->he_ltf,
					r->pp_fd_val);
		break;
	}
	case UNI_EVENT_BF_FBK_INFO: {
		struct mt7996_mcu_txbf_fbk_info *info;
		__le32 total, i;

		info = (struct mt7996_mcu_txbf_fbk_info *)skb->data;

		total = info->u4PFMUWRDoneCnt + info->u4PFMUWRFailCnt;
		total += info->u4PFMUWRTimeoutFreeCnt + info->u4FbRptPktDropCnt;

		dev_info(dev->mt76.dev, "\n");
		dev_info(dev->mt76.dev, "\x1b[32m =================================\x1b[m\n");
		dev_info(dev->mt76.dev, "\x1b[32m PFMUWRDoneCnt              = %u\x1b[m\n",
			info->u4PFMUWRDoneCnt);
		dev_info(dev->mt76.dev, "\x1b[32m PFMUWRFailCnt              = %u\x1b[m\n",
			info->u4PFMUWRFailCnt);
		dev_info(dev->mt76.dev, "\x1b[32m PFMUWRTimeOutCnt           = %u\x1b[m\n",
			info->u4PFMUWRTimeOutCnt);
		dev_info(dev->mt76.dev, "\x1b[32m PFMUWRTimeoutFreeCnt       = %u\x1b[m\n",
			info->u4PFMUWRTimeoutFreeCnt);
		dev_info(dev->mt76.dev, "\x1b[32m FbRptPktDropCnt            = %u\x1b[m\n",
			info->u4FbRptPktDropCnt);
		dev_info(dev->mt76.dev, "\x1b[32m TotalFbRptPkt              = %u\x1b[m\n", total);
		dev_info(dev->mt76.dev, "\x1b[32m PollPFMUIntrStatTimeOut    = %u(micro-sec)\x1b[m\n",
			info->u4PollPFMUIntrStatTimeOut);
		dev_info(dev->mt76.dev, "\x1b[32m FbRptDeQInterval           = %u(milli-sec)\x1b[m\n",
			info->u4DeQInterval);
		dev_info(dev->mt76.dev, "\x1b[32m PktCntInFbRptTimeOutQ      = %u\x1b[m\n",
			info->u4RptPktTimeOutListNum);
		dev_info(dev->mt76.dev, "\x1b[32m PktCntInFbRptQ             = %u\x1b[m\n",
			info->u4RptPktListNum);

		// [ToDo] Check if it is valid entry
		for (i = 0; ((i < 5) && (i < CFG_BF_STA_REC_NUM)); i++) {

			// [ToDo] AID needs to be refined
			dev_info(dev->mt76.dev,"\x1b[32m AID%u  RxFbRptCnt           = %u\x1b[m\n"
				, i, info->au4RxPerStaFbRptCnt[i]);
		}

		break;
	}
	case UNI_EVENT_BF_TXSND_INFO: {
		struct mt7996_mcu_tx_snd_info *info;
		struct uni_event_bf_txsnd_sta_info *snd_sta_info;
		int Idx;
		int max_wtbl_size = mt7996_wtbl_size(dev);

		info = (struct mt7996_mcu_tx_snd_info *)skb->data;
		dev_info(dev->mt76.dev, "=================== Global Setting ===================\n");

		dev_info(dev->mt76.dev, "VhtOpt = 0x%02X, HeOpt = 0x%02X, GloOpt = 0x%02X\n",
			info->vht_opt, info->he_opt, info->glo_opt);

		for (Idx = 0; Idx < BF_SND_CTRL_STA_DWORD_CNT; Idx++) {
			dev_info(dev->mt76.dev, "SuSta[%d] = 0x%08X,", Idx,
				 info->snd_rec_su_sta[Idx]);
			if ((Idx & 0x03) == 0x03)
				dev_info(dev->mt76.dev, "\n");
		}

		if ((Idx & 0x03) != 0x03)
			dev_info(dev->mt76.dev, "\n");


		for (Idx = 0; Idx < BF_SND_CTRL_STA_DWORD_CNT; Idx++) {
			dev_info(dev->mt76.dev, "VhtMuSta[%d] = 0x%08X,", Idx, info->snd_rec_vht_mu_sta[Idx]);
			if ((Idx & 0x03) == 0x03)
				dev_info(dev->mt76.dev, "\n");
		}

		if ((Idx & 0x03) != 0x03)
			dev_info(dev->mt76.dev, "\n");

		for (Idx = 0; Idx < BF_SND_CTRL_STA_DWORD_CNT; Idx++) {
			dev_info(dev->mt76.dev, "HeTBSta[%d] = 0x%08X,", Idx, info->snd_rec_he_tb_sta[Idx]);
			if ((Idx & 0x03) == 0x03)
				dev_info(dev->mt76.dev, "\n");
		}

		if ((Idx & 0x03) != 0x03)
			dev_info(dev->mt76.dev, "\n");

		for (Idx = 0; Idx < BF_SND_CTRL_STA_DWORD_CNT; Idx++) {
			dev_info(dev->mt76.dev, "EhtTBSta[%d] = 0x%08X,", Idx, info->snd_rec_eht_tb_sta[Idx]);
			if ((Idx & 0x03) == 0x03)
				dev_info(dev->mt76.dev, "\n");
		}

		if ((Idx & 0x03) != 0x03)
			dev_info(dev->mt76.dev, "\n");

		for (Idx = 0; Idx < CFG_WIFI_RAM_BAND_NUM; Idx++) {
			dev_info(dev->mt76.dev, "Band%u:\n", Idx);
			dev_info(dev->mt76.dev, "	 Wlan Idx For VHT MC Sounding = %u\n", info->wlan_idx_for_mc_snd[Idx]);
			dev_info(dev->mt76.dev, "	 Wlan Idx For HE TB Sounding = %u\n", info->wlan_idx_for_he_tb_snd[Idx]);
			dev_info(dev->mt76.dev, "	 Wlan Idx For EHT TB Sounding = %u\n", info->wlan_idx_for_eht_tb_snd[Idx]);
		}

		dev_info(dev->mt76.dev, "ULLen = %d, ULMcs = %d, ULLDCP = %d\n",
			info->ul_length, info->mcs, info->ldpc);

		dev_info(dev->mt76.dev, "=================== STA Info ===================\n");

		for (Idx = 1; (Idx < 5 && (Idx < CFG_BF_STA_REC_NUM)); Idx++) {
			snd_sta_info = &info->snd_sta_info[Idx];
			dev_info(dev->mt76.dev, "Idx%2u Interval = %d, interval counter = %d, TxCnt = %d, StopReason = 0x%02X\n",
				Idx,
				snd_sta_info->snd_intv,
				snd_sta_info->snd_intv_cnt,
				snd_sta_info->snd_tx_cnt,
				snd_sta_info->snd_stop_reason);
		}

		dev_info(dev->mt76.dev, "=================== STA Info Connected ===================\n");
		// [ToDo] How to iterate and get AID info of station
		// Check UniEventBFCtrlTxSndHandle() on Logan

		//hardcode max_wtbl_size as 5
		max_wtbl_size = 5;
		for (Idx = 1; ((Idx < max_wtbl_size) && (Idx < CFG_BF_STA_REC_NUM)); Idx++) {

			// [ToDo] We do not show AID info here
			snd_sta_info = &info->snd_sta_info[Idx];
			dev_info(dev->mt76.dev, " Interval = %d (%u ms), interval counter = %d (%u ms), TxCnt = %d, StopReason = 0x%02X\n",
				snd_sta_info->snd_intv,
				snd_sta_info->snd_intv * 10,
				snd_sta_info->snd_intv_cnt,
				snd_sta_info->snd_intv_cnt * 10,
				snd_sta_info->snd_tx_cnt,
				snd_sta_info->snd_stop_reason);
		}

		dev_info(dev->mt76.dev, "======================================\n");

		break;
	}
	case UNI_EVENT_BF_CAL_PHASE: {
		struct mt7996_ibf_cal_info *cal;
		struct mt7996_txbf_phase_out phase_out;
		struct mt7996_txbf_phase *phase;

		cal = (struct mt7996_ibf_cal_info *)skb->data;
		phase = (struct mt7996_txbf_phase *)dev->test.txbf_phase_cal;
		memcpy(&phase_out, &cal->phase_out, sizeof(phase_out));
		switch (cal->cal_type) {
		case IBF_PHASE_CAL_NORMAL:
		case IBF_PHASE_CAL_NORMAL_INSTRUMENT:
			/* Only calibrate group M */
			if (cal->group_l_m_n != GROUP_M)
				break;
			phase = &phase[cal->group];
			phase->status = cal->status;
			dev_info(dev->mt76.dev, "Calibrated result = %d\n", phase->status);
			dev_info(dev->mt76.dev, "Group %d and Group M\n", cal->group);
			mt7996_ibf_phase_assign(dev, cal, phase);
			break;
		case IBF_PHASE_CAL_VERIFY:
		case IBF_PHASE_CAL_VERIFY_INSTRUMENT:
			dev_info(dev->mt76.dev, "Verification result = %d\n", cal->status);
			break;
		default:
			break;
		}

		dev_info(dev->mt76.dev, "c0_uh = %d, c1_uh = %d, c2_uh = %d, c3_uh = %d\n",
			 phase_out.c0_uh, phase_out.c1_uh, phase_out.c2_uh, phase_out.c3_uh);
		dev_info(dev->mt76.dev, "c0_h = %d, c1_h = %d, c2_h = %d, c3_h = %d\n",
			 phase_out.c0_h, phase_out.c1_h, phase_out.c2_h, phase_out.c3_h);
		dev_info(dev->mt76.dev, "c0_mh = %d, c1_mh = %d, c2_mh = %d, c3_mh = %d\n",
			 phase_out.c0_mh, phase_out.c1_mh, phase_out.c2_mh, phase_out.c3_mh);
		dev_info(dev->mt76.dev, "c0_m = %d, c1_m = %d, c2_m = %d, c3_m = %d\n",
			 phase_out.c0_m, phase_out.c1_m, phase_out.c2_m, phase_out.c3_m);
		dev_info(dev->mt76.dev, "c0_l = %d, c1_l = %d, c2_l = %d, c3_l = %d\n",
			 phase_out.c0_l, phase_out.c1_l, phase_out.c2_l, phase_out.c3_l);

		break;
	}
	default:
		dev_info(dev->mt76.dev, "%s: unknown bf event tag %d\n",
			 __func__, event->tag);
	}

}


int mt7996_mcu_set_muru_fixed_rate_enable(struct mt7996_dev *dev, u8 action, int val)
{
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		__le16 value;
		__le16 rsv;
	} __packed data = {
		.tag = cpu_to_le16(action),
		.len = cpu_to_le16(sizeof(data) - 4),
		.value = cpu_to_le16(!!val),
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &data, sizeof(data),
				 false);
}

int mt7996_mcu_set_muru_fixed_rate_parameter(struct mt7996_dev *dev, u8 action, void *para)
{
	char *buf = (char *)para;
	u8 num_user = 0, recv_arg = 0, max_mcs = 0, usr_mcs[4] = {0};
	__le16 bw;
	int i;
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		u8 cmd_version;
		u8 cmd_revision;
		__le16 rsv;

		struct uni_muru_mum_set_group_tbl_entry entry;
	} __packed data = {
		.tag = cpu_to_le16(action),
		.len = cpu_to_le16(sizeof(data) - 4),
	};

#define __RUALLOC_TYPE_CHECK_HE(BW) ((BW == RUALLOC_BW20) || (BW == RUALLOC_BW40) || (BW == RUALLOC_BW80) || (BW == RUALLOC_BW160))
#define __RUALLOC_TYPE_CHECK_EHT(BW) (__RUALLOC_TYPE_CHECK_HE(BW) || (BW == RUALLOC_BW320))
	/* [Num of user] - 1~4
	 * [RUAlloc] - BW320: 395, BW160: 137, BW80: 134, BW40: 130, BW20: 122
	 * [LTF/GI] - For VHT, short GI: 0, Long GI: 1; 	 *
	 * For HE/EHT, 4xLTF+3.2us: 0, 4xLTF+0.8us: 1, 2xLTF+0.8us:2
	 * [Phy/FullBW] - VHT: 0 / HEFullBw: 1 / HEPartialBw: 2 / EHTFullBW: 3, EHTPartialBW: 4
	 * [DL/UL] DL: 0, UL: 1, DL_UL: 2
	 * [Wcid User0] - WCID 0
	 * [MCS of WCID0] - For HE/VHT, 0-11: 1ss MCS0-MCS11, 12-23: 2SS MCS0-MCS11
	 * For EHT, 0-13: 1ss MCS0-MCS13, 14-27: 2SS MCS0-MCS13
	 * [WCID 1]
	 * [MCS of WCID1]
	 * [WCID 2]
	 * [MCS of WCID2]
	 * [WCID 3]
	 * [MCS of WCID3]
	 */

	recv_arg = sscanf(buf, "%hhu %hu %hhu %hhu %hhu %hu %hhu %hu %hhu %hu %hhu %hu %hhu",
			  &num_user, &bw, &data.entry.gi, &data.entry.capa, &data.entry.dl_ul,
			  &data.entry.wlan_idx0, &usr_mcs[0],
			  &data.entry.wlan_idx1, &usr_mcs[1],
			  &data.entry.wlan_idx2, &usr_mcs[2],
			  &data.entry.wlan_idx3, &usr_mcs[3]);

	if (recv_arg != (5 + (2 * num_user))) {
		dev_err(dev->mt76.dev, "The number of argument is invalid\n");
		goto error;
	}

	if (num_user > 0 && num_user < 5)
		data.entry.num_user = num_user - 1;
	else {
		dev_err(dev->mt76.dev, "The number of user count is invalid\n");
		goto error;
	}

	/**
	 * Older chip shall be set as HE. Refer to getHWSupportByChip() in Logan
	 * driver to know the value for differnt chips
	 */
	data.cmd_version = UNI_CMD_MURU_VER_EHT;

	if (data.cmd_version == UNI_CMD_MURU_VER_EHT)
		max_mcs = UNI_MAX_MCS_SUPPORT_EHT;
	else
		max_mcs = UNI_MAX_MCS_SUPPORT_HE;


	// Parameter Check
	if (data.cmd_version != UNI_CMD_MURU_VER_EHT) {
		if ((data.entry.capa > MAX_MODBF_HE) || (bw == RUALLOC_BW320))
			goto error;
	} else {
		if ((data.entry.capa <= MAX_MODBF_HE) && (bw == RUALLOC_BW320))
			goto error;
	}

	if (data.entry.capa <= MAX_MODBF_HE)
		max_mcs = UNI_MAX_MCS_SUPPORT_HE;

	if (__RUALLOC_TYPE_CHECK_EHT(bw)) {
		data.entry.ru_alloc = (u8)(bw & 0xFF);
		if (bw == RUALLOC_BW320)
			data.entry.ru_alloc_ext = (u8)(bw >> 8);
	} else {
		dev_err(dev->mt76.dev, "RU_ALLOC argument is invalid\n");
		goto error;
	}

	if ((data.entry.gi > 2) ||
	    ((data.entry.gi > 1) && (data.entry.capa == MAX_MODBF_VHT))) {
		dev_err(dev->mt76.dev, "GI argument is invalid\n");
		goto error;
	}

	if (data.entry.dl_ul > 2) {
		dev_err(dev->mt76.dev, "DL_UL argument is invalid\n");
		goto error;
	}

#define __mcs_handler(_n)							\
	do {									\
		if (usr_mcs[_n] > max_mcs) {					\
			usr_mcs[_n] -= (max_mcs + 1);				\
			data.entry.nss##_n = 1;					\
			if (usr_mcs[_n] > max_mcs)				\
				usr_mcs[_n] = max_mcs;				\
		}								\
		if ((data.entry.dl_ul & 0x1) == 0)				\
			data.entry.dl_mcs_user##_n = usr_mcs[_n];		\
		if ((data.entry.dl_ul & 0x3) > 0)				\
			data.entry.ul_mcs_user##_n = usr_mcs[_n];		\
	}									\
	while (0)

	for (i=0; i<= data.entry.num_user; i++) {
		switch (i) {
			case 0:
				__mcs_handler(0);
				break;
			case 1:
				__mcs_handler(1);
				break;
			case 2:
				__mcs_handler(2);
				break;
			case 3:
				__mcs_handler(3);
				break;
			default:
				break;
		}
	}
#undef __mcs_handler


	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &data,
				 sizeof(data), false);

error:
	dev_err(dev->mt76.dev, "Command failed!\n");
	return -EINVAL;
}

/**
 * This function can be used to build the following commands
 * MURU_SUTX_CTRL (0x10)
 * SET_FORCE_MU (0x33)
 * SET_MUDL_ACK_POLICY (0xC8)
 * SET_TRIG_TYPE (0xC9)
 * SET_20M_DYN_ALGO (0xCA)
 * SET_CERT_MU_EDCA_OVERRIDE (0xCD)
 */
int mt7996_mcu_set_muru_cmd(struct mt7996_dev *dev, u16 action, int val)
{
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		u8 config;
		u8 rsv[3];
	} __packed data = {
		.tag = cpu_to_le16(action),
		.len = cpu_to_le16(sizeof(data) - 4),
		.config = (u8) val,
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &data, sizeof(data),
				 false);
}

int mt7996_mcu_muru_set_prot_frame_thr(struct mt7996_dev *dev, u32 val)
{
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		__le32 prot_frame_thr;
	} __packed data = {
		.tag = cpu_to_le16(UNI_CMD_MURU_PROT_FRAME_THR),
		.len = cpu_to_le16(sizeof(data) - 4),
		.prot_frame_thr = cpu_to_le32(val),
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &data, sizeof(data),
				 false);
}

int mt7996_mcu_set_bypass_smthint(struct mt7996_phy *phy, u8 val)
{
#define BF_PHY_SMTH_INT_BYPASS 0
#define BYPASS_VAL 1
	struct mt7996_dev *dev = phy->dev;
	struct {
		u8 _rsv[4];

		u16 tag;
		u16 len;

		u8 action;
		u8 band_idx;
		u8 smthintbypass;
		u8 __rsv2[5];
	} __packed data = {
		.tag = cpu_to_le16(BF_CFG_PHY),
		.len = cpu_to_le16(sizeof(data) - 4),
		.action = BF_PHY_SMTH_INT_BYPASS,
		.band_idx = phy->mt76->band_idx,
		.smthintbypass = val,
	};

	if (val != BYPASS_VAL)
		return -EINVAL;

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(BF), &data, sizeof(data),
				 true);
}

int mt7996_mcu_set_bsrp_ctrl(struct mt7996_phy *phy, u16 interval,
			     u16 ru_alloc, u32 trig_type, u8 trig_flow, u8 ext_cmd)
{
	struct mt7996_dev *dev = phy->dev;
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		__le16 interval;
		__le16 ru_alloc;
		__le32 trigger_type;
		u8 trigger_flow;
		u8 ext_cmd_bsrp;
		u8 band_bitmap;
		u8 _rsv2;
	} __packed req = {
		.tag = cpu_to_le16(UNI_CMD_MURU_BSRP_CTRL),
		.len = cpu_to_le16(sizeof(req) - 4),
		.interval = cpu_to_le16(interval),
		.ru_alloc = cpu_to_le16(ru_alloc),
		.trigger_type = cpu_to_le32(trig_type),
		.trigger_flow = trig_flow,
		.ext_cmd_bsrp = ext_cmd,
		.band_bitmap = BIT(phy->mt76->band_idx),
	};

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &req,
				 sizeof(req), false);
}

int mt7996_mcu_set_rfeature_trig_type(struct mt7996_phy *phy, u8 enable, u8 trig_type)
{
	struct mt7996_dev *dev = phy->dev;
	int ret = 0;
	char buf[] = "01:00:00:1B";

	if (enable) {
		ret = mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_TRIG_TYPE, trig_type);
		if (ret)
			return ret;
	}

	switch (trig_type) {
	case CAPI_BASIC:
		return mt7996_mcu_set_bsrp_ctrl(phy, 5, 67, 0, 0, enable);
	case CAPI_BRP:
		return mt7996_mcu_set_txbf_snd_info(phy, buf);
	case CAPI_MU_BAR:
		return mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_MUDL_ACK_POLICY,
					       MU_DL_ACK_POLICY_MU_BAR);
	case CAPI_BSRP:
		return mt7996_mcu_set_bsrp_ctrl(phy, 5, 67, 4, 0, enable);
	default:
		return 0;
	}
}

int mt7996_mcu_set_muru_cfg(struct mt7996_phy *phy, void *data)
{
	struct mt7996_dev *dev = phy->dev;
	struct mt7996_muru *muru;
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;

		u8 version;
		u8 revision;
		u8 _rsv2[2];

		struct mt7996_muru muru;
	} __packed req = {
		.tag = cpu_to_le16(UNI_CMD_MURU_MUNUAL_CONFIG),
		.len = cpu_to_le16(sizeof(req) - 4),
		.version = UNI_CMD_MURU_VER_EHT,
	};

	muru = (struct mt7996_muru *) data;
	memcpy(&req.muru, muru, sizeof(struct mt7996_muru));

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(MURU), &req,
				 sizeof(req), false);
}

int mt7996_set_muru_cfg(struct mt7996_phy *phy, u8 action, u8 val)
{
	struct mt7996_muru *muru;
	struct mt7996_muru_dl *dl;
	struct mt7996_muru_ul *ul;
	struct mt7996_muru_comm *comm;
	int ret = 0;

	muru = kzalloc(sizeof(struct mt7996_muru), GFP_KERNEL);
	dl = &muru->dl;
	ul = &muru->ul;
	comm = &muru->comm;

	switch (action) {
	case MU_CTRL_DL_USER_CNT:
		dl->user_num = val;
		comm->ppdu_format = MURU_PPDU_HE_MU;
		comm->sch_type = MURU_OFDMA_SCH_TYPE_DL;
		muru->cfg_comm = cpu_to_le32(MURU_COMM_SET);
		muru->cfg_dl = cpu_to_le32(MURU_FIXED_DL_TOTAL_USER_CNT);
		ret = mt7996_mcu_set_muru_cfg(phy, muru);
		break;
	case MU_CTRL_UL_USER_CNT:
		ul->user_num = val;
		comm->ppdu_format = MURU_PPDU_HE_TRIG;
		comm->sch_type = MURU_OFDMA_SCH_TYPE_UL;
		muru->cfg_comm = cpu_to_le32(MURU_COMM_SET);
		muru->cfg_ul = cpu_to_le32(MURU_FIXED_UL_TOTAL_USER_CNT);
		ret = mt7996_mcu_set_muru_cfg(phy, muru);
		break;
	default:
		break;
	}

	kfree(muru);
	return ret;
}

void mt7996_mcu_set_ppdu_tx_type(struct mt7996_phy *phy, u8 ppdu_type)
{
	struct mt7996_dev *dev = phy->dev;
	int enable_su;

	switch (ppdu_type) {
	case CAPI_SU:
		enable_su = 1;
		mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SUTX_CTRL, enable_su);
		mt7996_set_muru_cfg(phy, MU_CTRL_DL_USER_CNT, 0);
		break;
	case CAPI_MU:
		enable_su = 0;
		mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SUTX_CTRL, enable_su);
		break;
	default:
		break;
	}
}

void mt7996_mcu_set_nusers_ofdma(struct mt7996_phy *phy, u8 type, u8 user_cnt)
{
	struct mt7996_dev *dev = phy->dev;
	int enable_su = 0;

	mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SUTX_CTRL, enable_su);
	mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_MUDL_ACK_POLICY, MU_DL_ACK_POLICY_SU_BAR);
	mt7996_mcu_muru_set_prot_frame_thr(dev, 9999);

	mt7996_set_muru_cfg(phy, type, user_cnt);
}

void mt7996_mcu_set_mimo(struct mt7996_phy *phy)
{
	struct mt7996_dev *dev = phy->dev;
	struct cfg80211_chan_def *chandef = &phy->mt76->chandef;
	int disable_ra = 1;
	char buf[] = "2 134 0 1 0 1 2 2 2";
	int force_mu = 1;

	switch (chandef->width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
	case NL80211_CHAN_WIDTH_20:
		strscpy(buf, "2 122 0 1 0 1 2 2 2", sizeof(buf));
		break;
	case NL80211_CHAN_WIDTH_80:
		break;
	case NL80211_CHAN_WIDTH_160:
		strscpy(buf, "2 137 0 1 0 1 2 2 2", sizeof(buf));
		break;
	default:
		break;
	}

	mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_MUDL_ACK_POLICY, MU_DL_ACK_POLICY_SU_BAR);
	mt7996_mcu_set_muru_fixed_rate_enable(dev, UNI_CMD_MURU_FIXED_RATE_CTRL, disable_ra);
	mt7996_mcu_set_muru_fixed_rate_parameter(dev, UNI_CMD_MURU_FIXED_GROUP_RATE_CTRL, buf);
	mt7996_mcu_set_muru_cmd(dev, UNI_CMD_MURU_SET_FORCE_MU, force_mu);
}

void mt7996_mcu_set_cert(struct mt7996_phy *phy, u8 type)
{
	struct mt7996_dev *dev = phy->dev;
	struct {
		u8 _rsv[4];

		__le16 tag;
		__le16 len;
		u8 action;
		u8 _rsv2[3];
	} __packed req = {
		.tag = cpu_to_le16(UNI_CMD_CERT_CFG),
		.len = cpu_to_le16(sizeof(req) - 4),
		.action = type, /* 1: CAPI Enable */
	};

	mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(WSYS_CONFIG), &req,
			  sizeof(req), false);
}

int mt7996_mcu_set_vow_drr_dbg(struct mt7996_dev *dev, u32 val)
{
#define MT7996_VOW_DEBUG_MODE	0xe
	struct {
		u8 __rsv1[4];

		__le16 tag;
		__le16 len;
		u8 __rsv2[4];
		__le32 action;
		__le32 val;
		u8 __rsv3[8];
	} __packed req = {
		.tag = cpu_to_le16(UNI_VOW_DRR_CTRL),
		.len = cpu_to_le16(sizeof(req) - 4),
		.action = cpu_to_le32(MT7996_VOW_DEBUG_MODE),
		.val = cpu_to_le32(val),
	};

	if (val & ~VOW_DRR_DBG_FLAGS)
		return -EINVAL;

	return mt76_mcu_send_msg(&dev->mt76, MCU_WM_UNI_CMD(VOW), &req,
				 sizeof(req), true);
}

#endif
