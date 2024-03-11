// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (C) 2020 Felix Fietkau <nbd@nbd.name> */

#ifndef __MTK_PPE_H
#define __MTK_PPE_H

#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <linux/rhashtable.h>

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MTK_MAX_PPE_NUM			3
#define MTK_ETH_PPE_BASE		0x2000
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
#define MTK_MAX_PPE_NUM			2
#define MTK_ETH_PPE_BASE		0x2000
#else
#define MTK_MAX_PPE_NUM			1
#define MTK_ETH_PPE_BASE		0xc00
#endif

#define MTK_PPE_ENTRIES_SHIFT		3
#define MTK_PPE_ENTRIES			(1024 << MTK_PPE_ENTRIES_SHIFT)
#define MTK_PPE_HASH_MASK		(MTK_PPE_ENTRIES - 1)
#define MTK_PPE_WAIT_TIMEOUT_US		1000000

#define MTK_FOE_IB1_UNBIND_TIMESTAMP	GENMASK(7, 0)
#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MTK_FOE_IB1_UNBIND_SRC_PORT	GENMASK(11, 8)
#define MTK_FOE_IB1_UNBIND_PACKETS	GENMASK(19, 12)
#define MTK_FOE_IB1_UNBIND_PREBIND	BIT(22)
#define MTK_FOE_IB1_UNBIND_PACKET_TYPE	GENMASK(27, 23)
#define MTK_FOE_IB1_BIND_TIMESTAMP	GENMASK(7, 0)
#define MTK_FOE_IB1_BIND_SRC_PORT	GENMASK(11, 8)
#define MTK_FOE_IB1_BIND_MC		BIT(12)
#define MTK_FOE_IB1_BIND_KEEPALIVE	BIT(13)
#define MTK_FOE_IB1_BIND_VLAN_LAYER	GENMASK(16, 14)
#define MTK_FOE_IB1_BIND_PPPOE		BIT(17)
#define MTK_FOE_IB1_BIND_VLAN_TAG	BIT(18)
#define MTK_FOE_IB1_BIND_PKT_SAMPLE	BIT(19)
#define MTK_FOE_IB1_BIND_CACHE		BIT(20)
#define MTK_FOE_IB1_BIND_TUNNEL_DECAP	BIT(21)
#define MTK_FOE_IB1_BIND_TTL		BIT(22)
#define MTK_FOE_IB1_PACKET_TYPE		GENMASK(27, 23)
#else
#define MTK_FOE_IB1_UNBIND_PACKETS	GENMASK(23, 8)
#define MTK_FOE_IB1_UNBIND_PREBIND	BIT(24)

#define MTK_FOE_IB1_BIND_TIMESTAMP	GENMASK(14, 0)
#define MTK_FOE_IB1_BIND_KEEPALIVE	BIT(15)
#define MTK_FOE_IB1_BIND_VLAN_LAYER	GENMASK(18, 16)
#define MTK_FOE_IB1_BIND_PPPOE		BIT(19)
#define MTK_FOE_IB1_BIND_VLAN_TAG	BIT(20)
#define MTK_FOE_IB1_BIND_PKT_SAMPLE	BIT(21)
#define MTK_FOE_IB1_BIND_CACHE		BIT(22)
#define MTK_FOE_IB1_BIND_TUNNEL_DECAP	BIT(23)
#define MTK_FOE_IB1_BIND_TTL		BIT(24)

#define MTK_FOE_IB1_PACKET_TYPE		GENMASK(27, 25)
#endif

#define MTK_FOE_IB1_STATE		GENMASK(29, 28)
#define MTK_FOE_IB1_UDP			BIT(30)
#define MTK_FOE_IB1_STATIC		BIT(31)

enum {
	MTK_PPE_PKT_TYPE_IPV4_HNAPT = 0,
	MTK_PPE_PKT_TYPE_IPV4_ROUTE = 1,
	MTK_PPE_PKT_TYPE_BRIDGE = 2,
	MTK_PPE_PKT_TYPE_IPV4_DSLITE = 3,
	MTK_PPE_PKT_TYPE_IPV6_ROUTE_3T = 4,
	MTK_PPE_PKT_TYPE_IPV6_ROUTE_5T = 5,
	MTK_PPE_PKT_TYPE_IPV6_6RD = 7,
};

