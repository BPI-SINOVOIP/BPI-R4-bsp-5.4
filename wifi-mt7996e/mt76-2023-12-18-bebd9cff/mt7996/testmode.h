/* SPDX-License-Identifier: ISC */
/* Copyright (C) 2020 MediaTek Inc. */

#ifndef __MT7996_TESTMODE_H
#define __MT7996_TESTMODE_H

enum {
	TM_CBW_20MHZ,
	TM_CBW_40MHZ,
	TM_CBW_80MHZ,
	TM_CBW_10MHZ,
	TM_CBW_5MHZ,
	TM_CBW_160MHZ,
	TM_CBW_8080MHZ,
	TM_CBW_320MHZ = 12,
};

/* BW defined in FW hal_cal_flow_rom.h */
enum {
	FW_CDBW_20MHZ,
	FW_CDBW_40MHZ,
	FW_CDBW_80MHZ,
	FW_CDBW_160MHZ,
	FW_CDBW_320MHZ,
	FW_CDBW_5MHZ,
	FW_CDBW_10MHZ,
	FW_CDBW_8080MHZ,
};

enum {
	BF_CDBW_20MHZ,
	BF_CDBW_40MHZ,
	BF_CDBW_80MHZ,
	BF_CDBW_160MHZ,
	BF_CDBW_320MHZ,
	BF_CDBW_10MHZ = BF_CDBW_320MHZ,
	BF_CDBW_5MHZ,
	BF_CDBW_8080MHZ,
};

#define FIRST_CONTROL_CHAN_BITMAP_BW40		2
#define FIRST_CONTROL_CHAN_BITMAP_BW80		4
#define FIRST_CONTROL_CHAN_BITMAP_BW160		0x10010101

enum bw_mapping_method {
	BW_MAP_NL_TO_FW,
	BW_MAP_NL_TO_TM,
	BW_MAP_NL_TO_BF,
	BW_MAP_NL_TO_MHZ,
	BW_MAP_NL_TO_CONTROL_BITMAP_5G,

	NUM_BW_MAP,
};

enum rate_mapping_type {
	RATE_MODE_TO_PHY,
	RATE_MODE_TO_LM,

	NUM_RATE_MAP,
};

struct tm_cal_param {
	__le32 func_data;
	u8 band_idx;
	u8 rsv[3];
};

struct mt7996_tm_rf_test {
	__le16 tag;
	__le16 len;

	u8 action;
	u8 icap_len;
	u8 _rsv[2];
	union {
		__le32 op_mode;
		__le32 freq;

		struct {
			__le32 func_idx;
			union {
				__le32 func_data;
				__le32 cal_dump;
				struct tm_cal_param cal_param;
				u8 _pad[80];
			} param;
		} rf;
	} op;
} __packed;

struct mt7996_tm_req {
	u8 _rsv[4];

	struct mt7996_tm_rf_test rf_test;
} __packed;

struct mt7996_tm_rf_test_data {
	__le32 cal_idx;
	__le32 cal_type;
	u8 cal_data[0];
} __packed;

struct mt7996_tm_rf_test_result {
	__le32 func_idx;
	__le32 payload_length;
	u8 data[0];
};

struct mt7996_tm_event {
	u8 _rsv[4];

	__le16 tag;
	__le16 len;
	struct mt7996_tm_rf_test_result result;
} __packed;

#define RF_TEST_RE_CAL		0x01

enum {
	RF_ACTION_SWITCH_TO_RF_TEST,
	RF_ACTION_IN_RF_TEST,
	RF_ACTION_SET = 3,
	RF_ACTION_GET,
};

#define RF_TEST_ICAP_LEN	120

enum {
	RF_OPER_NORMAL,
	RF_OPER_RF_TEST,
	RF_OPER_ICAP,
	RF_OPER_ICAP_OVERLAP,
	RF_OPER_WIFI_SPECTRUM,
};

enum {
	UNI_RF_TEST_CTRL,
};

#define RF_CMD(cmd)		RF_TEST_CMD_##cmd

enum {
	RF_TEST_CMD_STOP_TEST = 0,
	RF_TEST_CMD_START_TX = 1,
	RF_TEST_CMD_START_RX = 2,
	RF_TEST_CMD_CONT_WAVE = 10,
	RF_TEST_CMD_TX_COMMIT = 18,
	RF_TEST_CMD_RX_COMMIT = 19,
};

#define SET_ID(id)		RF_TEST_ID_SET_##id
#define GET_ID(id)		RF_TEST_ID_GET_##id

