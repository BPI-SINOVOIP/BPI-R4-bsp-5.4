/* SPDX-License-Identifier: ISC */
/*
 * Copyright (C) 2022 MediaTek Inc.
 */

#ifndef __MT7996_H
#define __MT7996_H

#include <linux/interrupt.h>
#include <linux/ktime.h>
#include "../mt76_connac.h"
#include "regs.h"

#define MT7996_MAX_INTERFACES		19	/* per-band */
#define MT7996_MAX_WMM_SETS		4
#define MT7996_WTBL_BMC_SIZE		(is_mt7992(&dev->mt76) ? 32 : 64)
#define MT7996_WTBL_RESERVED		(mt7996_wtbl_size(dev) - 1)
#define MT7996_WTBL_STA			(MT7996_WTBL_RESERVED - \
					 mt7996_max_interface_num(dev))

#define MT7996_WATCHDOG_TIME		(HZ / 10)
#define MT7996_RESET_TIMEOUT		(30 * HZ)

#define MT7996_TX_RING_SIZE		2048
#define MT7996_TX_MCU_RING_SIZE		256
#define MT7996_TX_FWDL_RING_SIZE	128

#define MT7996_RX_RING_SIZE		1536
#define MT7996_RX_MCU_RING_SIZE		512
#define MT7996_RX_MCU_RING_SIZE_WA	1024

#define MT7996_FIRMWARE_WA		"mediatek/mt7996/mt7996_wa.bin"
#define MT7996_FIRMWARE_WM		"mediatek/mt7996/mt7996_wm.bin"
#define MT7996_FIRMWARE_DSP		"mediatek/mt7996/mt7996_dsp.bin"
#define MT7996_FIRMWARE_WM_TM		"mediatek/mt7996/mt7996_wm_tm.bin"
#define MT7996_ROM_PATCH		"mediatek/mt7996/mt7996_rom_patch.bin"

#define MT7996_FIRMWARE_WA_233		"mediatek/mt7996/mt7996_wa_233.bin"
#define MT7996_FIRMWARE_WM_233		"mediatek/mt7996/mt7996_wm_233.bin"
#define MT7996_FIRMWARE_DSP_233		MT7996_FIRMWARE_DSP
#define MT7996_FIRMWARE_WM_TM_233	"mediatek/mt7996/mt7996_wm_tm_233.bin"
#define MT7996_ROM_PATCH_233		"mediatek/mt7996/mt7996_rom_patch_233.bin"

#define MT7992_FIRMWARE_WA		"mediatek/mt7996/mt7992_wa.bin"
#define MT7992_FIRMWARE_WM		"mediatek/mt7996/mt7992_wm.bin"
#define MT7992_FIRMWARE_DSP		"mediatek/mt7996/mt7992_dsp.bin"
#define MT7992_FIRMWARE_WM_TM		"mediatek/mt7996/mt7992_wm_tm.bin"
#define MT7992_ROM_PATCH		"mediatek/mt7996/mt7992_rom_patch.bin"

#define MT7992_FIRMWARE_WA_24		"mediatek/mt7996/mt7992_wa_24.bin"
#define MT7992_FIRMWARE_WM_24		"mediatek/mt7996/mt7992_wm_24.bin"
#define MT7992_FIRMWARE_DSP_24		"mediatek/mt7996/mt7992_dsp_24.bin"
#define MT7992_FIRMWARE_WM_TM_24	"mediatek/mt7996/mt7992_wm_tm_24.bin"
#define MT7992_ROM_PATCH_24		"mediatek/mt7996/mt7992_rom_patch_24.bin"

#define MT7992_FIRMWARE_WA_23		"mediatek/mt7996/mt7992_wa_23.bin"
#define MT7992_FIRMWARE_WM_23		"mediatek/mt7996/mt7992_wm_23.bin"
#define MT7992_FIRMWARE_DSP_23		"mediatek/mt7996/mt7992_dsp_23.bin"
#define MT7992_FIRMWARE_WM_TM_23	"mediatek/mt7996/mt7992_wm_tm_23.bin"
#define MT7992_ROM_PATCH_23		"mediatek/mt7996/mt7992_rom_patch_23.bin"

#define MT7996_EEPROM_DEFAULT		"mediatek/mt7996/mt7996_eeprom.bin"
#define MT7996_EEPROM_DEFAULT_233	"mediatek/mt7996/mt7996_eeprom_233.bin"
#define MT7996_EEPROM_DEFAULT_404	"mediatek/mt7996/mt7996_eeprom_dual_404.bin"
#define MT7996_EEPROM_DEFAULT_TM	"mediatek/mt7996/mt7996_eeprom_tm.bin"
#define MT7992_EEPROM_DEFAULT		"mediatek/mt7996/mt7992_eeprom_2i5i.bin"
#define MT7992_EEPROM_DEFAULT_EXT	"mediatek/mt7996/mt7992_eeprom_2e5e.bin"
#define MT7992_EEPROM_DEFAULT_MIX	"mediatek/mt7996/mt7992_eeprom_2i5e.bin"
#define MT7992_EEPROM_DEFAULT_24	"mediatek/mt7996/mt7992_eeprom_24_2i5i.bin"
#define MT7992_EEPROM_DEFAULT_23	"mediatek/mt7996/mt7992_eeprom_23_2i5i.bin"
#define MT7992_EEPROM_DEFAULT_23_EXT	"mediatek/mt7996/mt7992_eeprom_23_2e5e.bin"
#define MT7992_EEPROM_DEFAULT_TM	"mediatek/mt7996/mt7992_eeprom_tm.bin"
#define MT7996_EEPROM_SIZE		7680
#define MT7996_EEPROM_BLOCK_SIZE	16
#define MT7996_TOKEN_SIZE		16384
#define MT7996_HW_TOKEN_SIZE		8192
#define MT7996_SW_TOKEN_SIZE		15360

