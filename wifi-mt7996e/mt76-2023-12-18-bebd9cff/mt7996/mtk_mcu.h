/* SPDX-License-Identifier: ISC */
/*
 * Copyright (C) 2023 MediaTek Inc.
 */

#ifndef __MT7996_MTK_MCU_H
#define __MT7996_MTK_MCU_H

#include "../mt76_connac_mcu.h"

#ifdef CONFIG_MTK_DEBUG

enum {
	UNI_CMD_MURU_DBG_INFO = 0x18,
};

struct txpower_basic_info {
	u8 category;
	u8 rsv1;

	/* basic info */
	u8 band_idx;
	u8 band;

	/* board type info */
	bool is_epa;
	bool is_elna;

	/* power percentage info */
	bool percentage_ctrl_enable;
	s8 power_drop_level;

	/* frond-end loss TX info */
	s8 front_end_loss_tx[4];

	/* frond-end loss RX info */
	s8 front_end_loss_rx[4];

	/* thermal info */
	bool thermal_compensate_enable;
	s8 thermal_compensate_value;
	u8 rsv2;

	/* TX power max/min limit info */
	s8 max_power_bound;
	s8 min_power_bound;

	/* power limit info */
	bool sku_enable;
	bool bf_backoff_enable;

	/* MU TX power info */
	bool mu_tx_power_manual_enable;
	s8 mu_tx_power_auto;
	s8 mu_tx_power_manual;
	u8 rsv3;
};

struct txpower_phy_rate_info {
	u8 category;
	u8 band_idx;
	u8 band;
	u8 epa_gain;

	/* rate power info [dBm] */
	s8 frame_power[MT7996_SKU_RATE_NUM][__MT_MAX_BAND];

	/* TX power max/min limit info */
	s8 max_power_bound;
	s8 min_power_bound;
	u8 rsv1;
};

struct txpower_backoff_table_info {
	u8 category;
	u8 band_idx;
	u8 band;
	u8 backoff_en;

	s8 frame_power[MT7996_SKU_PATH_NUM];
	u8 rsv[3];
};

struct mt7996_mcu_txpower_event {
	u8 _rsv[4];

	__le16 tag;
	__le16 len;

	union {
		struct txpower_basic_info basic_info;
		struct txpower_phy_rate_info phy_rate_info;
		struct txpower_backoff_table_info backoff_table_info;
	};
};

enum txpower_category {
	BASIC_INFO,
	BACKOFF_TABLE_INFO,
	PHY_RATE_INFO,
};

enum txpower_event {
	UNI_TXPOWER_BASIC_INFO = 0,
	UNI_TXPOWER_BACKOFF_TABLE_SHOW_INFO = 3,
	UNI_TXPOWER_PHY_RATE_INFO = 5,
};

enum {
	EDCCA_CTRL_SET_EN = 0,
	EDCCA_CTRL_SET_THRES,
	EDCCA_CTRL_GET_EN,
	EDCCA_CTRL_GET_THRES,
	EDCCA_CTRL_NUM,
};

enum {
	EDCCA_DEFAULT = 0,
	EDCCA_FCC = 1,
	EDCCA_ETSI = 2,
	EDCCA_JAPAN = 3
};

enum {
	UNI_CMD_MURU_BSRP_CTRL = 0x01,
	UNI_CMD_MURU_SUTX_CTRL = 0x10,
	UNI_CMD_MURU_FIXED_RATE_CTRL = 0x11,
	UNI_CMD_MURU_FIXED_GROUP_RATE_CTRL = 0x12,
	UNI_CMD_MURU_SET_FORCE_MU = 0x33,
	UNI_CMD_MURU_MUNUAL_CONFIG = 0x64,
	UNI_CMD_MURU_SET_MUDL_ACK_POLICY = 0xC8,
	UNI_CMD_MURU_SET_TRIG_TYPE = 0xC9,
	UNI_CMD_MURU_SET_20M_DYN_ALGO = 0xCA,
	UNI_CMD_MURU_PROT_FRAME_THR = 0xCC,
	UNI_CMD_MURU_SET_CERT_MU_EDCA_OVERRIDE,
};

struct bf_pfmu_tag {
	__le16 tag;
	__le16 len;

	u8 pfmu_id;
	bool bfer;
	u8 band_idx;
	u8 __rsv[5];
	u8 buf[56];
} __packed;

