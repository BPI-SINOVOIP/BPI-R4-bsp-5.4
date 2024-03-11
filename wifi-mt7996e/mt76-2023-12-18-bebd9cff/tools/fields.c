// SPDX-License-Identifier: ISC
/* Copyright (C) 2020 Felix Fietkau <nbd@nbd.name> */
#define _GNU_SOURCE
#include "mt76-test.h"

static char prefix[64];

static const char * const testmode_state[] = {
	[MT76_TM_STATE_OFF] = "off",
	[MT76_TM_STATE_IDLE] = "idle",
	[MT76_TM_STATE_TX_FRAMES] = "tx_frames",
	[MT76_TM_STATE_RX_FRAMES] = "rx_frames",
	[MT76_TM_STATE_TX_CONT] = "tx_cont",
	[MT76_TM_STATE_GROUP_PREK] = "group_prek",
	[MT76_TM_STATE_GROUP_PREK_DUMP] = "group_prek_dump",
	[MT76_TM_STATE_GROUP_PREK_CLEAN] = "group_prek_clean",
	[MT76_TM_STATE_DPD_2G] = "dpd_2g",
	[MT76_TM_STATE_DPD_5G] = "dpd_5g",
	[MT76_TM_STATE_DPD_6G] = "dpd_6g",
	[MT76_TM_STATE_DPD_DUMP] = "dpd_dump",
	[MT76_TM_STATE_DPD_CLEAN] = "dpd_clean",
};

static const char * const testmode_tx_mode[] = {
	[MT76_TM_TX_MODE_CCK] = "cck",
	[MT76_TM_TX_MODE_OFDM] = "ofdm",
	[MT76_TM_TX_MODE_HT] = "ht",
	[MT76_TM_TX_MODE_VHT] = "vht",
	[MT76_TM_TX_MODE_HE_SU] = "he_su",
	[MT76_TM_TX_MODE_HE_EXT_SU] = "he_ext_su",
	[MT76_TM_TX_MODE_HE_TB] = "he_tb",
	[MT76_TM_TX_MODE_HE_MU] = "he_mu",
	[MT76_TM_TX_MODE_EHT_SU] = "eht_su",
	[MT76_TM_TX_MODE_EHT_TRIG] = "eht_tb",
	[MT76_TM_TX_MODE_EHT_MU] = "eht_mu",
};

static const char * const testmode_offchan_bw[] = {
	[NL80211_CHAN_WIDTH_20_NOHT] = "NOHT",
	[NL80211_CHAN_WIDTH_20] = "20",
	[NL80211_CHAN_WIDTH_40] = "40",
	[NL80211_CHAN_WIDTH_80] = "80",
	[NL80211_CHAN_WIDTH_80P80] = "80p80",
	[NL80211_CHAN_WIDTH_160] = "160",
};

static const char * const testmode_txbf_act[] = {
	[MT76_TM_TXBF_ACT_GOLDEN_INIT] = "golden_init",
	[MT76_TM_TXBF_ACT_INIT] = "init",
	[MT76_TM_TX_EBF_ACT_GOLDEN_INIT] = "ebf_golden_init",
	[MT76_TM_TX_EBF_ACT_INIT] = "ebf_init",
	[MT76_TM_TXBF_ACT_UPDATE_CH] = "update_ch",
	[MT76_TM_TXBF_ACT_PHASE_COMP] = "phase_comp",
	[MT76_TM_TXBF_ACT_TX_PREP] = "tx_prep",
	[MT76_TM_TXBF_ACT_IBF_PROF_UPDATE] = "ibf_prof_update",
	[MT76_TM_TXBF_ACT_EBF_PROF_UPDATE] = "ebf_prof_update",
	[MT76_TM_TXBF_ACT_APPLY_TX] = "apply_tx",
	[MT76_TM_TXBF_ACT_PHASE_CAL] = "phase_cal",
	[MT76_TM_TXBF_ACT_PROF_UPDATE_ALL] = "prof_update",
	[MT76_TM_TXBF_ACT_PROF_UPDATE_ALL_CMD] = "prof_update_all",
	[MT76_TM_TXBF_ACT_E2P_UPDATE] = "e2p_update",
	[MT76_TM_TXBF_ACT_TRIGGER_SOUNDING] = "trigger_sounding",
	[MT76_TM_TXBF_ACT_STOP_SOUNDING] = "stop_sounding",
	[MT76_TM_TXBF_ACT_PROFILE_TAG_READ] = "pfmu_tag_read",
	[MT76_TM_TXBF_ACT_PROFILE_TAG_WRITE] = "pfmu_tag_write",
	[MT76_TM_TXBF_ACT_PROFILE_TAG_INVALID] = "set_invalid_prof",
	[MT76_TM_TXBF_ACT_STA_REC_READ] = "sta_rec_read",
	[MT76_TM_TXBF_ACT_TXCMD] = "txcmd",
};