#define MT7996_CFEND_RATE_DEFAULT	0x49	/* OFDM 24M */
#define MT7996_CFEND_RATE_11B		0x03	/* 11B LP, 11M */

#define MT7996_SKU_RATE_NUM		417
#define MT7996_SKU_PATH_NUM		494

#define MT7996_MAX_TWT_AGRT		16
#define MT7996_MAX_STA_TWT_AGRT		8
#define MT7996_MIN_TWT_DUR		64
#define MT7996_MAX_QUEUE		(__MT_RXQ_MAX +	__MT_MCUQ_MAX + 3)

/* NOTE: used to map mt76_rates. idx may change if firmware expands table */
#define MT7996_BASIC_RATES_TBL		31
#define MT7996_BEACON_RATES_TBL		25

#define MT7996_THERMAL_THROTTLE_MAX	100
#define MT7996_CDEV_THROTTLE_MAX	99
#define MT7996_CRIT_TEMP_IDX		0
#define MT7996_MAX_TEMP_IDX		1
#define MT7996_CRIT_TEMP		110
#define MT7996_MAX_TEMP			120

#define MT7996_BUILD_TIME_LEN		24

#define MT7996_RRO_MAX_SESSION		1024
#define MT7996_RRO_WINDOW_MAX_LEN	1024
#define MT7996_RRO_ADDR_ELEM_LEN	128
#define MT7996_RRO_BA_BITMAP_LEN	2
#define MT7996_RRO_BA_BITMAP_CR_SIZE	((MT7996_RRO_MAX_SESSION * 128) /	\
					 MT7996_RRO_BA_BITMAP_LEN)
#define MT7996_RRO_BA_BITMAP_SESSION_SIZE	(MT7996_RRO_MAX_SESSION /	\
						 MT7996_RRO_ADDR_ELEM_LEN)
#define MT7996_RRO_WINDOW_MAX_SIZE	(MT7996_RRO_WINDOW_MAX_LEN *		\
					 MT7996_RRO_BA_BITMAP_SESSION_SIZE)

#define MT7996_RX_BUF_SIZE		(1800 + \
					 SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
#define MT7996_RX_MSDU_PAGE_SIZE	(128 + \
					 SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))

#define MT7996_DRR_STA_BSS_GRP_MASK	GENMASK(5, 0)
#define MT7996_DRR_STA_AC0_QNTM_MASK	GENMASK(10, 8)
#define MT7996_DRR_STA_AC1_QNTM_MASK	GENMASK(14, 12)
#define MT7996_DRR_STA_AC2_QNTM_MASK	GENMASK(18, 16)
#define MT7996_DRR_STA_AC3_QNTM_MASK	GENMASK(22, 20)

/* RRO 3.1 */
#define MT7996_RRO_MSDU_PG_CR_CNT 8
#define MT7996_RRO_MSDU_PG_SIZE_PER_CR 0x10000

#define to_rssi(field, rcpi)	((FIELD_GET(field, rcpi) - 220) / 2)

struct mt7996_vif;
struct mt7996_sta;
struct mt7996_dfs_pulse;
struct mt7996_dfs_pattern;

enum mt7996_fem_type {
	MT7996_FEM_UNSET,
	MT7996_FEM_EXT,
	MT7996_FEM_INT,
	MT7996_FEM_MIX,
};

enum mt7996_sku_type {
	MT7996_SKU_404,
	MT7996_SKU_444,
	MT7996_SKU_233,
};

enum mt7992_sku_type {
	MT7992_SKU_23,
	MT7992_SKU_24,
	MT7992_SKU_44,
};

enum mt7996_ram_type {
	MT7996_RAM_TYPE_WM,
	MT7996_RAM_TYPE_WM_TM = MT7996_RAM_TYPE_WM,
	MT7996_RAM_TYPE_WA,
	MT7996_RAM_TYPE_DSP,
	__MT7996_RAM_TYPE_MAX,
};

enum mt7996_coredump_state {
	MT7996_COREDUMP_IDLE = 0,
	MT7996_COREDUMP_MANUAL_WA,
	MT7996_COREDUMP_MANUAL_WM,
	MT7996_COREDUMP_AUTO,
	__MT7996_COREDUMP_TYPE_MAX,
};

enum mt7996_txq_id {
	MT7996_TXQ_FWDL = 16,
	MT7996_TXQ_MCU_WM,
	MT7996_TXQ_BAND0,
	MT7996_TXQ_BAND1,
	MT7996_TXQ_MCU_WA,
	MT7996_TXQ_BAND2,
};

enum mt7996_rxq_id {
	MT7996_RXQ_MCU_WM = 0,
	MT7996_RXQ_MCU_WA,
	MT7996_RXQ_MCU_WA_MAIN = 2,
	MT7996_RXQ_MCU_WA_EXT = 3, /* for mt7992 */
	MT7996_RXQ_MCU_WA_TRI = 3,
	MT7996_RXQ_BAND0 = 4,
	MT7996_RXQ_BAND1 = 5, /* for mt7992 */
	MT7996_RXQ_BAND2 = 5,
	MT7996_RXQ_RRO_BAND0 = 8,
	MT7996_RXQ_RRO_BAND1 = 9,
	MT7996_RXQ_RRO_BAND2 = 6,
	MT7996_RXQ_MSDU_PG_BAND0 = 10,
	MT7996_RXQ_MSDU_PG_BAND1 = 11,
	MT7996_RXQ_MSDU_PG_BAND2 = 12,
	MT7996_RXQ_TXFREE0 = 9,
	MT7996_RXQ_TXFREE1 = 9,
	MT7996_RXQ_TXFREE2 = 7,
	MT7996_RXQ_RRO_IND = 0,
};