enum {
	RF_TEST_ID_SET_COMMAND = 1,
	RF_TEST_ID_SET_POWER = 2,
	RF_TEST_ID_SET_TX_RATE = 3,
	RF_TEST_ID_SET_TX_MODE = 4,
	RF_TEST_ID_SET_TX_LEN = 6,
	RF_TEST_ID_SET_TX_COUNT = 7,
	RF_TEST_ID_SET_IPG = 8,
	RF_TEST_ID_SET_GI = 16,
	RF_TEST_ID_SET_STBC = 17,
	RF_TEST_ID_SET_CHAN_FREQ = 18,
	RF_TEST_ID_GET_TXED_COUNT = 32,
	RF_TEST_ID_SET_CONT_WAVE_MODE = 65,
	RF_TEST_ID_SET_DA = 68,
	RF_TEST_ID_SET_SA = 69,
	RF_TEST_ID_SET_CBW = 71,
	RF_TEST_ID_SET_DBW = 72,
	RF_TEST_ID_SET_PRIMARY_CH = 73,
	RF_TEST_ID_SET_ENCODE_MODE = 74,
	RF_TEST_ID_SET_BAND = 90,
	RF_TEST_ID_SET_TRX_COUNTER_RESET = 91,
	RF_TEST_ID_SET_MAC_HEADER = 101,
	RF_TEST_ID_SET_SEQ_CTRL = 102,
	RF_TEST_ID_SET_PAYLOAD = 103,
	RF_TEST_ID_SET_BAND_IDX = 104,
	RF_TEST_ID_SET_RX_PATH = 106,
	RF_TEST_ID_SET_FREQ_OFFSET = 107,
	RF_TEST_ID_GET_FREQ_OFFSET = 108,
	RF_TEST_ID_SET_TX_PATH = 113,
	RF_TEST_ID_SET_NSS = 114,
	RF_TEST_ID_SET_ANT_MASK = 115,
	RF_TEST_ID_SET_IBF_ENABLE = 126,
	RF_TEST_ID_SET_EBF_ENABLE = 127,
	RF_TEST_ID_GET_TX_POWER = 136,
	RF_TEST_ID_SET_RX_MU_AID = 157,
	RF_TEST_ID_SET_HW_TX_MODE = 167,
	RF_TEST_ID_SET_PUNCTURE = 168,
	RF_TEST_ID_SET_FREQ_OFFSET_C2 = 171,
	RF_TEST_ID_GET_FREQ_OFFSET_C2 = 172,
	RF_TEST_ID_SET_CFG_ON = 176,
	RF_TEST_ID_SET_CFG_OFF = 177,
	RF_TEST_ID_SET_BSSID = 189,
	RF_TEST_ID_SET_TX_TIME = 190,
	RF_TEST_ID_SET_MAX_PE = 191,
	RF_TEST_ID_SET_AID_OFFSET = 204,
};

#define POWER_CTRL(type)	UNI_TXPOWER_##type##_CTRL

struct mt7996_tm_rx_stat_user_ctrl {
	__le16 tag;
	__le16 len;

	u8 band_idx;
	u8 rsv;
	__le16 user_idx;
} __packed;

struct mt7996_tm_rx_stat_all {
	__le16 tag;
	__le16 len;

	u8 band_idx;
	u8 rsv[3];
} __packed;

struct mt7996_tm_rx_req {
	u8 band;
	u8 _rsv[3];

	union {
		struct mt7996_tm_rx_stat_user_ctrl user_ctrl;
		struct mt7996_tm_rx_stat_all rx_stat_all;
	};
} __packed;

enum {
	UNI_TM_RX_STAT_SET_USER_CTRL = 7,
	UNI_TM_RX_STAT_GET_ALL_V2 = 9,
};

struct rx_band_info {
	/* mac part */
	__le16 mac_rx_fcs_err_cnt;
	__le16 mac_rx_len_mismatch;
	__le16 mac_rx_fcs_ok_cnt;
	u8 rsv1[2];
	__le32 mac_rx_mdrdy_cnt;

	/* phy part */
	__le16 phy_rx_fcs_err_cnt_cck;
	__le16 phy_rx_fcs_err_cnt_ofdm;
	__le16 phy_rx_pd_cck;
	__le16 phy_rx_pd_ofdm;
	__le16 phy_rx_sig_err_cck;
	__le16 phy_rx_sfd_err_cck;
	__le16 phy_rx_sig_err_ofdm;
	__le16 phy_rx_tag_err_ofdm;
	__le16 phy_rx_mdrdy_cnt_cck;
	__le16 phy_rx_mdrdy_cnt_ofdm;
} __packed;