struct bf_starec_read {
	__le16 tag;
	__le16 len;

	__le16 wlan_idx;
	u8 __rsv[2];
} __packed;

struct bf_fbk_rpt_info {
	__le16 tag;
	__le16 len;

	__le16 wlan_idx; // Only need for dynamic_pfmu_update 0x4
	u8 action;
	u8 band_idx;
	u8 __rsv[4];

} __packed;

struct bf_txsnd_info {
	__le16 tag;
	__le16 len;

	u8 action;
	u8 read_clr;
	u8 vht_opt;
	u8 he_opt;
	__le16 wlan_idx;
	u8 glo_opt;
	u8 snd_intv;
	u8 snd_stop;
	u8 max_snd_stas;
	u8 tx_time;
	u8 mcs;
	u8 ldpc;
	u8 inf;
	u8 man;
	u8 ac_queue;
	u8 sxn_protect;
	u8 direct_fbk;
	u8 __rsv[2];
} __packed;

#define MAX_PHASE_GROUP_NUM	9

struct bf_phase_comp {
	__le16 tag;
	__le16 len;

	u8 bw;
	u8 jp_band;
	u8 band_idx;
	bool read_from_e2p;
	bool disable;
	u8 group;
	u8 rsv[2];
	u8 buf[44];
} __packed;

struct bf_tx_apply {
	__le16 tag;
	__le16 len;

	__le16 wlan_idx;
	bool ebf;
	bool ibf;
	bool mu_txbf;
	bool phase_cal;
	u8 rsv[2];
} __packed;

struct bf_phase_cal {
	__le16 tag;
	__le16 len;

	u8 group_l_m_n;
	u8 group;
	u8 sx2;
	u8 cal_type;
	u8 lna_gain_level;
	u8 band_idx;
	u8 rsv[2];
} __packed;

struct bf_txcmd {
	__le16 tag;
	__le16 len;

	u8 action;
	u8 bf_manual;
	u8 bf_bit;
	u8 rsv[5];
} __packed;

struct bf_pfmu_data_all {
	__le16 tag;
	__le16 len;

	u8 pfmu_id;
	u8 band_idx;
	u8 rsv[2];

	u8 buf[512];
} __packed;

#define TXBF_DUT_MAC_SUBADDR		0x22
#define TXBF_GOLDEN_MAC_SUBADDR		0x11

struct mt7996_tm_bf_req {
	u8 _rsv[4];

	union {
		struct bf_sounding_on sounding;
		struct bf_tx_apply tx_apply;
		struct bf_pfmu_tag pfmu_tag;
		struct bf_pfmu_data_all pfmu_data_all;
		struct bf_phase_cal phase_cal;
		struct bf_phase_comp phase_comp;
		struct bf_txcmd txcmd;
	};
} __packed;

enum tm_trx_mac_type {
	TM_TRX_MAC_TX = 1,
	TM_TRX_MAC_RX,
	TM_TRX_MAC_TXRX,
	TM_TRX_MAC_TXRX_RXV,
	TM_TRX_MAC_RXV,
	TM_TRX_MAC_RX_RXV,
};

enum tm_trx_param_idx {
	TM_TRX_PARAM_RSV,
	/* MAC */
	TM_TRX_PARAM_SET_TRX,
	TM_TRX_PARAM_RX_FILTER,
	TM_TRX_PARAM_RX_FILTER_PKT_LEN,
	TM_TRX_PARAM_SLOT_TIME,
	TM_TRX_PARAM_CLEAN_PERSTA_TXQUEUE,
	TM_TRX_PARAM_AMPDU_WTBL,
	TM_TRX_PARAM_MU_RX_AID,
	TM_TRX_PARAM_PHY_MANUAL_TX,

	/* PHY */
	TM_TRX_PARAM_RX_PATH,
	TM_TRX_PARAM_TX_STREAM,
	TM_TRX_PARAM_TSSI_STATUS,
	TM_TRX_PARAM_DPD_STATUS,
	TM_TRX_PARAM_RATE_POWER_OFFSET_ON_OFF,
	TM_TRX_PARAM_THERMO_COMP_STATUS,
	TM_TRX_PARAM_FREQ_OFFSET,
	TM_TRX_PARAM_FAGC_RSSI_PATH,
	TM_TRX_PARAM_PHY_STATUS_COUNT,
	TM_TRX_PARAM_RXV_INDEX,