struct mt7996_twt_flow {
	struct list_head list;
	u64 start_tsf;
	u64 tsf;
	u32 duration;
	u16 wcid;
	__le16 mantissa;
	u8 exp;
	u8 table_id;
	u8 id;
	u8 protection:1;
	u8 flowtype:1;
	u8 trigger:1;
	u8 sched:1;
};

DECLARE_EWMA(avg_signal, 10, 8)

enum mt7996_dpd_ch_num {
	DPD_CH_NUM_BW20_2G,
	DPD_CH_NUM_BW20_5G,
	DPD_CH_NUM_BW20_5G_SKIP,
	DPD_CH_NUM_BW80_5G,
	DPD_CH_NUM_BW160_5G,
	DPD_CH_NUM_BW20_6G,
	DPD_CH_NUM_BW80_6G,
	DPD_CH_NUM_BW160_6G,
	DPD_CH_NUM_BW320_6G,
	DPD_CH_NUM_TYPE_MAX,
};

enum {
	VOW_SEARCH_AC_FIRST,
	VOW_SEARCH_WMM_FIRST
};

enum {
	VOW_REFILL_PERIOD_1US,
	VOW_REFILL_PERIOD_2US,
	VOW_REFILL_PERIOD_4US,
	VOW_REFILL_PERIOD_8US,
	VOW_REFILL_PERIOD_16US,
	VOW_REFILL_PERIOD_32US,
	VOW_REFILL_PERIOD_64US,
	VOW_REFILL_PERIOD_128US
};

/* Default DRR airtime quantum of each level */
enum {
	VOW_DRR_QUANTUM_L0 = 6,
	VOW_DRR_QUANTUM_L1 = 12,
	VOW_DRR_QUANTUM_L2 = 16,
	VOW_DRR_QUANTUM_L3 = 20,
	VOW_DRR_QUANTUM_L4 = 24,
	VOW_DRR_QUANTUM_L5 = 28,
	VOW_DRR_QUANTUM_L6 = 32,
	VOW_DRR_QUANTUM_L7 = 36
};

enum {
	VOW_DRR_QUANTUM_IDX0,
	VOW_DRR_QUANTUM_IDX1,
	VOW_DRR_QUANTUM_IDX2,
	VOW_DRR_QUANTUM_IDX3,
	VOW_DRR_QUANTUM_IDX4,
	VOW_DRR_QUANTUM_IDX5,
	VOW_DRR_QUANTUM_IDX6,
	VOW_DRR_QUANTUM_IDX7,
	VOW_DRR_QUANTUM_NUM
};

enum {
	VOW_SCH_TYPE_FOLLOW_POLICY,
	VOW_SCH_TYPE_FOLLOW_HW
};

enum {
	VOW_SCH_POLICY_SRR, /* Shared Round-Robin */
	VOW_SCH_POLICY_WRR /* Weighted Round-Robin */
};

enum vow_drr_ctrl_id {
	VOW_DRR_CTRL_STA_ALL,
	VOW_DRR_CTRL_STA_BSS_GROUP,
	VOW_DRR_CTRL_AIRTIME_DEFICIT_BOUND = 0x10,
	VOW_DRR_CTRL_AIRTIME_QUANTUM_ALL = 0x28,
	VOW_DRR_CTRL_STA_PAUSE = 0x30
};

struct mt7996_vow_ctrl {
	bool atf_enable;
	bool watf_enable;
	u8 drr_quantum[VOW_DRR_QUANTUM_NUM];
	u8 max_deficit;
	u8 sch_type;
	u8 sch_policy;
};

struct mt7996_vow_sta_ctrl {
	bool paused;
	u8 bss_grp_idx;
	u8 drr_quantum[IEEE80211_NUM_ACS];
};

struct mt7996_sta {
	struct mt76_wcid wcid; /* must be first */

	struct mt7996_vif *vif;

	struct list_head rc_list;

	int ack_signal;
	struct ewma_avg_signal avg_ack_signal;

	unsigned long changed;

	struct mt76_connac_sta_key_conf bip;

	struct {
		u8 flowid_mask;
		struct mt7996_twt_flow flow[MT7996_MAX_STA_TWT_AGRT];
	} twt;

	struct mt7996_vow_sta_ctrl vow;
};

struct mt7996_vif {
	struct mt76_vif mt76; /* must be first */

	struct mt7996_sta sta;
	struct mt7996_phy *phy;

	struct ieee80211_tx_queue_params queue_params[IEEE80211_NUM_ACS];
	struct cfg80211_bitrate_mask bitrate_mask;
};

/* crash-dump */
struct mt7996_crash_data {
	guid_t guid;
	struct timespec64 timestamp;
	bool supported;

	u8 *memdump_buf;
	size_t memdump_buf_len;
};

struct mt7996_hif {
	struct list_head list;

	struct device *dev;
	void __iomem *regs;
	int irq;
};

struct mt7996_scs_ctrl {
	u8 scs_enable;
	s8 sta_min_rssi;
};

enum {
	SCS_DISABLE = 0,
	SCS_ENABLE,
};

struct mt7996_wed_rro_addr {
	u32 head_low;
	u32 head_high : 4;
	u32 count: 11;
	u32 oor: 1;
	u32 rsv : 8;
	u32 signature : 8;
};

struct mt7996_wed_rro_session_id {
	struct list_head list;
	u16 id;
};

#ifdef CONFIG_MTK_VENDOR
#define MT7996_AIR_MONITOR_MAX_ENTRY	16
#define MT7996_AIR_MONITOR_MAX_GROUP	(MT7996_AIR_MONITOR_MAX_ENTRY >> 1)

struct mt7996_air_monitor_group {
	bool enable;
	bool used[2];
};

struct mt7996_air_monitor_entry {
	bool enable;

	u8 group_idx;
	u8 group_used_idx;
	u8 muar_idx;
	u8 addr[ETH_ALEN];
	u32 last_seen;
	s8 rssi[4];
	struct ieee80211_sta *sta;
};