static void print_enum(const struct tm_field *field, struct nlattr *attr)
{
	unsigned int i = nla_get_u8(attr);

	if (i < field->enum_len && field->enum_str[i])
		printf("%s", field->enum_str[i]);
	else
		printf("unknown (%d)", i);
}

static bool parse_enum(const struct tm_field *field, int idx,
		       struct nl_msg *msg, const char *val)
{
	int i;

	for (i = 0; i < field->enum_len; i++) {
		if (strcmp(field->enum_str[i], val) != 0)
			continue;

		if (nla_put_u8(msg, idx, i))
			return false;

		return true;
	}

	printf("Invalid value for parameter '%s': %s\n", field->name, val);
	printf("Possible values:");
	for (i = 0; i < field->enum_len; i++)
		printf(" %s", field->enum_str[i]);
	printf("\n");

	return false;
}

static bool parse_u8(const struct tm_field *field, int idx,
		     struct nl_msg *msg, const char *val)
{
	return !nla_put_u8(msg, idx, strtoul(val, NULL, 0));
}

static void print_u8(const struct tm_field *field, struct nlattr *attr)
{
	printf("%u", nla_get_u8(attr));
}

static void print_s8(const struct tm_field *field, struct nlattr *attr)
{
	printf("%d", (int8_t)nla_get_u8(attr));
}

static bool parse_u16_hex(const struct tm_field *field, int idx,
			  struct nl_msg *msg, const char *val)
{
	return !nla_put_u16(msg, idx, strtoul(val, NULL, 16));
}

static void print_u16_hex(const struct tm_field *field, struct nlattr *attr)
{
	printf("%d", nla_get_u16(attr));
}

static bool parse_u32(const struct tm_field *field, int idx,
		      struct nl_msg *msg, const char *val)
{
	return !nla_put_u32(msg, idx, strtoul(val, NULL, 0));
}

static void print_s32(const struct tm_field *field, struct nlattr *attr)
{
	printf("%d", (int32_t)nla_get_u32(attr));
}

static void print_u32(const struct tm_field *field, struct nlattr *attr)
{
	printf("%u", nla_get_u32(attr));
}

static void print_u64(const struct tm_field *field, struct nlattr *attr)
{
	printf("%llu", (unsigned long long)nla_get_u64(attr));
}

static bool parse_flag(const struct tm_field *field, int idx,
		       struct nl_msg *msg, const char *val)
{
	bool set = strtoul(val, NULL, 0);

	if (!set)
	    return true;

	return !nla_put_flag(msg, idx);
}

static void print_string(const struct tm_field *field, struct nlattr *attr)
{
	printf("%s", nla_get_string(attr));
}

static bool parse_array(const struct tm_field *field, int idx,
			   struct nl_msg *msg, const char *val)
{
	bool ret = true;
	char *str, *cur, *ap;
	void *a;

	ap = str = strdup(val);

	a = nla_nest_start(msg, idx);

	idx = 0;
	while ((cur = strsep(&ap, ",")) != NULL) {
		ret = field->parse2(field, idx++, msg, cur);
		if (!ret)
			break;
	}

	nla_nest_end(msg, a);

	free(str);

	return ret;
}

static void print_array(const struct tm_field *field, struct nlattr *attr)
{
	struct nlattr *cur;
	int idx = 0;
	int rem;

	nla_for_each_nested(cur, attr, rem) {
		if (idx++ > 0)
			printf(",");
		if (nla_len(cur) != 1)
			continue;
		field->print2(field, cur);
	}
}