#if defined(CONFIG_MEDIATEK_NETSYS_V2) || defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MTK_FOE_IB2_QID			GENMASK(6, 0)
#define MTK_FOE_IB2_PORT_MG		BIT(7)
#define MTK_FOE_IB2_PSE_QOS		BIT(8)
#define MTK_FOE_IB2_DEST_PORT		GENMASK(12, 9)
#define MTK_FOE_IB2_MULTICAST		BIT(13)
#define MTK_FOE_IB2_MIB_CNT		BIT(15)
#define MTK_FOE_IB2_RX_IDX		GENMASK(18, 17)
#define MTK_FOE_IB2_WDMA_WINFO		BIT(19)
#define MTK_FOE_IB2_PORT_AG		GENMASK(23, 20)
#else
#define MTK_FOE_IB2_QID			GENMASK(3, 0)
#define MTK_FOE_IB2_PSE_QOS		BIT(4)
#define MTK_FOE_IB2_DEST_PORT		GENMASK(7, 5)
#define MTK_FOE_IB2_MULTICAST		BIT(8)

#define MTK_FOE_IB2_WDMA_QID2		GENMASK(13, 12)
#define MTK_FOE_IB2_MIB_CNT		BIT(15)
#define MTK_FOE_IB2_WDMA_DEVIDX		BIT(16)
#define MTK_FOE_IB2_WDMA_WINFO		BIT(17)

#define MTK_FOE_IB2_PORT_MG		GENMASK(17, 12)

#define MTK_FOE_IB2_PORT_AG		GENMASK(23, 18)
#endif

#define MTK_FOE_IB2_DSCP		GENMASK(31, 24)

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
#define MTK_FOE_WINFO_WCID		GENMASK(15, 0)
#define MTK_FOE_WINFO_BSS		GENMASK(23, 16)

#define MTK_FOE_WINFO_PAO_USR_INFO	GENMASK(15, 0)
#define MTK_FOE_WINFO_PAO_TID		GENMASK(19, 16)
#define MTK_FOE_WINFO_PAO_IS_FIXEDRATE	BIT(20)
#define MTK_FOE_WINFO_PAO_IS_PRIOR	BIT(21)
#define MTK_FOE_WINFO_PAO_IS_SP		BIT(22)
#define MTK_FOE_WINFO_PAO_HF		BIT(23)
#define MTK_FOE_WINFO_PAO_AMSDU_EN	BIT(24)
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
#define MTK_FOE_WINFO_BSS		GENMASK(5, 0)
#define MTK_FOE_WINFO_WCID		GENMASK(15, 6)
#else
#define MTK_FOE_VLAN2_WINFO_BSS		GENMASK(5, 0)
#define MTK_FOE_VLAN2_WINFO_WCID	GENMASK(13, 6)
#define MTK_FOE_VLAN2_WINFO_RING	GENMASK(15, 14)
#endif

enum {
	MTK_FOE_STATE_INVALID,
	MTK_FOE_STATE_UNBIND,
	MTK_FOE_STATE_BIND,
	MTK_FOE_STATE_FIN
};

struct mtk_foe_mac_info {
	u16 vlan1;
	u16 etype;

	u32 dest_mac_hi;

	u16 vlan2;
	u16 dest_mac_lo;

	u32 src_mac_hi;

	u16 pppoe_id;
	u16 src_mac_lo;

#if defined(CONFIG_MEDIATEK_NETSYS_V3)
	u16 minfo;
	u16 resv1;
	u32 winfo;
	u32 winfo_pao;
	u16 cdrt_id:8;
	u16 tops_entry:6;
	u16 resv3:2;
	u16 tport_id:4;
	u16 resv4:12;
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
	u16 minfo;
	u16 winfo;
#endif
};

/* software-only entry type */
struct mtk_foe_bridge {
	u8 dest_mac[ETH_ALEN];
	u8 src_mac[ETH_ALEN];
	u16 vlan;

	struct {} key_end;

	u32 ib2;

	struct mtk_foe_mac_info l2;
};

struct mtk_ipv4_tuple {
	u32 src_ip;
	u32 dest_ip;
	union {
		struct {
			u16 dest_port;
			u16 src_port;
		};
		struct {
			u8 protocol;
			u8 _pad[3]; /* fill with 0xa5a5a5 */
		};
		u32 ports;
	};
};

struct mtk_foe_ipv4 {
	struct mtk_ipv4_tuple orig;

	u32 ib2;

	struct mtk_ipv4_tuple new;

	u16 timestamp;
	u16 _rsv0[3];

	u32 udf_tsid;

	struct mtk_foe_mac_info l2;
};

struct mtk_foe_ipv4_dslite {
	struct mtk_ipv4_tuple ip4;

	u32 tunnel_src_ip[4];
	u32 tunnel_dest_ip[4];

	u8 flow_label[3];
	u8 priority;

	u32 udf_tsid;