struct mt7996_air_monitor_ctrl {
	u8 enable;
	struct mt7996_air_monitor_group group[MT7996_AIR_MONITOR_MAX_GROUP];
	struct mt7996_air_monitor_entry entry[MT7996_AIR_MONITOR_MAX_ENTRY];
};
#endif

struct mt7996_rro_ba_session {
	u32 ack_sn         :12;
	u32 win_sz         :3;
	u32 bn             :1;
	u32 last_in_sn     :12;
	u32 bc             :1;
	u32 bd             :1;
	u32 sat            :1;
	u32 cn             :1;
	u32 within_cnt     :12;
	u32 to_sel         :3;
	u32 rsv            :1;
	u32 last_in_rxtime :12;
};

struct mt7996_phy {
	struct mt76_phy *mt76;
	struct mt7996_dev *dev;

	struct ieee80211_sband_iftype_data iftype[NUM_NL80211_BANDS][NUM_NL80211_IFTYPES];

	struct ieee80211_vif *monitor_vif;

	struct thermal_cooling_device *cdev;
	u8 cdev_state;
	u8 throttle_state;
	u32 throttle_temp[2]; /* 0: critical high, 1: maximum */

	u32 rxfilter;
	u64 omac_mask;

	u16 noise;

	s16 coverage_class;
	u8 slottime;

	u8 rdd_state;

	u16 beacon_rate;

	u32 rx_ampdu_ts;
	u32 ampdu_ref;

	struct mt76_mib_stats mib;
	struct mt76_channel_state state_ts;
	struct delayed_work ipi_work;

	bool has_aux_rx;

	struct mt7996_scs_ctrl scs_ctrl;
	u32 red_drop;

	u8 muru_onoff;

#ifdef CONFIG_NL80211_TESTMODE
	struct {
		u32 *reg_backup;

		s32 last_freq_offset;
		u8 last_rcpi[4];
		s8 last_rssi[4];
		s8 last_ib_rssi[4];
		s8 last_wb_rssi[4];
		u8 last_snr;

		u8 spe_idx;
	} test;
#endif
#ifdef CONFIG_MTK_VENDOR
	u8 rts_bw_sig;
	spinlock_t amnt_lock;
	struct mt7996_air_monitor_ctrl amnt_ctrl;
#endif
#ifdef CONFIG_MTK_DEBUG
	bool sr_enable:1;
	bool enhanced_sr_enable:1;
#endif
};

struct mt7996_dev {
	union { /* must be first */
		struct mt76_dev mt76;
		struct mt76_phy mphy;
	};

	struct mt7996_hif *hif2;
	struct mt7996_reg_desc reg;
	u8 q_id[MT7996_MAX_QUEUE];
	u32 q_int_mask[MT7996_MAX_QUEUE];
	u32 q_wfdma_mask;

	const struct mt76_bus_ops *bus_ops;
	struct mt7996_phy phy;

	/* monitor rx chain configured channel */
	struct cfg80211_chan_def rdd2_chandef;
	struct mt7996_phy *rdd2_phy;

	u8 chip_sku;
	u8 fem_type;

	u16 chainmask;
	u8 chainshift[__MT_MAX_BAND];
	u32 hif_idx;

	struct work_struct init_work;
	struct work_struct rc_work;
	struct work_struct dump_work;
	struct work_struct reset_work;
	struct delayed_work scs_work;
	wait_queue_head_t reset_wait;
	struct {
		u32 state;
		u32 l1_reset;
		u32 l1_reset_last;
		u32 wa_reset_count;
		u32 wm_reset_count;
		bool hw_full_reset:1;
		bool hw_init_done:1;
		bool restart:1;
	} recovery;

	/* protects coredump data */
	struct mutex dump_mutex;
	u8 dump_state;
#ifdef CONFIG_DEV_COREDUMP
	struct {
		struct mt7996_crash_data *crash_data[__MT7996_RAM_TYPE_MAX];
	} coredump;
#endif
	char patch_build_date[MT7996_BUILD_TIME_LEN];
	char ram_build_date[__MT7996_RAM_TYPE_MAX][MT7996_BUILD_TIME_LEN];

	struct list_head sta_rc_list;
	struct list_head twt_list;

	u32 hw_pattern;

	bool flash_mode:1;
	bool has_eht:1;
	bool has_rro:1;

	struct {
		struct {
			void *ptr;
			dma_addr_t phy_addr;
		} ba_bitmap[MT7996_RRO_BA_BITMAP_LEN];
		struct {
			void *ptr;
			dma_addr_t phy_addr;
		} addr_elem[MT7996_RRO_ADDR_ELEM_LEN];
		struct {
			void *ptr;
			dma_addr_t phy_addr;
		} session;
		struct {
			void *ptr;
			dma_addr_t phy_addr;
		} msdu_pg[MT7996_RRO_MSDU_PG_CR_CNT];

		struct work_struct work;
		struct list_head poll_list;
		spinlock_t lock;
	} wed_rro;

	bool testmode_enable;
	bool bin_file_mode;
	u8 eeprom_mode;
	u32 bg_nxt_freq;

	bool ibf;
	u8 fw_debug_wm;
	u8 fw_debug_wa;
	u8 fw_debug_bin;
	u16 fw_debug_seq;
	bool fw_debug_muru_disable;

	struct dentry *debugfs_dir;
	struct rchan *relay_fwlog;

	void *cal;
	u32 cur_prek_offset;
	struct {
		const u32 *rev;
		u8 dpd_ch_num[DPD_CH_NUM_TYPE_MAX];
	} prek;

	struct {
		u16 table_mask;
		u8 n_agrt;
	} twt;

	spinlock_t reg_lock;

	u8 wtbl_size_group;

	struct mt7996_vow_ctrl vow;