static void print_nested(const struct tm_field *field, struct nlattr *attr)
{
	struct nlattr **tb = alloca(field->len * sizeof(struct nlattr *));
	const struct tm_field *fields = field->fields;
	int i;

	nla_parse_nested(tb, field->len - 1, attr, field->policy);
	for (i = 0; i < field->len; i++) {
		int prefix_len = 0;

		if (!tb[i])
			continue;

		if (!fields[i].print)
			continue;

		if (fields[i].name)
			printf("%s%s=", prefix, fields[i].name);

		if (fields[i].prefix) {
			prefix_len = strlen(prefix);
			strncat(prefix + prefix_len, fields[i].prefix,
				sizeof(prefix) - prefix_len - 1);
		}

		fields[i].print(&fields[i], tb[i]);
		if (fields[i].prefix)
			prefix[prefix_len] = 0;

		if (fields[i].name)
			printf("\n");
	}

	if (field->print_extra)
		field->print_extra(field, tb);
}

static void print_extra_stats(const struct tm_field *field, struct nlattr **tb)
{
	float total, failed;

	if (!tb[MT76_TM_STATS_ATTR_RX_PACKETS] ||
	    !tb[MT76_TM_STATS_ATTR_RX_FCS_ERROR])
		return;

	total = nla_get_u64(tb[MT76_TM_STATS_ATTR_RX_PACKETS]);
	failed = nla_get_u64(tb[MT76_TM_STATS_ATTR_RX_FCS_ERROR]);

	printf("%srx_per=%.02f%%\n", prefix, 100 * failed / total);
}

static bool parse_mac(const struct tm_field *field, int idx,
		      struct nl_msg *msg, const char *val)
{
#define ETH_ALEN	6
	bool ret = true;
	char *str, *cur, *ap;
	void *a;

	str = strdup(val);
	ap = str;

	a = nla_nest_start(msg, idx);

	idx = 0;
	while ((cur = strsep(&ap, ",")) != NULL) {
		unsigned char addr[ETH_ALEN];
		char *val, *tmp = cur;
		int i = 0;

		while ((val = strsep(&tmp, ":")) != NULL) {
			if (i >= ETH_ALEN)
				break;

			addr[i++] = strtoul(val, NULL, 16);
		}

		nla_put(msg, idx, ETH_ALEN, addr);

		idx++;
	}

	nla_nest_end(msg, a);

	free(str);

	return ret;
}

static void print_mac(const struct tm_field *field, struct nlattr *attr)
{
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
	unsigned char addr[3][6];
	struct nlattr *cur;
	int idx = 0;
	int rem;

	nla_for_each_nested(cur, attr, rem) {
		if (nla_len(cur) != 6)
			continue;
		memcpy(addr[idx++], nla_data(cur), 6);
	}

	printf("" MACSTR "," MACSTR "," MACSTR "",
	       MAC2STR(addr[0]), MAC2STR(addr[1]), MAC2STR(addr[2]));
}

#define FIELD_GENERIC(_field, _name, ...)	\
	[FIELD_NAME(_field)] = {			\
		.name = _name,				\
		##__VA_ARGS__				\
	}

#define FIELD_RO(_type, _field, _name, ...)		\
	FIELD_GENERIC(_field, _name,	\
		      .print = print_##_type,		\
		      ##__VA_ARGS__			\
	)

#define FIELD(_type, _field, _name, ...)		\
	FIELD_RO(_type, _field, _name,			\
		 .parse = parse_##_type,		\
		 ##__VA_ARGS__				\
	)

#define FIELD_FLAG(_field, _name)			\
	FIELD_GENERIC(_field, _name, .parse = parse_flag)

#define FIELD_ENUM(_field, _name, _vals)		\
	FIELD(enum, _field, _name,			\
	      .enum_str = _vals,			\
	      .enum_len = ARRAY_SIZE(_vals)		\
	)

#define FIELD_ARRAY_RO(_type, _field, _name, ...)	\
	FIELD(array, _field, _name,			\
	      .print2 = print_##_type,			\
	      ##__VA_ARGS__				\
	)

#define FIELD_ARRAY(_type, _field, _name, ...)		\
	FIELD_ARRAY_RO(_type, _field, _name,		\
		       .parse2 = parse_##_type,		\
		       ##__VA_ARGS__			\
	)