	u32 ib2;

	struct mtk_foe_mac_info l2;
};

struct mtk_foe_ipv6 {
	u32 src_ip[4];
	u32 dest_ip[4];

	union {
		struct {
			u8 protocol;
			u8 _pad[3]; /* fill with 0xa5a5a5 */
		}; /* 3-tuple */
		struct {
			u16 dest_port;
			u16 src_port;
		}; /* 5-tuple */
		u32 ports;
	};

	u32 _rsv[3];

	u32 udf;

	u32 ib2;
	struct mtk_foe_mac_info l2;
};

struct mtk_foe_ipv6_6rd {
	u32 src_ip[4];
	u32 dest_ip[4];
	u16 dest_port;
	u16 src_port;

	u32 tunnel_src_ip;
	u32 tunnel_dest_ip;

	u16 hdr_csum;
	u8 dscp;
	u8 ttl;

	u8 flag;
	u8 pad;
	u8 per_flow_6rd_id;
	u8 pad2;

	u32 ib2;
	struct mtk_foe_mac_info l2;
};

struct mtk_foe_entry {
	u32 ib1;

	union {
		struct mtk_foe_bridge bridge;
		struct mtk_foe_ipv4 ipv4;
		struct mtk_foe_ipv4_dslite dslite;
		struct mtk_foe_ipv6 ipv6;
		struct mtk_foe_ipv6_6rd ipv6_6rd;
#if defined(CONFIG_MEDIATEK_NETSYS_V3)
		u32 data[31];
#elif defined(CONFIG_MEDIATEK_NETSYS_V2)
		u32 data[23];
#else
		u32 data[19];
#endif
	};
};

enum {
	MTK_PPE_CPU_REASON_TTL_EXCEEDED			= 0x02,
	MTK_PPE_CPU_REASON_OPTION_HEADER		= 0x03,
	MTK_PPE_CPU_REASON_NO_FLOW			= 0x07,
	MTK_PPE_CPU_REASON_IPV4_FRAG			= 0x08,
	MTK_PPE_CPU_REASON_IPV4_DSLITE_FRAG		= 0x09,
	MTK_PPE_CPU_REASON_IPV4_DSLITE_NO_TCP_UDP	= 0x0a,
	MTK_PPE_CPU_REASON_IPV6_6RD_NO_TCP_UDP		= 0x0b,
	MTK_PPE_CPU_REASON_TCP_FIN_SYN_RST		= 0x0c,
	MTK_PPE_CPU_REASON_UN_HIT			= 0x0d,
	MTK_PPE_CPU_REASON_HIT_UNBIND			= 0x0e,
	MTK_PPE_CPU_REASON_HIT_UNBIND_RATE_REACHED	= 0x0f,
	MTK_PPE_CPU_REASON_HIT_BIND_TCP_FIN		= 0x10,
	MTK_PPE_CPU_REASON_HIT_TTL_1			= 0x11,
	MTK_PPE_CPU_REASON_HIT_BIND_VLAN_VIOLATION	= 0x12,
	MTK_PPE_CPU_REASON_KEEPALIVE_UC_OLD_HDR		= 0x13,
	MTK_PPE_CPU_REASON_KEEPALIVE_MC_NEW_HDR		= 0x14,
	MTK_PPE_CPU_REASON_KEEPALIVE_DUP_OLD_HDR	= 0x15,
	MTK_PPE_CPU_REASON_HIT_BIND_FORCE_CPU		= 0x16,
	MTK_PPE_CPU_REASON_TUNNEL_OPTION_HEADER		= 0x17,
	MTK_PPE_CPU_REASON_MULTICAST_TO_CPU		= 0x18,
	MTK_PPE_CPU_REASON_MULTICAST_TO_GMAC1_CPU	= 0x19,
	MTK_PPE_CPU_REASON_HIT_PRE_BIND			= 0x1a,
	MTK_PPE_CPU_REASON_PACKET_SAMPLING		= 0x1b,
	MTK_PPE_CPU_REASON_EXCEED_MTU			= 0x1c,
	MTK_PPE_CPU_REASON_PPE_BYPASS			= 0x1e,
	MTK_PPE_CPU_REASON_INVALID			= 0x1f,
};

enum {
	MTK_FLOW_TYPE_L4,
	MTK_FLOW_TYPE_L2,
	MTK_FLOW_TYPE_L2_SUBFLOW,
};

