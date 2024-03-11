/*
 * Copyright (C) 2018-2021 Felix Fietkau <nbd@nbd.name>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netfilter.h>
#include <linux/netfilter/xt_FLOWOFFLOAD.h>
#include <linux/if_vlan.h>
#include <net/ip.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_extend.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_flow_table.h>

struct xt_flowoffload_hook {
	struct hlist_node list;
	struct nf_hook_ops ops;
	struct net *net;
	bool registered;
	bool used;
};

struct xt_flowoffload_table {
	struct nf_flowtable ft;
	struct hlist_head hooks;
	struct delayed_work work;
};

struct nf_forward_info {
	const struct net_device *indev;
	const struct net_device *outdev;
	const struct net_device *hw_outdev;
	struct id {
		__u16	id;
		__be16	proto;
	} encap[NF_FLOW_TABLE_ENCAP_MAX];
	u8 num_encaps;
	u8 ingress_vlans;
	u8 h_source[ETH_ALEN];
	u8 h_dest[ETH_ALEN];
	enum flow_offload_xmit_type xmit_type;
};

static DEFINE_SPINLOCK(hooks_lock);

struct xt_flowoffload_table flowtable[2];

static int
xt_flowoffload_dscp_init(struct sk_buff *skb, struct flow_offload *flow,
			 enum ip_conntrack_dir dir)
{
	const struct flow_offload_tuple *flow_tuple = &flow->tuplehash[dir].tuple;
	struct iphdr *iph;
	struct ipv6hdr *ip6h;
	u32 offset = 0;
	u8 tos = 0;

	switch (flow_tuple->l3proto) {
	case NFPROTO_IPV4:
		iph = (struct iphdr *)(skb_network_header(skb) + offset);
		tos = iph->tos;
		break;
	case NFPROTO_IPV6:
		ip6h = (struct ipv6hdr *)(skb_network_header(skb) + offset);
		tos = ipv6_get_dsfield(ip6h);
		break;
	default:
		return -1;
	};

	flow->tuplehash[dir].tuple.tos = tos;
	flow->tuplehash[!dir].tuple.tos = tos;

	return 0;
}

static unsigned int
xt_flowoffload_net_hook(void *priv, struct sk_buff *skb,
			const struct nf_hook_state *state)
{
	struct vlan_ethhdr *veth;
	__be16 proto;

	switch (skb->protocol) {
	case htons(ETH_P_8021Q):
		veth = (struct vlan_ethhdr *)skb_mac_header(skb);
		proto = veth->h_vlan_encapsulated_proto;
		break;
	case htons(ETH_P_PPP_SES):
		proto = nf_flow_pppoe_proto(skb);
		break;
	default:
		proto = skb->protocol;
		break;
	}

	switch (proto) {
	case htons(ETH_P_IP):
		return nf_flow_offload_ip_hook(priv, skb, state);
	case htons(ETH_P_IPV6):
		return nf_flow_offload_ipv6_hook(priv, skb, state);
	}

	return NF_ACCEPT;
}

static int
xt_flowoffload_create_hook(struct xt_flowoffload_table *table,
			   struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;
	struct nf_hook_ops *ops;

	hook = kzalloc(sizeof(*hook), GFP_ATOMIC);
	if (!hook)
		return -ENOMEM;

	ops = &hook->ops;
	ops->pf = NFPROTO_NETDEV;
	ops->hooknum = NF_NETDEV_INGRESS;
	ops->priority = 10;
	ops->priv = &table->ft;
	ops->hook = xt_flowoffload_net_hook;
	ops->dev = dev;

	hlist_add_head(&hook->list, &table->hooks);
	mod_delayed_work(system_power_efficient_wq, &table->work, 0);

	return 0;
}

static struct xt_flowoffload_hook *
flow_offload_lookup_hook(struct xt_flowoffload_table *table,
			 struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;

	hlist_for_each_entry(hook, &table->hooks, list) {
		if (hook->ops.dev == dev)
			return hook;
	}

	return NULL;
}

static void
xt_flowoffload_check_device(struct xt_flowoffload_table *table,
			    struct net_device *dev)
{
	struct xt_flowoffload_hook *hook;

	if (!dev)
		return;

	spin_lock_bh(&hooks_lock);
	hook = flow_offload_lookup_hook(table, dev);
	if (hook)
		hook->used = true;
	else
		xt_flowoffload_create_hook(table, dev);
	spin_unlock_bh(&hooks_lock);
}

static void
xt_flowoffload_register_hooks(struct xt_flowoffload_table *table)
{
	struct xt_flowoffload_hook *hook;

restart:
	hlist_for_each_entry(hook, &table->hooks, list) {
		if (hook->registered)
			continue;

		hook->registered = true;
		hook->net = dev_net(hook->ops.dev);
		spin_unlock_bh(&hooks_lock);
		nf_register_net_hook(hook->net, &hook->ops);
		if (table->ft.flags & NF_FLOWTABLE_HW_OFFLOAD)
			table->ft.type->setup(&table->ft, hook->ops.dev,
					      FLOW_BLOCK_BIND);
		spin_lock_bh(&hooks_lock);
		goto restart;
	}

}

static bool
xt_flowoffload_cleanup_hooks(struct xt_flowoffload_table *table)
{
	struct xt_flowoffload_hook *hook;
	bool active = false;

restart:
	spin_lock_bh(&hooks_lock);
	hlist_for_each_entry(hook, &table->hooks, list) {
		if (hook->used || !hook->registered) {
			active = true;
			continue;
		}

		hlist_del(&hook->list);
		spin_unlock_bh(&hooks_lock);
		if (table->ft.flags & NF_FLOWTABLE_HW_OFFLOAD)
			table->ft.type->setup(&table->ft, hook->ops.dev,
					      FLOW_BLOCK_UNBIND);
		nf_unregister_net_hook(hook->net, &hook->ops);
		kfree(hook);
		goto restart;
	}
	spin_unlock_bh(&hooks_lock);

	return active;
}

static void
xt_flowoffload_check_hook(struct flow_offload *flow, void *data)
{
	struct xt_flowoffload_table *table = data;
	struct flow_offload_tuple *tuple0 = &flow->tuplehash[0].tuple;
	struct flow_offload_tuple *tuple1 = &flow->tuplehash[1].tuple;
	struct xt_flowoffload_hook *hook;

	spin_lock_bh(&hooks_lock);
	hlist_for_each_entry(hook, &table->hooks, list) {
		if (hook->ops.dev->ifindex != tuple0->iifidx &&
		    hook->ops.dev->ifindex != tuple1->iifidx)
			continue;

		hook->used = true;
	}
	spin_unlock_bh(&hooks_lock);
}

static void
xt_flowoffload_hook_work(struct work_struct *work)
{
	struct xt_flowoffload_table *table;
	struct xt_flowoffload_hook *hook;
	int err;

	table = container_of(work, struct xt_flowoffload_table, work.work);

	spin_lock_bh(&hooks_lock);
	xt_flowoffload_register_hooks(table);
	hlist_for_each_entry(hook, &table->hooks, list)
		hook->used = false;
	spin_unlock_bh(&hooks_lock);

	err = nf_flow_table_iterate(&table->ft, xt_flowoffload_check_hook,
				    table);
	if (err && err != -EAGAIN)
		goto out;

	if (!xt_flowoffload_cleanup_hooks(table))
		return;

out:
	queue_delayed_work(system_power_efficient_wq, &table->work, HZ);
}

static bool
xt_flowoffload_skip(struct sk_buff *skb, int family)
{
	if (skb_sec_path(skb))
		return true;

	if (family == NFPROTO_IPV4) {
		const struct ip_options *opt = &(IPCB(skb)->opt);

		if (unlikely(opt->optlen))
			return true;
	}

	return false;
}

static enum flow_offload_xmit_type nf_xmit_type(struct dst_entry *dst)
{
	if (dst_xfrm(dst))
		return FLOW_OFFLOAD_XMIT_XFRM;

	return FLOW_OFFLOAD_XMIT_NEIGH;
}

static void nf_default_forward_path(struct nf_flow_route *route,
				    struct dst_entry *dst_cache,
				    enum ip_conntrack_dir dir,
				    struct net_device **dev)
{
	route->tuple[!dir].in.ifindex	= dst_cache->dev->ifindex;
	route->tuple[dir].dst		= dst_cache;
	route->tuple[dir].xmit_type	= nf_xmit_type(dst_cache);
}

static bool nf_is_valid_ether_device(const struct net_device *dev)
{
	if (!dev || (dev->flags & IFF_LOOPBACK) || dev->type != ARPHRD_ETHER ||
	    dev->addr_len != ETH_ALEN || !is_valid_ether_addr(dev->dev_addr))
		return false;

	return true;
}

static void nf_dev_path_info(const struct net_device_path_stack *stack,
			     struct nf_forward_info *info,
			     unsigned char *ha)
{
	const struct net_device_path *path;
	int i;

	memcpy(info->h_dest, ha, ETH_ALEN);

	for (i = 0; i < stack->num_paths; i++) {
		path = &stack->path[i];

		info->indev = path->dev;

		switch (path->type) {
		case DEV_PATH_ETHERNET:
		case DEV_PATH_DSA:
		case DEV_PATH_VLAN:
		case DEV_PATH_PPPOE:
			if (is_zero_ether_addr(info->h_source))
				memcpy(info->h_source, path->dev->dev_addr, ETH_ALEN);

			if (path->type == DEV_PATH_ETHERNET)
				break;
			if (path->type == DEV_PATH_DSA) {
				i = stack->num_paths;
				break;
			}

			/* DEV_PATH_VLAN and DEV_PATH_PPPOE */
			if (info->num_encaps >= NF_FLOW_TABLE_ENCAP_MAX) {
				info->indev = NULL;
				break;
			}
			if (!info->outdev)
				info->outdev = path->dev;
			info->encap[info->num_encaps].id = path->encap.id;
			info->encap[info->num_encaps].proto = path->encap.proto;
			info->num_encaps++;
			if (path->type == DEV_PATH_PPPOE)
				memcpy(info->h_dest, path->encap.h_dest, ETH_ALEN);
			break;
		case DEV_PATH_BRIDGE:
			if (is_zero_ether_addr(info->h_source))
				memcpy(info->h_source, path->dev->dev_addr, ETH_ALEN);

			switch (path->bridge.vlan_mode) {
			case DEV_PATH_BR_VLAN_UNTAG_HW:
				info->ingress_vlans |= BIT(info->num_encaps - 1);
				break;
			case DEV_PATH_BR_VLAN_TAG:
				info->encap[info->num_encaps].id = path->bridge.vlan_id;
				info->encap[info->num_encaps].proto = path->bridge.vlan_proto;
				info->num_encaps++;
				break;
			case DEV_PATH_BR_VLAN_UNTAG:
				info->num_encaps--;
				break;
			case DEV_PATH_BR_VLAN_KEEP:
				break;
			}
			break;
		default:
			break;
		}
	}
	if (!info->outdev)
		info->outdev = info->indev;

	info->hw_outdev = info->indev;

	if (nf_is_valid_ether_device(info->indev))
		info->xmit_type = FLOW_OFFLOAD_XMIT_DIRECT;
}