	TM_TRX_PARAM_ANTENNA_PORT,
	TM_TRX_PARAM_THERMAL_ONOFF,
	TM_TRX_PARAM_TX_POWER_CONTROL_ALL_RF,
	TM_TRX_PARAM_RATE_POWER_OFFSET,
	TM_TRX_PARAM_SLT_CMD_TEST,
	TM_TRX_PARAM_SKU,
	TM_TRX_PARAM_POWER_PERCENTAGE_ON_OFF,
	TM_TRX_PARAM_BF_BACKOFF_ON_OFF,
	TM_TRX_PARAM_POWER_PERCENTAGE_LEVEL,
	TM_TRX_PARAM_FRTBL_CFG,
	TM_TRX_PARAM_PREAMBLE_PUNC_ON_OFF,

	TM_TRX_PARAM_MAX_NUM,
};

enum trx_action {
	TM_TRX_ACTION_SET,
	TM_TRX_ACTION_GET,
};

struct tm_trx_set {
	u8 type;
	u8 enable;
	u8 band_idx;
	u8 rsv;
} __packed;

struct mt7996_tm_trx_req {
	u8 param_num;
	u8 _rsv[3];

	__le16 tag;
	__le16 len;

	__le16 param_idx;
	u8 band_idx;
	u8 testmode_en;
	u8 action;
	u8 rsv[3];

	u32 data;
	struct tm_trx_set set_trx;

	u8 buf[220];
} __packed;

struct mt7996_mcu_bf_basic_event {
	struct mt7996_mcu_rxd rxd;

	u8 __rsv1[4];

	__le16 tag;
	__le16 len;
};

struct mt7996_mcu_bf_starec_read {

	struct mt7996_mcu_bf_basic_event event;

	__le16 pfmu_id;
	bool is_su_mu;
	u8 txbf_cap;
	u8 sounding_phy;
	u8 ndpa_rate;
	u8 ndp_rate;
	u8 rpt_poll_rate;
	u8 tx_mode;
	u8 nc;
	u8 nr;
	u8 bw;
	u8 total_mem_require;
	u8 mem_require_20m;
	u8 mem_row0;
	u8 mem_col0:6;
	u8 mem_row0_msb:2;
	u8 mem_row1;
	u8 mem_col1:6;
	u8 mem_row1_msb:2;
	u8 mem_row2;
	u8 mem_col2:6;
	u8 mem_row2_msb:2;
	u8 mem_row3;
	u8 mem_col3:6;
	u8 mem_row3_msb:2;

	__le16 smart_ant;
	u8 se_idx;
	u8 auto_sounding_ctrl;

	u8 bf_timeout;
	u8 bf_dbw;
	u8 bf_ncol;
	u8 bf_nrow;

	u8 nr_lt_bw80;
	u8 nc_lt_bw80;
	u8 ru_start_idx;
	u8 ru_end_idx;

	bool trigger_su;
	bool trigger_mu;

	bool ng16_su;
	bool ng16_mu;

	bool codebook42_su;
	bool codebook75_mu;

	u8 he_ltf;
	u8 pp_fd_val;
};

#define TXBF_PFMU_ID_NUM_MAX 48

#define TXBF_PFMU_ID_NUM_MAX_TBTC_BAND0 TXBF_PFMU_ID_NUM_MAX
#define TXBF_PFMU_ID_NUM_MAX_TBTC_BAND1 TXBF_PFMU_ID_NUM_MAX
#define TXBF_PFMU_ID_NUM_MAX_TBTC_BAND2 TXBF_PFMU_ID_NUM_MAX

/* CFG_BF_STA_REC shall be varied based on BAND Num */
#define CFG_BF_STA_REC_NUM (TXBF_PFMU_ID_NUM_MAX_TBTC_BAND0 + TXBF_PFMU_ID_NUM_MAX_TBTC_BAND1 + TXBF_PFMU_ID_NUM_MAX_TBTC_BAND2)

#define BF_SND_CTRL_STA_DWORD_CNT   ((CFG_BF_STA_REC_NUM + 0x1F) >> 5)

#ifndef ALIGN_4
	#define ALIGN_4(_value)             (((_value) + 3) & ~3u)
#endif /* ALIGN_4 */

#define CFG_WIFI_RAM_BAND_NUM 3