	bool wmm_pbc_enable;
	struct work_struct wmm_pbc_work;
#ifdef CONFIG_MTK_DEBUG
	u16 wlan_idx;
	struct {
		u8 sku_disable;
		u32 fw_dbg_module;
		u8 fw_dbg_lv;
		u32 bcn_total_cnt[__MT_MAX_BAND];
		u32 sid;
	} dbg;
	const struct mt7996_dbg_reg_desc *dbg_reg;
#endif
#ifdef CONFIG_MTK_VENDOR
	bool cert_mode;
#endif

#if defined CONFIG_NL80211_TESTMODE || defined CONFIG_MTK_DEBUG
	struct {
		void *txbf_phase_cal;
		void *txbf_pfmu_data;
		void *txbf_pfmu_tag;
	} test;
#endif
};

enum {
	WFDMA0 = 0x0,
	WFDMA1,
	WFDMA_EXT,
	__MT_WFDMA_MAX,
};

enum {
	MT_RX_SEL0,
	MT_RX_SEL1,
	MT_RX_SEL2, /* monitor chain */
};

enum mt7996_rdd_cmd {
	RDD_STOP,
	RDD_START,
	RDD_DET_MODE,
	RDD_RADAR_EMULATE,
	RDD_START_TXQ = 20,
	RDD_CAC_START = 50,
	RDD_CAC_END,
	RDD_NORMAL_START,
	RDD_DISABLE_DFS_CAL,
	RDD_PULSE_DBG,
	RDD_READ_PULSE,
	RDD_RESUME_BF,
	RDD_IRQ_OFF,
	RDD_DISABLE_ZW_TIMER,
};

#define RDD_ZW_TIMER_OFF	BIT(31)

static inline struct mt7996_phy *
mt7996_hw_phy(struct ieee80211_hw *hw)
{
	struct mt76_phy *phy = hw->priv;

	return phy->priv;
}

static inline struct mt7996_dev *
mt7996_hw_dev(struct ieee80211_hw *hw)
{
	struct mt76_phy *phy = hw->priv;

	return container_of(phy->dev, struct mt7996_dev, mt76);
}

static inline struct mt7996_phy *
__mt7996_phy(struct mt7996_dev *dev, enum mt76_band_id band)
{
	struct mt76_phy *phy = dev->mt76.phys[band];

	if (!phy)
		return NULL;

	return phy->priv;
}

static inline struct mt7996_phy *
mt7996_phy2(struct mt7996_dev *dev)
{
	return __mt7996_phy(dev, MT_BAND1);
}

static inline struct mt7996_phy *
mt7996_phy3(struct mt7996_dev *dev)
{
	return __mt7996_phy(dev, MT_BAND2);
}

static inline bool
mt7996_band_valid(struct mt7996_dev *dev, u8 band)
{
	if (is_mt7992(&dev->mt76))
		return band <= MT_BAND1;

	/* tri-band support */
	if (band <= MT_BAND2 && dev->chip_sku)
		return true;

	return band == MT_BAND0 || band == MT_BAND2;
}

static inline bool
mt7996_get_background_radar_cap(struct mt7996_dev *dev)
{
	switch (mt76_chip(&dev->mt76)) {
	case 0x7990:
		if (dev->chip_sku == MT7996_SKU_233)
			return 0;
		break;
	case 0x7992:
		if (dev->chip_sku == MT7992_SKU_23 ||
		    dev->chip_sku == MT7992_SKU_24)
			return 0;
		break;
	default:
		break;
	}

	return 1;
}

extern const struct ieee80211_ops mt7996_ops;
extern struct pci_driver mt7996_pci_driver;
extern struct pci_driver mt7996_hif_driver;
extern const struct mt76_testmode_ops mt7996_testmode_ops;

struct mt7996_dev *mt7996_mmio_probe(struct device *pdev,
				     void __iomem *mem_base, u32 device_id);
void mt7996_wfsys_reset(struct mt7996_dev *dev);
void mt7996_rro_hw_init(struct mt7996_dev *dev);
irqreturn_t mt7996_irq_handler(int irq, void *dev_instance);
u64 __mt7996_get_tsf(struct ieee80211_hw *hw, struct mt7996_vif *mvif);
int mt7996_register_device(struct mt7996_dev *dev);
void mt7996_unregister_device(struct mt7996_dev *dev);
const char *mt7996_eeprom_name(struct mt7996_dev *dev);
int mt7996_eeprom_init(struct mt7996_dev *dev);
int mt7996_eeprom_check_fw_mode(struct mt7996_dev *dev);
int mt7996_eeprom_parse_hw_cap(struct mt7996_dev *dev, struct mt7996_phy *phy);
int mt7996_eeprom_get_target_power(struct mt7996_dev *dev,
				   struct ieee80211_channel *chan);
s8 mt7996_eeprom_get_power_delta(struct mt7996_dev *dev, int band);
int mt7996_dma_init(struct mt7996_dev *dev);
void mt7996_dma_reset(struct mt7996_dev *dev, bool force);
void mt7996_dma_prefetch(struct mt7996_dev *dev);
void mt7996_dma_cleanup(struct mt7996_dev *dev);
void mt7996_dma_start(struct mt7996_dev *dev, bool reset, bool wed_reset);
int mt7996_init_tx_queues(struct mt7996_phy *phy, int idx,
			  int n_desc, int ring_base, struct mtk_wed_device *wed);
void mt7996_init_txpower(struct mt7996_phy *phy);
int mt7996_txbf_init(struct mt7996_dev *dev);
int mt7996_get_chip_sku(struct mt7996_dev *dev);
void mt7996_reset(struct mt7996_dev *dev);
void mt7996_coredump(struct mt7996_dev *dev, u8 state);
int mt7996_run(struct ieee80211_hw *hw);
int mt7996_mcu_init(struct mt7996_dev *dev);
int mt7996_mcu_init_firmware(struct mt7996_dev *dev);
int mt7996_mcu_twt_agrt_update(struct mt7996_dev *dev,
			       struct mt7996_vif *mvif,
			       struct mt7996_twt_flow *flow,
			       int cmd);