static int nf_dev_fill_forward_path(const struct nf_flow_route *route,
				     const struct dst_entry *dst_cache,
				     const struct nf_conn *ct,
				     enum ip_conntrack_dir dir, u8 *ha,
				     struct net_device_path_stack *stack)
{
	const void *daddr = &ct->tuplehash[!dir].tuple.src.u3;
	struct net_device *dev = dst_cache->dev;
	struct neighbour *n;
	u8 nud_state;

	if (!nf_is_valid_ether_device(dev))
		goto out;

	if (ct->status & IPS_NAT_MASK || ct->inet6_mode == CT_INET_MODE_IPV6) {
		n = dst_neigh_lookup(dst_cache, daddr);
		if (!n)
			return -1;

		read_lock_bh(&n->lock);
		nud_state = n->nud_state;
		ether_addr_copy(ha, n->ha);
		read_unlock_bh(&n->lock);
		neigh_release(n);

		if (!(nud_state & NUD_VALID))
			return -1;
	}

out:
	return dev_fill_forward_path(dev, ha, stack);
}

static int nf_dev_forward_path(struct sk_buff *skb,
				struct nf_flow_route *route,
				const struct nf_conn *ct,
				enum ip_conntrack_dir dir,
				struct net_device **devs)
{
	const struct dst_entry *dst = route->tuple[dir].dst;
	struct ethhdr *eth;
	enum ip_conntrack_dir skb_dir;
	struct net_device_path_stack stack;
	struct nf_forward_info info = {};
	unsigned char ha[ETH_ALEN];
	int i;