struct uni_event_bf_txsnd_sta_info {
	u8 snd_intv;       /* Sounding interval upper bound, unit:15ms */
	u8 snd_intv_cnt;   /* Sounding interval counter */
	u8 snd_tx_cnt;     /* Tx sounding count for debug */
	u8 snd_stop_reason;  /* Bitwise reason to put in Stop Queue */
};

struct mt7996_mcu_tx_snd_info {

	struct mt7996_mcu_bf_basic_event event;

	u8 vht_opt;
	u8 he_opt;
	u8 glo_opt;
	u8 __rsv;
	__le32 snd_rec_su_sta[BF_SND_CTRL_STA_DWORD_CNT];
	__le32 snd_rec_vht_mu_sta[BF_SND_CTRL_STA_DWORD_CNT];
	__le32 snd_rec_he_tb_sta[BF_SND_CTRL_STA_DWORD_CNT];
	__le32 snd_rec_eht_tb_sta[BF_SND_CTRL_STA_DWORD_CNT];
	__le16 wlan_idx_for_mc_snd[ALIGN_4(CFG_WIFI_RAM_BAND_NUM)];
	__le16 wlan_idx_for_he_tb_snd[ALIGN_4(CFG_WIFI_RAM_BAND_NUM)];
	__le16 wlan_idx_for_eht_tb_snd[ALIGN_4(CFG_WIFI_RAM_BAND_NUM)];
	__le16 ul_length;
	u8 mcs;
	u8 ldpc;
	struct uni_event_bf_txsnd_sta_info snd_sta_info[CFG_BF_STA_REC_NUM];
};

struct mt7996_mcu_txbf_fbk_info {

	struct mt7996_mcu_bf_basic_event event;

	__le32 u4DeQInterval;     /* By ms */
	__le32 u4PollPFMUIntrStatTimeOut; /* micro-sec */
	__le32 u4RptPktTimeOutListNum;
	__le32 u4RptPktListNum;
	__le32 u4PFMUWRTimeOutCnt;
	__le32 u4PFMUWRFailCnt;
	__le32 u4PFMUWRDoneCnt;
	__le32 u4PFMUWRTimeoutFreeCnt;
	__le32 u4FbRptPktDropCnt;
	__le32 au4RxPerStaFbRptCnt[CFG_BF_STA_REC_NUM];
};

struct pfmu_ru_field {
	__le32 ru_start_id:7;
	__le32 _rsv1:1;
	__le32 ru_end_id:7;
	__le32 _rsv2:1;
} __packed;

struct pfmu_partial_bw_info {
	__le32 partial_bw_info:9;
	__le32 _rsv1:7;
} __packed;

struct mt7996_pfmu_tag1 {
	__le32 pfmu_idx:10;
	__le32 ebf:1;
	__le32 data_bw:3;
	__le32 lm:3;
	__le32 is_mu:1;
	__le32 nr:3;
	__le32 nc:3;
	__le32 codebook:2;
	__le32 ngroup:2;
	__le32 invalid_prof:1;
	__le32 _rsv:3;

	__le32 col_id1:7, row_id1:9;
	__le32 col_id2:7, row_id2:9;
	__le32 col_id3:7, row_id3:9;
	__le32 col_id4:7, row_id4:9;

	union {
		struct pfmu_ru_field field;
		struct pfmu_partial_bw_info bw_info;
	};
	__le32 mob_cal_en:1;
	__le32 _rsv2:3;
	__le32 mob_ru_alloc:9;	/* EHT profile uses full 9 bit */
	__le32 _rsv3:3;

	__le32 snr_sts0:8, snr_sts1:8, snr_sts2:8, snr_sts3:8;
	__le32 snr_sts4:8, snr_sts5:8, snr_sts6:8, snr_sts7:8;

	__le32 _rsv4;
} __packed;

struct mt7996_pfmu_tag2 {
	__le32 smart_ant:24;
	__le32 se_idx:5;
	__le32 _rsv:3;

	__le32 _rsv1:16;
	__le32 ibf_timeout:8;
	__le32 _rsv2:8;

	__le32 ibf_data_bw:3;
	__le32 ibf_nc:3;
	__le32 ibf_nr:3;
	__le32 ibf_ru:9;
	__le32 _rsv3:14;

	__le32 mob_delta_t:8;
	__le32 mob_lq_result:7;
	__le32 _rsv5:1;
	__le32 _rsv6:16;

	__le32 _rsv7;
} __packed;