int mt7996_mcu_add_dev_info(struct mt7996_phy *phy,
			    struct ieee80211_vif *vif, bool enable);
int mt7996_mcu_add_bss_info(struct mt7996_phy *phy,
			    struct ieee80211_vif *vif, int enable);
int mt7996_mcu_add_sta(struct mt7996_dev *dev, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta, bool enable);
int mt7996_mcu_add_tx_ba(struct mt7996_dev *dev,
			 struct ieee80211_ampdu_params *params,
			 bool add);
int mt7996_mcu_add_rx_ba(struct mt7996_dev *dev,
			 struct ieee80211_ampdu_params *params,
			 bool add);
int mt7996_mcu_update_bss_color(struct mt7996_dev *dev, struct ieee80211_vif *vif,
				struct cfg80211_he_bss_color *he_bss_color);
int mt7996_mcu_add_beacon(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			  int enable);
int mt7996_mcu_beacon_inband_discov(struct mt7996_dev *dev,
				    struct ieee80211_vif *vif, u32 changed);
int mt7996_mcu_add_obss_spr(struct mt7996_phy *phy, struct ieee80211_vif *vif,
			    struct ieee80211_he_obss_pd *he_obss_pd);
int mt7996_mcu_add_rate_ctrl(struct mt7996_dev *dev, struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta, bool changed);
int mt7996_set_channel(struct mt7996_phy *phy);
int mt7996_mcu_set_chan_info(struct mt7996_phy *phy, u16 tag);
int mt7996_mcu_set_tx(struct mt7996_dev *dev, struct ieee80211_vif *vif);
int mt7996_mcu_set_fixed_rate_ctrl(struct mt7996_dev *dev,
				   void *data, u16 version);
int mt7996_mcu_set_fixed_field(struct mt7996_dev *dev, struct ieee80211_vif *vif,
			       struct ieee80211_sta *sta, void *data, u32 field);
int mt7996_mcu_set_eeprom(struct mt7996_dev *dev);
int mt7996_mcu_get_eeprom(struct mt7996_dev *dev, u32 offset, u8 *read_buf);
int mt7996_mcu_get_eeprom_free_block(struct mt7996_dev *dev, u8 *block_num);
int mt7996_mcu_get_chip_config(struct mt7996_dev *dev, u32 *cap);
int mt7996_mcu_set_ser(struct mt7996_dev *dev, u8 action, u8 set, u8 band);
int mt7996_mcu_set_txbf(struct mt7996_dev *dev, u8 action);
int mt7996_mcu_set_fcc5_lpn(struct mt7996_dev *dev, int val);
int mt7996_mcu_set_pulse_th(struct mt7996_dev *dev,
			    const struct mt7996_dfs_pulse *pulse);
int mt7996_mcu_set_radar_th(struct mt7996_dev *dev, int index,
			    const struct mt7996_dfs_pattern *pattern);
int mt7996_mcu_set_radio_en(struct mt7996_phy *phy, bool enable);
int mt7996_mcu_set_rts_thresh(struct mt7996_phy *phy, u32 val);
int mt7996_mcu_set_timing(struct mt7996_phy *phy, struct ieee80211_vif *vif);
int mt7996_mcu_get_chan_mib_info(struct mt7996_phy *phy, bool chan_switch);
int mt7996_mcu_get_temperature(struct mt7996_phy *phy);
int mt7996_mcu_set_thermal_throttling(struct mt7996_phy *phy, u8 state);
int mt7996_mcu_set_thermal_protect(struct mt7996_phy *phy, bool enable);
int mt7996_mcu_set_txpower_sku(struct mt7996_phy *phy);
int mt7996_mcu_rdd_cmd(struct mt7996_dev *dev, int cmd, u8 index,
		       u8 rx_sel, u8 val);
int mt7996_mcu_rdd_background_disable_timer(struct mt7996_dev *dev,
					    bool disable_timer);
int mt7996_mcu_rdd_background_enable(struct mt7996_phy *phy,
				     struct cfg80211_chan_def *chandef);
int mt7996_mcu_set_fixed_rate_table(struct mt7996_phy *phy, u8 table_idx,
				    u16 rate_idx, bool beacon);
int mt7996_mcu_rf_regval(struct mt7996_dev *dev, u32 regidx, u32 *val, bool set);
int mt7996_mcu_set_hdr_trans(struct mt7996_dev *dev, bool hdr_trans);
int mt7996_mcu_set_rro(struct mt7996_dev *dev, u16 tag, u16 val);
int mt7996_mcu_wa_cmd(struct mt7996_dev *dev, int cmd, u32 a1, u32 a2, u32 a3);
int mt7996_mcu_red_config(struct mt7996_dev *dev, bool enable);
int mt7996_mcu_fw_log_2_host(struct mt7996_dev *dev, u8 type, u8 ctrl);
int mt7996_mcu_fw_dbg_ctrl(struct mt7996_dev *dev, u32 module, u8 level);
int mt7996_mcu_trigger_assert(struct mt7996_dev *dev);
void mt7996_mcu_rx_event(struct mt7996_dev *dev, struct sk_buff *skb);
void mt7996_mcu_exit(struct mt7996_dev *dev);
int mt7996_mcu_get_per_sta_info(struct mt76_dev *dev, u16 tag,
	                        u16 sta_num, u16 *sta_list);