	if (!(ct->status & IPS_NAT_MASK) && skb_mac_header_was_set(skb) &&
	    ct->inet6_mode != CT_INET_MODE_IPV6) {
		eth = eth_hdr(skb);
		skb_dir = CTINFO2DIR(skb_get_nfct(skb) & NFCT_INFOMASK);

		if (skb_dir != dir) {
			memcpy(ha, eth->h_source, ETH_ALEN);
			memcpy(info.h_source, eth->h_dest, ETH_ALEN);
		} else {
			memcpy(ha, eth->h_dest, ETH_ALEN);
			memcpy(info.h_source, eth->h_source, ETH_ALEN);
		}
	}

	if (nf_dev_fill_forward_path(route, dst, ct, dir, ha, &stack) >= 0)
		nf_dev_path_info(&stack, &info, ha);

	devs[!dir] = (struct net_device *)info.indev;
	if (!info.indev)
		return -1;

	route->tuple[!dir].in.ifindex = info.indev->ifindex;
	for (i = 0; i < info.num_encaps; i++) {
		route->tuple[!dir].in.encap[i].id = info.encap[i].id;
		route->tuple[!dir].in.encap[i].proto = info.encap[i].proto;
	}
	route->tuple[!dir].in.num_encaps = info.num_encaps;
	route->tuple[!dir].in.ingress_vlans = info.ingress_vlans;