struct mt7996_pfmu_tag_event {
	struct mt7996_mcu_bf_basic_event event;

	u8 bfer;
	u8 __rsv[3];

	struct mt7996_pfmu_tag1 t1;
	struct mt7996_pfmu_tag2 t2;
};

struct mt7996_pfmu_tag {
	struct mt7996_pfmu_tag1 t1;
	struct mt7996_pfmu_tag2 t2;
};

enum bf_lm_type {
	BF_LM_LEGACY,
	BF_LM_HT,
	BF_LM_VHT,
	BF_LM_HE,
	BF_LM_EHT,
};

struct mt7996_txbf_phase_out {
	u8 c0_l;
	u8 c1_l;
	u8 c2_l;
	u8 c3_l;
	u8 c0_m;
	u8 c1_m;
	u8 c2_m;
	u8 c3_m;
	u8 c0_mh;
	u8 c1_mh;
	u8 c2_mh;
	u8 c3_mh;
	u8 c0_h;
	u8 c1_h;
	u8 c2_h;
	u8 c3_h;
	u8 c0_uh;
	u8 c1_uh;
	u8 c2_uh;
	u8 c3_uh;
};

struct mt7996_txbf_rx_phase_2g {
	u8 rx_uh;
	u8 rx_h;
	u8 rx_m;
	u8 rx_l;
	u8 rx_ul;
};

struct mt7996_txbf_rx_phase_5g {
	u8 rx_uh;
	u8 rx_h;
	u8 rx_mh;
	u8 rx_m;
	u8 rx_l;
	u8 rx_ul;
};

struct mt7996_txbf_phase_info_2g {
	struct mt7996_txbf_rx_phase_2g r0;
	struct mt7996_txbf_rx_phase_2g r1;
	struct mt7996_txbf_rx_phase_2g r2;
	struct mt7996_txbf_rx_phase_2g r3;
	struct mt7996_txbf_rx_phase_2g r2_sx2;
	struct mt7996_txbf_rx_phase_2g r3_sx2;
	u8 m_t0_h;
	u8 m_t1_h;
	u8 m_t2_h;
	u8 m_t2_h_sx2;
	u8 r0_reserved;
	u8 r1_reserved;
	u8 r2_reserved;
	u8 r3_reserved;
	u8 r2_sx2_reserved;
	u8 r3_sx2_reserved;
};

struct mt7996_txbf_phase_info_5g {
	struct mt7996_txbf_rx_phase_5g r0;
	struct mt7996_txbf_rx_phase_5g r1;
	struct mt7996_txbf_rx_phase_5g r2;
	struct mt7996_txbf_rx_phase_5g r3;
	struct mt7996_txbf_rx_phase_2g r2_sx2;	/* no middle-high in r2_sx2 */
	struct mt7996_txbf_rx_phase_2g r3_sx2;	/* no middle-high in r3_sx2 */
	u8 m_t0_h;
	u8 m_t1_h;
	u8 m_t2_h;
	u8 m_t2_h_sx2;
	u8 r0_reserved;
	u8 r1_reserved;
	u8 r2_reserved;
	u8 r3_reserved;
	u8 r2_sx2_reserved;
	u8 r3_sx2_reserved;
};

struct mt7996_txbf_phase {
	u8 status;
	union {
		struct mt7996_txbf_phase_info_2g phase_2g;
		struct mt7996_txbf_phase_info_5g phase_5g;
	};
};

#define phase_assign(group, field, dump, ...)	({						\
	if (group) {										\
		phase->phase_5g.field = cal->phase_5g.field;					\
		if (dump)									\
			dev_info(dev->mt76.dev, "%s = %d\n", #field, phase->phase_5g.field);	\
	} else {										\
		phase->phase_2g.field = cal->phase_5g.field;					\
		if (dump)									\
			dev_info(dev->mt76.dev, "%s = %d\n", #field, phase->phase_2g.field);	\
	}											\
})

#define phase_assign_rx_g0(group, rx, ...)	({						\
	phase_assign(group, rx.rx_uh, false);							\
	phase_assign(group, rx.rx_h, false);							\
	phase_assign(group, rx.rx_m, false);							\
	phase_assign(group, rx.rx_l, false);							\
	phase_assign(group, rx.rx_ul, false);							\
})