#define FIELD_NESTED_RO(_field, _data, _prefix, ...)	\
	FIELD_RO(nested, _field, NULL,			\
		 .prefix = _prefix,			\
		 .fields = _data ## _fields,		\
		 .policy = _data ## _policy,		\
		 .len = ARRAY_SIZE(_data ## _fields),	\
		 ##__VA_ARGS__				\
	)

#define FIELD_MAC(_field, _name)			\
	[FIELD_NAME(_field)] = {			\
		.name = _name,				\
		.parse = parse_mac,			\
		.print = print_mac			\
	}

#define FIELD_NAME(_field) MT76_TM_RX_ATTR_##_field
static const struct tm_field rx_fields[NUM_MT76_TM_RX_ATTRS] = {
	FIELD_RO(s32, FREQ_OFFSET, "freq_offset"),
	FIELD_ARRAY_RO(u8, RCPI, "rcpi"),
	FIELD_ARRAY_RO(s8, RSSI, "rssi"),
	FIELD_ARRAY_RO(s8, IB_RSSI, "ib_rssi"),
	FIELD_ARRAY_RO(s8, WB_RSSI, "wb_rssi"),
	FIELD_RO(s8, SNR, "snr"),
};
static struct nla_policy rx_policy[NUM_MT76_TM_RX_ATTRS] = {
	[MT76_TM_RX_ATTR_FREQ_OFFSET] = { .type = NLA_U32 },
	[MT76_TM_RX_ATTR_RCPI] = { .type = NLA_NESTED },
	[MT76_TM_RX_ATTR_RSSI] = { .type = NLA_NESTED },
	[MT76_TM_RX_ATTR_IB_RSSI] = { .type = NLA_NESTED },
	[MT76_TM_RX_ATTR_WB_RSSI] = { .type = NLA_NESTED },
	[MT76_TM_RX_ATTR_SNR] = { .type = NLA_U8 },
};
#undef FIELD_NAME

#define FIELD_NAME(_field) MT76_TM_STATS_ATTR_##_field
static const struct tm_field stats_fields[NUM_MT76_TM_STATS_ATTRS] = {
	FIELD_RO(u32, TX_PENDING, "tx_pending"),
	FIELD_RO(u32, TX_QUEUED, "tx_queued"),
	FIELD_RO(u32, TX_DONE, "tx_done"),
	FIELD_RO(u64, RX_PACKETS, "rx_packets"),
	FIELD_RO(u64, RX_FCS_ERROR, "rx_fcs_error"),
	FIELD_RO(u64, RX_LEN_MISMATCH, "rx_len_mismatch"),
	FIELD_NESTED_RO(LAST_RX, rx, "last_"),
};
static struct nla_policy stats_policy[NUM_MT76_TM_STATS_ATTRS] = {
	[MT76_TM_STATS_ATTR_TX_PENDING] = { .type = NLA_U32 },
	[MT76_TM_STATS_ATTR_TX_QUEUED] = { .type = NLA_U32 },
	[MT76_TM_STATS_ATTR_TX_DONE] = { .type = NLA_U32 },
	[MT76_TM_STATS_ATTR_RX_PACKETS] = { .type = NLA_U64 },
	[MT76_TM_STATS_ATTR_RX_FCS_ERROR] = { .type = NLA_U64 },
	[MT76_TM_STATS_ATTR_RX_LEN_MISMATCH] = { .type = NLA_U64 },
};
#undef FIELD_NAME

#define FIELD_NAME(_field) MT76_TM_ATTR_##_field
static const struct tm_field testdata_fields[NUM_MT76_TM_ATTRS] = {
	FIELD_FLAG(RESET, "reset"),
	FIELD_ENUM(STATE, "state", testmode_state),
	FIELD_RO(string, MTD_PART, "mtd_part"),
	FIELD_RO(u32, MTD_OFFSET, "mtd_offset"),
	FIELD(u8, SKU_EN, "sku_en"),
	FIELD(u32, TX_COUNT, "tx_count"),
	FIELD(u32, TX_LENGTH, "tx_length"),
	FIELD_ENUM(TX_RATE_MODE, "tx_rate_mode", testmode_tx_mode),
	FIELD(u8, TX_RATE_NSS, "tx_rate_nss"),
	FIELD(u8, TX_RATE_IDX, "tx_rate_idx"),
	FIELD(u8, TX_RATE_SGI, "tx_rate_sgi"),
	FIELD(u8, TX_RATE_LDPC, "tx_rate_ldpc"),
	FIELD(u8, TX_RATE_STBC, "tx_rate_stbc"),
	FIELD(u8, TX_LTF, "tx_ltf"),
	FIELD(u8, TX_DUTY_CYCLE, "tx_duty_cycle"),
	FIELD(u32, TX_IPG, "tx_ipg"),
	FIELD(u32, TX_TIME, "tx_time"),
	FIELD(u8, TX_POWER_CONTROL, "tx_power_control"),
	FIELD_ARRAY(u8, TX_POWER, "tx_power"),
	FIELD(u8, TX_ANTENNA, "tx_antenna"),
	FIELD(u32, FREQ_OFFSET, "freq_offset"),
	FIELD(u8, AID, "aid"),
	FIELD(u8, RU_ALLOC, "ru_alloc"),
	FIELD(u8, RU_IDX, "ru_idx"),
	FIELD_ENUM(TXBF_ACT, "txbf_act", testmode_txbf_act),
	FIELD_ARRAY(u16_hex, TXBF_PARAM, "txbf_param"),
	FIELD(u8, OFF_CH_SCAN_CH, "offchan_ch"),
	FIELD(u8, OFF_CH_SCAN_CENTER_CH, "offchan_center_ch"),
	FIELD_ENUM(OFF_CH_SCAN_BW, "offchan_bw", testmode_offchan_bw),
	FIELD(u8, IPI_THRESHOLD, "ipi_threshold"),
	FIELD(u32, IPI_PERIOD, "ipi_period"),
	FIELD(u8, IPI_RESET, "ipi_reset"),
	FIELD_MAC(MAC_ADDRS, "mac_addrs"),
	FIELD_NESTED_RO(STATS, stats, "",
			.print_extra = print_extra_stats),

};
#undef FIELD_NAME

static struct nla_policy testdata_policy[NUM_MT76_TM_ATTRS] = {
	[MT76_TM_ATTR_STATE] = { .type = NLA_U8 },
	[MT76_TM_ATTR_MTD_PART] = { .type = NLA_STRING },
	[MT76_TM_ATTR_MTD_OFFSET] = { .type = NLA_U32 },
	[MT76_TM_ATTR_SKU_EN] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_COUNT] = { .type = NLA_U32 },
	[MT76_TM_ATTR_TX_LENGTH] = { .type = NLA_U32 },
	[MT76_TM_ATTR_TX_RATE_MODE] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_RATE_NSS] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_RATE_IDX] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_RATE_SGI] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_RATE_LDPC] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_RATE_STBC] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_LTF] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_DUTY_CYCLE] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_IPG] = { .type = NLA_U32 },
	[MT76_TM_ATTR_TX_TIME] = { .type = NLA_U32 },
	[MT76_TM_ATTR_TX_POWER_CONTROL] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_ANTENNA] = { .type = NLA_U8 },
	[MT76_TM_ATTR_TX_SPE_IDX] = { .type = NLA_U8 },
	[MT76_TM_ATTR_FREQ_OFFSET] = { .type = NLA_U32 },
	[MT76_TM_ATTR_AID] = { .type = NLA_U8 },
	[MT76_TM_ATTR_RU_ALLOC] = { .type = NLA_U8 },
	[MT76_TM_ATTR_RU_IDX] = { .type = NLA_U8 },
	[MT76_TM_ATTR_STATS] = { .type = NLA_NESTED },
	[MT76_TM_ATTR_TXBF_ACT] = { .type = NLA_U8 },
	[MT76_TM_ATTR_OFF_CH_SCAN_CH] = { .type = NLA_U8 },
	[MT76_TM_ATTR_OFF_CH_SCAN_CENTER_CH] = { .type = NLA_U8 },
	[MT76_TM_ATTR_OFF_CH_SCAN_BW] = { .type = NLA_U8 },
	[MT76_TM_ATTR_IPI_THRESHOLD] = { .type = NLA_U8 },
	[MT76_TM_ATTR_IPI_PERIOD] = { .type = NLA_U32 },
	[MT76_TM_ATTR_IPI_ANTENNA_INDEX] = { .type = NLA_U8 },
	[MT76_TM_ATTR_IPI_RESET] = { .type = NLA_U8 },
};

const struct tm_field msg_field = {
	.print = print_nested,
	.fields = testdata_fields,
	.policy = testdata_policy,
	.len = ARRAY_SIZE(testdata_fields)
};