struct rx_band_info_ext {
	/* mac part */
	__le32 mac_rx_mpdu_cnt;

	/* phy part */
	u8 rsv[4];
} __packed;

struct rx_common_info {
	__le16 rx_fifo_full;
	u8 rsv[2];
	__le32 aci_hit_low;
	__le32 aci_hit_high;
} __packed;

struct rx_common_info_ext {
	__le32 driver_rx_count;
	__le32 sinr;
	__le32 mu_pkt_count;

	/* mac part */
	u8 _rsv[4];

	/* phy part */
	u8 sig_mcs;
	u8 rsv[3];
} __packed;

struct rx_rxv_info {
	__le16 rcpi;
	s16 rssi;
	s16 snr;
	s16 adc_rssi;
} __packed;

struct rx_rssi_info {
	s8 ib_rssi;
	s8 wb_rssi;
	u8 rsv[2];
} __packed;

struct rx_user_info {
	s32 freq_offset;
	s32 snr;
	__le32 fcs_err_count;
} __packed;

struct rx_user_info_ext {
	s8 ne_var_db_all_user;
	u8 rsv[3];
} __packed;

#define MAX_ANTENNA_NUM		8
#define MAX_USER_NUM		16

struct mt7996_tm_rx_event_stat_all {
	__le16 tag;
	__le16 len;

	struct rx_band_info band_info;
	struct rx_band_info_ext band_info_ext;
	struct rx_common_info common_info;
	struct rx_common_info_ext common_info_ext;

	/* RXV info */
	struct rx_rxv_info rxv_info[MAX_ANTENNA_NUM];

	/* RSSI info */
	struct rx_rssi_info fagc[MAX_ANTENNA_NUM];
	struct rx_rssi_info inst[MAX_ANTENNA_NUM];

	/* User info */
	struct rx_user_info user_info[MAX_USER_NUM];
	struct rx_user_info_ext user_info_ext[MAX_USER_NUM];
} __packed;

struct mt7996_tm_rx_event {
	u8 _rsv[4];

	union {
		struct mt7996_tm_rx_event_stat_all rx_stat_all;
	};
} __packed;

enum {
	RDD_SET_IPI_CR_INIT,		/* CR initialization */
	RDD_SET_IPI_HIST_RESET,		/* Reset IPI histogram counter */
	RDD_SET_IDLE_POWER,		/* Idle power info */
	RDD_SET_IPI_HIST_NUM
};

enum {
	RDD_IPI_HIST_0,			/* IPI count for power <= -92 (dBm) */
	RDD_IPI_HIST_1,			/* IPI count for -92 < power <= -89 (dBm) */
	RDD_IPI_HIST_2,			/* IPI count for -89 < power <= -86 (dBm) */
	RDD_IPI_HIST_3,			/* IPI count for -86 < power <= -83 (dBm) */
	RDD_IPI_HIST_4,			/* IPI count for -83 < power <= -80 (dBm) */
	RDD_IPI_HIST_5,			/* IPI count for -80 < power <= -75 (dBm) */
	RDD_IPI_HIST_6,			/* IPI count for -75 < power <= -70 (dBm) */
	RDD_IPI_HIST_7,			/* IPI count for -70 < power <= -65 (dBm) */
	RDD_IPI_HIST_8,			/* IPI count for -65 < power <= -60 (dBm) */
	RDD_IPI_HIST_9,			/* IPI count for -60 < power <= -55 (dBm) */
	RDD_IPI_HIST_10,		/* IPI count for -55 < power        (dBm) */
	RDD_IPI_FREE_RUN_CNT,		/* IPI count for counter++ per 8 us */
	RDD_IPI_HIST_ALL_CNT,		/* Get all IPI */
	RDD_IPI_HIST_0_TO_10_CNT,	/* Get IPI histogram 0 to 10 */
	RDD_IPI_HIST_2_TO_10_CNT,	/* Get IPI histogram 2 to 10 */
	RDD_TX_ASSERT_TIME,		/* Get band 1 TX assert time */
	RDD_IPI_HIST_NUM
};

#define POWER_INDICATE_HIST_MAX		RDD_IPI_FREE_RUN_CNT
#define IPI_HIST_TYPE_NUM		(POWER_INDICATE_HIST_MAX + 1)

struct mt7996_tm_rdd_ipi_ctrl {
	u8 ipi_hist_idx;
	u8 band_idx;
	u8 rsv[2];
	__le32 ipi_hist_val[IPI_HIST_TYPE_NUM];
	__le32 tx_assert_time;		/* unit: us */
} __packed;

#endif