#define phase_assign_rx(group, rx, ...)	({							\
	if (group) {										\
		phase_assign(group, rx.rx_uh, true);						\
		phase_assign(group, rx.rx_h, true);						\
		phase->phase_5g.rx.rx_mh = cal->phase_5g.rx.rx_mh;				\
		dev_info(dev->mt76.dev, "%s.rx_mh = %d\n", #rx, phase->phase_5g.rx.rx_mh);	\
		phase_assign(group, rx.rx_m, true);						\
		phase_assign(group, rx.rx_l, true);						\
		phase_assign(group, rx.rx_ul, true);						\
	} else {										\
		phase_assign(group, rx.rx_uh, true);						\
		phase_assign(group, rx.rx_h, true);						\
		phase_assign(group, rx.rx_m, true);						\
		phase_assign(group, rx.rx_l, true);						\
		phase_assign(group, rx.rx_ul, true);						\
	}											\
})

#define GROUP_L		0
#define GROUP_M		1
#define GROUP_H		2

struct mt7996_pfmu_data {
	__le16 subc_idx;
	__le16 phi11;
	__le16 phi21;
	__le16 phi31;
};

struct mt7996_ibf_cal_info {
	struct mt7996_mcu_bf_basic_event event;

	u8 category_id;
	u8 group_l_m_n;
	u8 group;
	bool sx2;
	u8 status;
	u8 cal_type;
	u8 _rsv[2];
	struct mt7996_txbf_phase_out phase_out;
	union {
		struct mt7996_txbf_phase_info_2g phase_2g;
		struct mt7996_txbf_phase_info_5g phase_5g;
	};
} __packed;

enum {
	IBF_PHASE_CAL_UNSPEC,
	IBF_PHASE_CAL_NORMAL,
	IBF_PHASE_CAL_VERIFY,
	IBF_PHASE_CAL_NORMAL_INSTRUMENT,
	IBF_PHASE_CAL_VERIFY_INSTRUMENT,
};

#define MT7996_TXBF_SUBCAR_NUM	64

enum {
	UNI_EVENT_BF_PFMU_TAG = 0x5,
	UNI_EVENT_BF_PFMU_DATA = 0x7,
	UNI_EVENT_BF_STAREC = 0xB,
	UNI_EVENT_BF_CAL_PHASE = 0xC,
	UNI_EVENT_BF_FBK_INFO = 0x17,
	UNI_EVENT_BF_TXSND_INFO = 0x18,
	UNI_EVENT_BF_PLY_INFO = 0x19,
	UNI_EVENT_BF_METRIC_INFO = 0x1A,
	UNI_EVENT_BF_TXCMD_CFG_INFO = 0x1B,
	UNI_EVENT_BF_SND_CNT_INFO = 0x1D,
	UNI_EVENT_BF_MAX_NUM
};

struct uni_muru_mum_set_group_tbl_entry {
	__le16 wlan_idx0;
	__le16 wlan_idx1;
	__le16 wlan_idx2;
	__le16 wlan_idx3;

	u8 dl_mcs_user0:4;
	u8 dl_mcs_user1:4;
	u8 dl_mcs_user2:4;
	u8 dl_mcs_user3:4;
	u8 ul_mcs_user0:4;
	u8 ul_mcs_user1:4;
	u8 ul_mcs_user2:4;
	u8 ul_mcs_user3:4;

	u8 num_user:2;
	u8 rsv:6;
	u8 nss0:2;
	u8 nss1:2;
	u8 nss2:2;
	u8 nss3:2;
	u8 ru_alloc;
	u8 ru_alloc_ext;

	u8 capa;
	u8 gi;
	u8 dl_ul;
	u8 _rsv2;
};

enum UNI_CMD_MURU_VER_T {
	UNI_CMD_MURU_VER_LEG = 0,
	UNI_CMD_MURU_VER_HE,
	UNI_CMD_MURU_VER_EHT,
	UNI_CMD_MURU_VER_MAX
};

#define UNI_MAX_MCS_SUPPORT_HE 11
#define UNI_MAX_MCS_SUPPORT_EHT 13

enum {
	RUALLOC_BW20 = 122,
	RUALLOC_BW40 = 130,
	RUALLOC_BW80 = 134,
	RUALLOC_BW160 = 137,
	RUALLOC_BW320 = 395,
};

enum {
	MAX_MODBF_VHT = 0,
	MAX_MODBF_HE = 2,
	MAX_MODBF_EHT = 4,
};