struct mtk_flow_entry {
	union {
		struct hlist_node list;
		struct {
			struct rhash_head l2_node;
			struct hlist_head l2_flows;
		};
	};
	u8 type;
	s8 ppe_index;
	s8 wed_index;
	u16 hash;
	union {
		struct mtk_foe_entry data;
		struct {
			struct mtk_flow_entry *base_flow;
			struct hlist_node list;
			struct {} end;
		} l2_data;
	};
	struct rhash_head node;
	unsigned long cookie;
};

struct mtk_mib_entry {
	u32	byt_cnt_l;
	u16	byt_cnt_h;
	u32	pkt_cnt_l;
	u8	pkt_cnt_h;
	u8	_rsv0;
	u32	_rsv1;
} __packed;

struct mtk_foe_accounting {
	u64	bytes;
	u64	packets;
};

struct mtk_ppe {
	struct mtk_eth *eth;
	struct device *dev;
	void __iomem *base;
	int version;
	int id;
	int way;
	int accounting;

	struct mtk_foe_entry *foe_table;
	dma_addr_t foe_phys;

	struct mtk_mib_entry *mib_table;
	dma_addr_t mib_phys;

	u16 foe_check_time[MTK_PPE_ENTRIES];
	struct hlist_head *foe_flow;

	struct rhashtable l2_flows;

	void *acct_table;
	void *acct_updated_table;
};

struct mtk_ppe *mtk_ppe_init(struct mtk_eth *eth, void __iomem *base, int version, int way, int id,
			     int accounting);
int mtk_ppe_start(struct mtk_ppe *ppe);
int mtk_ppe_stop(struct mtk_ppe *ppe);

void __mtk_ppe_check_skb(struct mtk_ppe *ppe, struct sk_buff *skb, u16 hash);

static inline void
mtk_ppe_check_skb(struct mtk_ppe *ppe, struct sk_buff *skb, u16 hash)
{
	u16 now, diff;

	if (!ppe)
		return;

	now = (u16)jiffies;
	diff = now - ppe->foe_check_time[hash];
	if (diff < HZ / 10)
		return;

	ppe->foe_check_time[hash] = now;
	__mtk_ppe_check_skb(ppe, skb, hash);
}

static inline int
mtk_foe_entry_timestamp(struct mtk_ppe *ppe, u16 hash)
{
	u32 ib1 = READ_ONCE(ppe->foe_table[hash].ib1);

	if (FIELD_GET(MTK_FOE_IB1_STATE, ib1) != MTK_FOE_STATE_BIND)
		return -1;

	return FIELD_GET(MTK_FOE_IB1_BIND_TIMESTAMP, ib1);
}

int mtk_foe_entry_prepare(struct mtk_foe_entry *entry, int type, int l4proto,
			  u8 pse_port, u8 *src_mac, u8 *dest_mac);
int mtk_foe_entry_set_pse_port(struct mtk_foe_entry *entry, u8 port);
int mtk_foe_entry_set_ipv4_tuple(struct mtk_foe_entry *entry, bool orig,
				 __be32 src_addr, __be16 src_port,
				 __be32 dest_addr, __be16 dest_port);
int mtk_foe_entry_set_ipv6_tuple(struct mtk_foe_entry *entry,
				 __be32 *src_addr, __be16 src_port,
				 __be32 *dest_addr, __be16 dest_port);
int mtk_foe_entry_set_dsa(struct mtk_foe_entry *entry, int port);
int mtk_foe_entry_set_vlan(struct mtk_foe_entry *entry, int vid);
int mtk_foe_entry_set_pppoe(struct mtk_foe_entry *entry, int sid);
int mtk_foe_entry_set_wdma(struct mtk_foe_entry *entry, int wdma_idx, int txq,
			   int bss, int wcid, bool amsdu_en);
int mtk_foe_entry_set_qid(struct mtk_foe_entry *entry, int qid);
int mtk_foe_entry_set_dscp(struct mtk_foe_entry *entry, int dscp);
bool mtk_foe_entry_match(struct mtk_foe_entry *entry, struct mtk_foe_entry *data);
int mtk_foe_entry_set_sp(struct mtk_ppe *ppe, struct mtk_foe_entry *entry);
int mtk_foe_entry_commit(struct mtk_ppe *ppe, struct mtk_flow_entry *entry);
void mtk_foe_entry_clear(struct mtk_ppe *ppe, struct mtk_flow_entry *entry);
int mtk_foe_entry_idle_time(struct mtk_ppe *ppe, struct mtk_flow_entry *entry);
struct mtk_foe_accounting *mtk_foe_entry_get_mib(struct mtk_ppe *ppe, u32 index, struct mtk_foe_accounting *diff);
u32 mtk_ppe_hash_entry(struct mtk_ppe *ppe, struct mtk_foe_entry *e);

#endif