	if (info.xmit_type == FLOW_OFFLOAD_XMIT_DIRECT) {
		memcpy(route->tuple[dir].out.h_source, info.h_source, ETH_ALEN);
		memcpy(route->tuple[dir].out.h_dest, info.h_dest, ETH_ALEN);
		route->tuple[dir].out.ifindex = info.outdev->ifindex;
		route->tuple[dir].out.hw_ifindex = info.hw_outdev->ifindex;
		route->tuple[dir].xmit_type = info.xmit_type;
	}

	return 0;
}

static int
xt_flowoffload_route_dir(struct nf_flow_route *route, const struct nf_conn *ct,
			 enum ip_conntrack_dir dir,
			 const struct xt_action_param *par, int ifindex,
			 struct net_device **devs)
{
	struct dst_entry *dst = NULL;
	struct flowi fl;

	memset(&fl, 0, sizeof(fl));
	switch (xt_family(par)) {
	case NFPROTO_IPV4:
		fl.u.ip4.daddr = ct->tuplehash[!dir].tuple.src.u3.ip;
		fl.u.ip4.flowi4_oif = ifindex;
		break;
	case NFPROTO_IPV6:
		fl.u.ip6.saddr = ct->tuplehash[!dir].tuple.dst.u3.in6;
		fl.u.ip6.daddr = ct->tuplehash[!dir].tuple.src.u3.in6;
		fl.u.ip6.flowi6_oif = ifindex;
		break;
	}

	nf_route(xt_net(par), &dst, &fl, false, xt_family(par));
	if (!dst)
		return -ENOENT;

	nf_default_forward_path(route, dst, dir, devs);

	return 0;
}

static int
xt_flowoffload_route_nat(struct sk_buff *skb, const struct nf_conn *ct,
			 const struct xt_action_param *par,
			 struct nf_flow_route *route, enum ip_conntrack_dir dir,
			 struct net_device **devs)
{
	struct dst_entry *this_dst = skb_dst(skb);
	struct dst_entry *other_dst = NULL;
	struct flowi fl;

	memset(&fl, 0, sizeof(fl));
	switch (xt_family(par)) {
	case NFPROTO_IPV4:
		fl.u.ip4.daddr = ct->tuplehash[dir].tuple.src.u3.ip;
		fl.u.ip4.flowi4_oif = xt_in(par)->ifindex;
		break;
	case NFPROTO_IPV6:
		fl.u.ip6.saddr = ct->tuplehash[!dir].tuple.dst.u3.in6;
		fl.u.ip6.daddr = ct->tuplehash[dir].tuple.src.u3.in6;
		fl.u.ip6.flowi6_oif = xt_in(par)->ifindex;
		break;
	}

	nf_route(xt_net(par), &other_dst, &fl, false, xt_family(par));
	if (!other_dst)
		return -ENOENT;

	nf_default_forward_path(route, this_dst, dir, devs);
	nf_default_forward_path(route, other_dst, !dir, devs);

	if (route->tuple[dir].xmit_type	== FLOW_OFFLOAD_XMIT_NEIGH &&
	    route->tuple[!dir].xmit_type == FLOW_OFFLOAD_XMIT_NEIGH) {
		if (nf_dev_forward_path(skb, route, ct, dir, devs))
			return -1;
		if (nf_dev_forward_path(skb, route, ct, !dir, devs))
			return -1;
	}

	return 0;
}

static int
xt_flowoffload_route_bridge(struct sk_buff *skb, const struct nf_conn *ct,
			    const struct xt_action_param *par,
			    struct nf_flow_route *route, enum ip_conntrack_dir dir,
			    struct net_device **devs)
{
	int ret;