int mt7996_mcu_get_rssi(struct mt76_dev *dev);
int mt7996_mcu_get_all_sta_info(struct mt76_dev *dev, u16 tag);
int mt7996_mcu_wed_rro_reset_sessions(struct mt7996_dev *dev, u16 id);
int mt7996_mcu_set_tx_power_ctrl(struct mt7996_phy *phy, u8 power_ctrl_id, u8 data);
int mt7996_mcu_get_tx_power_info(struct mt7996_phy *phy, u8 category, void *event);
int mt7996_mcu_apply_group_cal(struct mt7996_dev *dev);
int mt7996_mcu_apply_tx_dpd(struct mt7996_phy *phy);
#ifdef CONFIG_NL80211_TESTMODE
void mt7996_tm_rf_test_event(struct mt7996_dev *dev, struct sk_buff *skb);
#endif
int mt7996_mcu_set_scs(struct mt7996_phy *phy, u8 enable);
void mt7996_mcu_scs_sta_poll(struct work_struct *work);
int mt7996_mcu_set_band_confg(struct mt7996_phy *phy, u16 option, bool enable);
int mt7996_mcu_set_vow_drr_ctrl(struct mt7996_phy *phy, struct mt7996_sta *msta,
	                        enum vow_drr_ctrl_id id);
int mt7996_mcu_set_vow_feature_ctrl(struct mt7996_phy *phy);
void mt7996_mcu_wmm_pbc_work(struct work_struct *work);

static inline u8 mt7996_max_interface_num(struct mt7996_dev *dev)
{
	return min(MT7996_MAX_INTERFACES * (1 + mt7996_band_valid(dev, MT_BAND1) +
					    mt7996_band_valid(dev, MT_BAND2)),
		   MT7996_WTBL_BMC_SIZE);
}

static inline u16 mt7996_wtbl_size(struct mt7996_dev *dev)
{
	return (dev->wtbl_size_group << 8) + MT7996_WTBL_BMC_SIZE;
}

void mt7996_dual_hif_set_irq_mask(struct mt7996_dev *dev, bool write_reg,
				  u32 clear, u32 set);

static inline void mt7996_irq_enable(struct mt7996_dev *dev, u32 mask)
{
	if (dev->hif2)
		mt7996_dual_hif_set_irq_mask(dev, false, 0, mask);
	else
		mt76_set_irq_mask(&dev->mt76, 0, 0, mask);

	tasklet_schedule(&dev->mt76.irq_tasklet);
}

static inline void mt7996_irq_disable(struct mt7996_dev *dev, u32 mask)
{
	if (dev->hif2)
		mt7996_dual_hif_set_irq_mask(dev, true, mask, 0);
	else
		mt76_set_irq_mask(&dev->mt76, MT_INT_MASK_CSR, mask, 0);
}

void mt7996_memcpy_fromio(struct mt7996_dev *dev, void *buf, u32 offset,
			  size_t len);

static inline u16 mt7996_rx_chainmask(struct mt7996_phy *phy)
{
	int max_nss = hweight8(phy->mt76->hw->wiphy->available_antennas_tx);
	int cur_nss = hweight8(phy->mt76->antenna_mask);
	u16 tx_chainmask = phy->mt76->chainmask;

	if (cur_nss != max_nss)
		return tx_chainmask;

	return tx_chainmask | (BIT(fls(tx_chainmask)) * phy->has_aux_rx);
}

static inline bool
mt7996_vow_should_enable(struct mt7996_dev *dev)
{
	return !wiphy_ext_feature_isset(mt76_hw(dev)->wiphy,
	                                NL80211_EXT_FEATURE_AIRTIME_FAIRNESS) ||
	       mtk_wed_device_active(&dev->mt76.mmio.wed);
}

void mt7996_mac_init(struct mt7996_dev *dev);
u32 mt7996_mac_wtbl_lmac_addr(struct mt7996_dev *dev, u16 wcid, u8 dw);
bool mt7996_mac_wtbl_update(struct mt7996_dev *dev, int idx, u32 mask);
void mt7996_mac_reset_counters(struct mt7996_phy *phy);
void mt7996_mac_cca_stats_reset(struct mt7996_phy *phy);
void mt7996_mac_enable_nf(struct mt7996_dev *dev, u8 band);
void mt7996_mac_enable_rtscts(struct mt7996_dev *dev,
			      struct ieee80211_vif *vif, bool enable);
void mt7996_mac_write_txwi(struct mt7996_dev *dev, __le32 *txwi,
			   struct sk_buff *skb, struct mt76_wcid *wcid,
			   struct ieee80211_key_conf *key, int pid,
			   enum mt76_txq_id qid, u32 changed);
void mt7996_mac_set_coverage_class(struct mt7996_phy *phy);
int mt7996_mac_sta_add(struct mt76_dev *mdev, struct ieee80211_vif *vif,
		       struct ieee80211_sta *sta);
void mt7996_mac_sta_remove(struct mt76_dev *mdev, struct ieee80211_vif *vif,
			   struct ieee80211_sta *sta);
void mt7996_mac_work(struct work_struct *work);
void mt7996_mac_reset_work(struct work_struct *work);
void mt7996_mac_dump_work(struct work_struct *work);
void mt7996_mac_sta_rc_work(struct work_struct *work);
void mt7996_mac_update_stats(struct mt7996_phy *phy);
void mt7996_mac_twt_teardown_flow(struct mt7996_dev *dev,
				  struct mt7996_sta *msta,
				  u8 flowid);
void mt7996_mac_add_twt_setup(struct ieee80211_hw *hw,
			      struct ieee80211_sta *sta,
			      struct ieee80211_twt_setup *twt);
int mt7996_tx_prepare_skb(struct mt76_dev *mdev, void *txwi_ptr,
			  enum mt76_txq_id qid, struct mt76_wcid *wcid,
			  struct ieee80211_sta *sta,
			  struct mt76_tx_info *tx_info);