enum {
	BF_SND_READ_INFO = 0,
	BF_SND_CFG_OPT,
	BF_SND_CFG_INTV,
	BF_SND_STA_STOP,
	BF_SND_CFG_MAX_STA,
	BF_SND_CFG_BFRP,
	BF_SND_CFG_INF,
	BF_SND_CFG_TXOP_SND
};

enum {
	UNI_EVENT_SR_CFG_SR_ENABLE = 0x1,
	UNI_EVENT_SR_SW_SD = 0x83,
	UNI_EVENT_SR_HW_IND = 0xC9,
	UNI_EVENT_SR_HW_ESR_ENABLE = 0xD8,
};
enum {
	UNI_CMD_SR_CFG_SR_ENABLE = 0x1,
	UNI_CMD_SR_SW_SD = 0x84,
	UNI_CMD_SR_HW_IND = 0xCB,
	UNI_CMD_SR_HW_ENHANCE_SR_ENABLE = 0xDA,
};

struct mt7996_mcu_sr_basic_event {
	struct mt7996_mcu_rxd rxd;

	u8 band_idx;
	u8 _rsv[3];

	__le16 tag;
	__le16 len;
};

struct sr_sd_tlv {
	u8 _rsv[16];
	__le32 sr_tx_airtime;
	__le32 obss_airtime;
	__le32 my_tx_airtime;
	__le32 my_rx_airtime;
	__le32 channel_busy_time;
	__le32 total_airtime;
	__le32 total_airtime_ratio;
	__le32 obss_airtime_ratio;
	u8 rule;
	u8 _rsv2[59];
} __packed;

struct mt7996_mcu_sr_swsd_event {
	struct mt7996_mcu_sr_basic_event basic;
	struct sr_sd_tlv tlv[3];
} __packed;

struct mt7996_mcu_sr_common_event {
	struct mt7996_mcu_sr_basic_event basic;
	__le32 value;
};

struct mt7996_mcu_sr_hw_ind_event {
	struct mt7996_mcu_sr_basic_event basic;
	__le16 non_srg_valid_cnt;
	u8 _rsv[4];
	__le16 inter_bss_ppdu_cnt;
	u8 _rsv2[4];
	__le32 sr_ampdu_mpdu_cnt;
	__le32 sr_ampdu_mpdu_acked_cnt;
};

struct mt7996_muru_comm {
	u8 pda_pol;
	u8 band;
	u8 spe_idx;
	u8 proc_type;

	__le16 mlo_ctrl;
	u8 sch_type;
	u8 ppdu_format;
	u8 ac;
	u8 _rsv[3];
};

struct mt7996_muru_dl {
	u8 user_num;
	u8 tx_mode;
	u8 bw;
	u8 gi;

	u8 ltf;
	u8 mcs;
	u8 dcm;
	u8 cmprs;

	__le16 ru[16];

	u8 c26[2];
	u8 ack_policy;
	u8 tx_power;

	__le16 mu_ppdu_duration;
	u8 agc_disp_order;
	u8 _rsv1;

	u8 agc_disp_pol;
	u8 agc_disp_ratio;
	__le16 agc_disp_linkMFG;

	__le16 prmbl_punc_bmp;
	u8 _rsv2[2];

	struct {
		__le16 wlan_idx;
		u8 ru_alloc_seg;
		u8 ru_idx;
		u8 ldpc;
		u8 nss;
		u8 mcs;
		u8 mu_group_idx;
		u8 vht_groud_id;
		u8 vht_up;
		u8 he_start_stream;
		u8 he_mu_spatial;
		__le16 tx_power_alpha;
		u8 ack_policy;
		u8 ru_allo_ps160;
	} usr[16];
};

struct mt7996_muru_ul {
	u8 user_num;
	u8 tx_mode;

	u8 ba_type;
	u8 _rsv;

	u8 bw;
	u8 gi_ltf;
	__le16 ul_len;

	__le16 trig_cnt;
	u8 pad;
	u8 trig_type;

	__le16 trig_intv;
	u8 trig_ta[ETH_ALEN];
	__le16 ul_ru[16];

	u8 c26[2];
	__le16 agc_disp_linkMFG;

	u8 agc_disp_mu_len;
	u8 agc_disp_pol;
	u8 agc_disp_ratio;
	u8 agc_disp_pu_idx;