	ret = xt_flowoffload_route_dir(route, ct, dir, par,
				       devs[dir]->ifindex,
				       devs);
	if (ret)
		return ret;

	ret = xt_flowoffload_route_dir(route, ct, !dir, par,
				       devs[!dir]->ifindex,
				       devs);
	if (ret)
		goto err_route_dir1;

	if (route->tuple[dir].xmit_type	== FLOW_OFFLOAD_XMIT_NEIGH &&
	    route->tuple[!dir].xmit_type == FLOW_OFFLOAD_XMIT_NEIGH) {
		if (nf_dev_forward_path(skb, route, ct, dir, devs) ||
		    nf_dev_forward_path(skb, route, ct, !dir, devs)) {
			ret = -1;
			goto err_route_dir2;
		}
	}

	return 0;

err_route_dir2:
	dst_release(route->tuple[!dir].dst);
err_route_dir1:
	dst_release(route->tuple[dir].dst);
	return ret;
}

static unsigned int
flowoffload_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct xt_flowoffload_table *table;
	const struct xt_flowoffload_target_info *info = par->targinfo;
	struct tcphdr _tcph, *tcph = NULL;
	enum ip_conntrack_info ctinfo;
	enum ip_conntrack_dir dir;
	struct nf_flow_route route = {};
	struct flow_offload *flow = NULL;
	struct net_device *devs[2] = {};
	struct nf_conn *ct;
	struct net *net;

	if (xt_flowoffload_skip(skb, xt_family(par)))
		return XT_CONTINUE;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL)
		return XT_CONTINUE;

	switch (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum) {
	case IPPROTO_TCP:
		if (ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED)
			return XT_CONTINUE;

		tcph = skb_header_pointer(skb, par->thoff,
					  sizeof(_tcph), &_tcph);
		if (unlikely(!tcph || tcph->fin || tcph->rst))
			return XT_CONTINUE;
		break;
	case IPPROTO_UDP:
		break;
	default:
		return XT_CONTINUE;
	}

	if (nf_ct_ext_exist(ct, NF_CT_EXT_HELPER) ||
	    ct->status & IPS_SEQ_ADJUST)
		return XT_CONTINUE;

	if (!nf_ct_is_confirmed(ct))
		return XT_CONTINUE;

	devs[dir] = xt_out(par);
	devs[!dir] = xt_in(par);

	if (!devs[dir] || !devs[!dir])
		return XT_CONTINUE;

	if (test_and_set_bit(IPS_OFFLOAD_BIT, &ct->status))
		return XT_CONTINUE;

	dir = CTINFO2DIR(ctinfo);

	if (skb->protocol == htons(ETH_P_IPV6))
		ct->inet6_mode = CT_INET_MODE_IPV6;
	else
		ct->inet6_mode = 0;

	if (ct->status & IPS_NAT_MASK) {
		if (xt_flowoffload_route_nat(skb, ct, par, &route, dir, devs) < 0)
			goto err_flow_route;
	} else {
		if (xt_flowoffload_route_bridge(skb, ct, par, &route, dir, devs) < 0)
			goto err_flow_route;
	}

	flow = flow_offload_alloc(ct);
	if (!flow)
		goto err_flow_alloc;

	if (flow_offload_route_init(flow, &route) < 0)
		goto err_flow_add;

	if (xt_flowoffload_dscp_init(skb, flow, dir) < 0)
		goto err_flow_add;

	if (tcph) {
		ct->proto.tcp.seen[0].flags |= IP_CT_TCP_FLAG_BE_LIBERAL;
		ct->proto.tcp.seen[1].flags |= IP_CT_TCP_FLAG_BE_LIBERAL;
	}

	table = &flowtable[!!(info->flags & XT_FLOWOFFLOAD_HW)];

	net = read_pnet(&table->ft.net);
	if (!net)
		write_pnet(&table->ft.net, xt_net(par));

	if (flow_offload_add(&table->ft, flow) < 0)
		goto err_flow_add;

	xt_flowoffload_check_device(table, devs[0]);
	xt_flowoffload_check_device(table, devs[1]);

	if (!(ct->status & IPS_NAT_MASK))
		dst_release(route.tuple[dir].dst);
	dst_release(route.tuple[!dir].dst);

	return XT_CONTINUE;

err_flow_add:
	flow_offload_free(flow);
err_flow_alloc:
	if (!(ct->status & IPS_NAT_MASK))
		dst_release(route.tuple[dir].dst);
	dst_release(route.tuple[!dir].dst);