void mt7996_tx_token_put(struct mt7996_dev *dev);
void mt7996_queue_rx_skb(struct mt76_dev *mdev, enum mt76_rxq_id q,
			 struct sk_buff *skb, u32 *info);
bool mt7996_rx_check(struct mt76_dev *mdev, void *data, int len);
void mt7996_stats_work(struct work_struct *work);
int mt76_dfs_start_rdd(struct mt7996_dev *dev, bool force);
int mt7996_dfs_init_radar_detector(struct mt7996_phy *phy);
void mt7996_set_stream_he_eht_caps(struct mt7996_phy *phy);
void mt7996_set_stream_vht_txbf_caps(struct mt7996_phy *phy);
void mt7996_update_channel(struct mt76_phy *mphy);
int mt7996_init_debugfs(struct mt7996_phy *phy);
void mt7996_debugfs_rx_fw_monitor(struct mt7996_dev *dev, const void *data, int len);
bool mt7996_debugfs_rx_log(struct mt7996_dev *dev, const void *data, int len);
int mt7996_mcu_add_key(struct mt76_dev *dev, struct ieee80211_vif *vif,
		       struct ieee80211_key_conf *key, int mcu_cmd,
		       struct mt76_wcid *wcid, enum set_key_cmd cmd);
int mt7996_mcu_bcn_prot_enable(struct mt7996_dev *dev, struct ieee80211_vif *vif,
			       struct ieee80211_key_conf *key);
int mt7996_mcu_wtbl_update_hdr_trans(struct mt7996_dev *dev,
				     struct ieee80211_vif *vif,
				     struct ieee80211_sta *sta);
int mt7996_mcu_set_pp_en(struct mt7996_phy *phy, bool auto_mode, u8 force_bitmap,
			 u16 bitmap);
#ifdef CONFIG_MAC80211_DEBUGFS
void mt7996_sta_add_debugfs(struct ieee80211_hw *hw, struct ieee80211_vif *vif,
			    struct ieee80211_sta *sta, struct dentry *dir);
#endif
int mt7996_mmio_wed_init(struct mt7996_dev *dev, void *pdev_ptr,
			 bool hif2, int *irq);
u32 mt7996_wed_init_buf(void *ptr, dma_addr_t phys, int token_id);

#ifdef CONFIG_MTK_VENDOR
void mt7996_set_wireless_vif(void *data, u8 *mac, struct ieee80211_vif *vif);
void mt7996_vendor_register(struct mt7996_phy *phy);
void mt7996_vendor_amnt_fill_rx(struct mt7996_phy *phy, struct sk_buff *skb);
int mt7996_vendor_amnt_sta_remove(struct mt7996_phy *phy,
				  struct ieee80211_sta *sta);
void mt7996_set_wireless_amsdu(struct ieee80211_hw *hw, u8 en);
void mt7996_mcu_set_mimo(struct mt7996_phy *phy);
int mt7996_set_muru_cfg(struct mt7996_phy *phy, u8 action, u8 val);
int mt7996_mcu_set_muru_cfg(struct mt7996_phy *phy, void *data);
void mt7996_set_beacon_vif(void *data, u8 *mac, struct ieee80211_vif *vif);
#endif

int mt7996_mcu_edcca_enable(struct mt7996_phy *phy, bool enable);
int mt7996_mcu_edcca_threshold_ctrl(struct mt7996_phy *phy, u8 *value, bool set);

enum edcca_bw_id {
	EDCCA_BW_20 = 0,
	EDCCA_BW_40,
	EDCCA_BW_80,
	EDCCA_BW_160,
	EDCCA_MAX_BW_NUM,
};

#ifdef CONFIG_MTK_DEBUG
int mt7996_mtk_init_debugfs(struct mt7996_phy *phy, struct dentry *dir);
int mt7996_mcu_muru_dbg_info(struct mt7996_dev *dev, u16 item, u8 val);
int mt7996_mcu_set_sr_enable(struct mt7996_phy *phy, u8 action, u64 val, bool set);
void mt7996_mcu_rx_sr_event(struct mt7996_dev *dev, struct sk_buff *skb);
int mt7996_mcu_set_dup_wtbl(struct mt7996_dev *dev);
int mt7996_mcu_set_txbf_internal(struct mt7996_phy *phy, u8 action, int idx, bool bfer);
void mt7996_mcu_rx_bf_event(struct mt7996_dev *dev, struct sk_buff *skb);
int mt7996_mcu_set_muru_fixed_rate_enable(struct mt7996_dev *dev, u8 action, int val);
int mt7996_mcu_set_muru_fixed_rate_parameter(struct mt7996_dev *dev, u8 action, void *para);
int mt7996_mcu_set_txbf_snd_info(struct mt7996_phy *phy, void *para);
int mt7996_mcu_set_muru_cmd(struct mt7996_dev *dev, u16 action, int val);
int mt7996_mcu_muru_set_prot_frame_thr(struct mt7996_dev *dev, u32 val);
int mt7996_mcu_set_bypass_smthint(struct mt7996_phy *phy, u8 val);
int mt7996_mcu_set_rfeature_trig_type(struct mt7996_phy *phy, u8 enable, u8 trig_type);
void mt7996_mcu_set_ppdu_tx_type(struct mt7996_phy *phy, u8 ppdu_type);
void mt7996_mcu_set_nusers_ofdma(struct mt7996_phy *phy, u8 type, u8 ofdma_user_cnt);
void mt7996_mcu_set_cert(struct mt7996_phy *phy, u8 type);
void mt7996_tm_update_channel(struct mt7996_phy *phy);

int mt7996_mcu_set_vow_drr_dbg(struct mt7996_dev *dev, u32 val);
#endif

#ifdef CONFIG_NET_MEDIATEK_SOC_WED
int mt7996_dma_rro_init(struct mt7996_dev *dev);
#endif /* CONFIG_NET_MEDIATEK_SOC_WED */

#endif