	struct {
		__le16 wlan_idx;
		u8 ru_alloc_seg;
		u8 ru_idx;
		u8 ldpc;
		u8 nss;
		u8 mcs;
		u8 target_rssi;
		__le32 trig_pkt_size;
		u8 ru_allo_ps160;
		u8 _rsv2[3];
	} usr[16];
};

struct mt7996_muru_dbg {
	/* HE TB RX Debug */
	__le32 rx_hetb_nonsf_en_bitmap;
	__le32 rx_hetb_cfg[2];
};

struct mt7996_muru {
	__le32 cfg_comm;
	__le32 cfg_dl;
	__le32 cfg_ul;
	__le32 cfg_dbg;

	struct mt7996_muru_comm comm;
	struct mt7996_muru_dl dl;
	struct mt7996_muru_ul ul;
	struct mt7996_muru_dbg dbg;
};


#define MURU_PPDU_HE_TRIG	BIT(2)
#define MURU_PPDU_HE_MU		BIT(3)

#define MURU_OFDMA_SCH_TYPE_DL	BIT(0)
#define MURU_OFDMA_SCH_TYPE_UL	BIT(1)

/* Common Config */
#define MURU_COMM_PPDU_FMT	BIT(0)
#define MURU_COMM_SCH_TYPE	BIT(1)
#define MURU_COMM_BAND		BIT(2)
#define MURU_COMM_WMM		BIT(3)
#define MURU_COMM_SPE_IDX	BIT(4)
#define MURU_COMM_PROC_TYPE	BIT(5)
#define MURU_COMM_SET		(MURU_COMM_PPDU_FMT | MURU_COMM_SCH_TYPE)
#define MURU_COMM_SET_TM	(MURU_COMM_PPDU_FMT | MURU_COMM_BAND | \
				 MURU_COMM_WMM | MURU_COMM_SPE_IDX)

/* DL Common config */
#define MURU_FIXED_DL_TOTAL_USER_CNT	BIT(4)

/* UL Common Config */
#define MURU_FIXED_UL_TOTAL_USER_CNT	BIT(4)

enum {
	CAPI_SU,
	CAPI_MU,
	CAPI_ER_SU,
	CAPI_TB,
	CAPI_LEGACY
};

enum {
	CAPI_BASIC,
	CAPI_BRP,
	CAPI_MU_BAR,
	CAPI_MU_RTS,
	CAPI_BSRP,
	CAPI_GCR_MU_BAR,
	CAPI_BQRP,
	CAPI_NDP_FRP,
};

enum {
	MU_DL_ACK_POLICY_MU_BAR = 3,
	MU_DL_ACK_POLICY_TF_FOR_ACK = 4,
	MU_DL_ACK_POLICY_SU_BAR = 5,
};

enum muru_vendor_ctrl {
	MU_CTRL_UPDATE,
	MU_CTRL_DL_USER_CNT,
	MU_CTRL_UL_USER_CNT,
};

enum {
	VOW_DRR_DBG_DUMP_BMP = BIT(0),
	VOW_DRR_DBG_EST_AT_PRINT = BIT(1),
	VOW_DRR_DBG_ADJ_GLOBAL_THLD = BIT(21),
	VOW_DRR_DBG_PRN_LOUD = BIT(22),
	VOW_DRR_DBG_PRN_ADJ_STA = BIT(23),
	VOW_DRR_DBG_FIX_CR = GENMASK(27, 24),
	VOW_DRR_DBG_CLR_FIX_CR = BIT(28),
	VOW_DRR_DBG_DISABLE = BIT(29),
	VOW_DRR_DBG_DUMP_CR = BIT(30),
	VOW_DRR_DBG_PRN = BIT(31)
};

#define VOW_DRR_DBG_FLAGS (VOW_DRR_DBG_DUMP_BMP |	\
			  VOW_DRR_DBG_EST_AT_PRINT |	\
			  VOW_DRR_DBG_ADJ_GLOBAL_THLD |	\
			  VOW_DRR_DBG_PRN_LOUD |	\
			  VOW_DRR_DBG_PRN_ADJ_STA |	\
			  VOW_DRR_DBG_FIX_CR |		\
			  VOW_DRR_DBG_CLR_FIX_CR |	\
			  VOW_DRR_DBG_DISABLE |		\
			  VOW_DRR_DBG_DUMP_CR |		\
			  VOW_DRR_DBG_PRN)
#endif

#endif