err_flow_route:
	clear_bit(IPS_OFFLOAD_BIT, &ct->status);

	return XT_CONTINUE;
}

static int flowoffload_chk(const struct xt_tgchk_param *par)
{
	struct xt_flowoffload_target_info *info = par->targinfo;

	if (info->flags & ~XT_FLOWOFFLOAD_MASK)
		return -EINVAL;

	return 0;
}

static struct xt_target offload_tg_reg __read_mostly = {
	.family		= NFPROTO_UNSPEC,
	.name		= "FLOWOFFLOAD",
	.revision	= 0,
	.targetsize	= sizeof(struct xt_flowoffload_target_info),
	.usersize	= sizeof(struct xt_flowoffload_target_info),
	.checkentry	= flowoffload_chk,
	.target		= flowoffload_tg,
	.me		= THIS_MODULE,
};

static int flow_offload_netdev_event(struct notifier_block *this,
				     unsigned long event, void *ptr)
{
	struct xt_flowoffload_hook *hook0, *hook1;
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);

	if (event != NETDEV_UNREGISTER)
		return NOTIFY_DONE;

	spin_lock_bh(&hooks_lock);
	hook0 = flow_offload_lookup_hook(&flowtable[0], dev);
	if (hook0)
		hlist_del(&hook0->list);

	hook1 = flow_offload_lookup_hook(&flowtable[1], dev);
	if (hook1)
		hlist_del(&hook1->list);
	spin_unlock_bh(&hooks_lock);

	if (hook0) {
		nf_unregister_net_hook(hook0->net, &hook0->ops);
		kfree(hook0);
	}

	if (hook1) {
		nf_unregister_net_hook(hook1->net, &hook1->ops);
		kfree(hook1);
	}

	nf_flow_table_cleanup(dev);

	return NOTIFY_DONE;
}

static struct notifier_block flow_offload_netdev_notifier = {
	.notifier_call	= flow_offload_netdev_event,
};

static int nf_flow_rule_route_inet(struct net *net,
				   const struct flow_offload *flow,
				   enum flow_offload_tuple_dir dir,
				   struct nf_flow_rule *flow_rule)
{
	const struct flow_offload_tuple *flow_tuple = &flow->tuplehash[dir].tuple;
	int err;

	switch (flow_tuple->l3proto) {
	case NFPROTO_IPV4:
		err = nf_flow_rule_route_ipv4(net, flow, dir, flow_rule);
		break;
	case NFPROTO_IPV6:
		err = nf_flow_rule_route_ipv6(net, flow, dir, flow_rule);
		break;
	default:
		err = -1;
		break;
	}

	return err;
}

static struct nf_flowtable_type flowtable_inet = {
	.family		= NFPROTO_INET,
	.init		= nf_flow_table_init,
	.setup		= nf_flow_table_offload_setup,
	.action		= nf_flow_rule_route_inet,
	.free		= nf_flow_table_free,
	.hook		= xt_flowoffload_net_hook,
	.owner		= THIS_MODULE,
};

static int init_flowtable(struct xt_flowoffload_table *tbl)
{
	INIT_DELAYED_WORK(&tbl->work, xt_flowoffload_hook_work);
	tbl->ft.type = &flowtable_inet;

	return nf_flow_table_init(&tbl->ft);
}

static int __init xt_flowoffload_tg_init(void)
{
	int ret;

	register_netdevice_notifier(&flow_offload_netdev_notifier);

	ret = init_flowtable(&flowtable[0]);
	if (ret)
		return ret;

	ret = init_flowtable(&flowtable[1]);
	if (ret)
		goto cleanup;

	flowtable[1].ft.flags = NF_FLOWTABLE_HW_OFFLOAD | NF_FLOWTABLE_COUNTER;

	ret = xt_register_target(&offload_tg_reg);
	if (ret)
		goto cleanup2;

	return 0;

cleanup2:
	nf_flow_table_free(&flowtable[1].ft);
cleanup:
	nf_flow_table_free(&flowtable[0].ft);
	return ret;
}

static void __exit xt_flowoffload_tg_exit(void)
{
	xt_unregister_target(&offload_tg_reg);
	unregister_netdevice_notifier(&flow_offload_netdev_notifier);
	nf_flow_table_free(&flowtable[0].ft);
	nf_flow_table_free(&flowtable[1].ft);
}

MODULE_LICENSE("GPL");
module_init(xt_flowoffload_tg_init);
module_exit(xt_flowoffload_tg_exit);
