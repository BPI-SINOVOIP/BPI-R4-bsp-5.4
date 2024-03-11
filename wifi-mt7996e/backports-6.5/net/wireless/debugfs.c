// SPDX-License-Identifier: GPL-2.0-only
/*
 * cfg80211 debugfs
 *
 * Copyright 2009	Luis R. Rodriguez <lrodriguez@atheros.com>
 * Copyright 2007	Johannes Berg <johannes@sipsolutions.net>
 */

#include <linux/slab.h>
#include "core.h"
#include "debugfs.h"
#include "rdev-ops.h"

#define DEBUGFS_READONLY_FILE(name, buflen, fmt, value...)		\
static ssize_t name## _read(struct file *file, char __user *userbuf,	\
			    size_t count, loff_t *ppos)			\
{									\
	struct wiphy *wiphy = file->private_data;			\
	char buf[buflen];						\
	int res;							\
									\
	res = scnprintf(buf, buflen, fmt "\n", ##value);		\
	return simple_read_from_buffer(userbuf, count, ppos, buf, res);	\
}									\
									\
static const struct file_operations name## _ops = {			\
	.read = name## _read,						\
	.open = simple_open,						\
	.llseek = generic_file_llseek,					\
}

DEBUGFS_READONLY_FILE(rts_threshold, 20, "%d",
		      wiphy->rts_threshold);
DEBUGFS_READONLY_FILE(fragmentation_threshold, 20, "%d",
		      wiphy->frag_threshold);
DEBUGFS_READONLY_FILE(short_retry_limit, 20, "%d",
		      wiphy->retry_short);
DEBUGFS_READONLY_FILE(long_retry_limit, 20, "%d",
		      wiphy->retry_long);

static int ht_print_chan(struct ieee80211_channel *chan,
			 char *buf, int buf_size, int offset)
{
	if (WARN_ON(offset > buf_size))
		return 0;

	if (chan->flags & IEEE80211_CHAN_DISABLED)
		return scnprintf(buf + offset,
				 buf_size - offset,
				 "%d Disabled\n",
				 chan->center_freq);

	return scnprintf(buf + offset,
			 buf_size - offset,
			 "%d HT40 %c%c\n",
			 chan->center_freq,
			 (chan->flags & IEEE80211_CHAN_NO_HT40MINUS) ?
				' ' : '-',
			 (chan->flags & IEEE80211_CHAN_NO_HT40PLUS) ?
				' ' : '+');
}

static ssize_t ht40allow_map_read(struct file *file,
				  char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	struct wiphy *wiphy = file->private_data;
	char *buf;
	unsigned int offset = 0, buf_size = PAGE_SIZE, i;
	enum nl80211_band band;
	struct ieee80211_supported_band *sband;
	ssize_t r;

	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++)
			offset += ht_print_chan(&sband->channels[i],
						buf, buf_size, offset);
	}

	r = simple_read_from_buffer(user_buf, count, ppos, buf, offset);

	kfree(buf);

	return r;
}

static const struct file_operations ht40allow_map_ops = {
	.read = ht40allow_map_read,
	.open = simple_open,
	.llseek = default_llseek,
};

static int dfs_print_chan(struct ieee80211_channel *chan, int remain_time, int wait_time,
			  char *buf, int buf_size, int offset, bool is_background)
{
	if (WARN_ON(offset > buf_size))
		return 0;

	if (chan->dfs_state == NL80211_DFS_UNAVAILABLE) {
		offset += scnprintf(buf + offset, buf_size - offset,
				    "	Channel = %d, DFS_state = Unavailable",
				    chan->hw_value);
		if (remain_time > 0)
			offset += scnprintf(buf + offset, buf_size - offset,
					    ", Non-occupancy Remain Time = %d / %d [sec]",
					    remain_time, wait_time);
		else
			offset += scnprintf(buf + offset, buf_size - offset,
					    ", Changing state...");
	} else if (chan->dfs_state == NL80211_DFS_USABLE) {
		offset += scnprintf(buf + offset, buf_size - offset,
				    "	Channel = %d, DFS_state = Usable",
				    chan->hw_value);
		if (remain_time > 0)
			offset += scnprintf(buf + offset, buf_size - offset,
					    ", CAC Remain Time = %d / %d [sec]",
					    remain_time, wait_time);
	} else if (chan->dfs_state == NL80211_DFS_AVAILABLE) {
		offset += scnprintf(buf + offset, buf_size - offset,
				    "	Channel = %d, DFS_state = Available",
				    chan->hw_value);
	} else {
		offset += scnprintf(buf + offset, buf_size - offset,
				    "	Channel = %d, DFS_state = Unknown",
				    chan->hw_value);
	}

	if (is_background)
		offset += scnprintf(buf + offset, buf_size - offset,
				    " (background chain)");
	offset += scnprintf(buf + offset, buf_size - offset, "\n");

	return offset;
}

static int dfs_status_read_wdev(struct wiphy *wiphy, struct wireless_dev *wdev, char *buf,
				unsigned int buf_size, unsigned int offset)
{
	struct cfg80211_registered_device *rdev = wiphy_to_rdev(wiphy);
	struct cfg80211_chan_def *chandef = &wdev->links[0].ap.chandef;
	struct cfg80211_chan_def *background_chandef = &rdev->background_radar_chandef;
	enum nl80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	unsigned long jiffies_passed;
	int i, remain_time = 0, wait_time_ms = 0;
	bool is_background;

	offset += scnprintf(buf + offset, buf_size - offset, "DFS Channel:\n");

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++) {
			is_background = false;
			chan = &sband->channels[i];

			if (!(chan->flags & IEEE80211_CHAN_RADAR))
				continue;

			if (chan->dfs_state == NL80211_DFS_UNAVAILABLE) {
				jiffies_passed = jiffies - chan->dfs_state_entered;
				wait_time_ms = IEEE80211_DFS_MIN_NOP_TIME_MS;
				remain_time = (wait_time_ms - jiffies_to_msecs(jiffies_passed));
				if (remain_time > wait_time_ms)
					remain_time = 0;
			} else if (chan->dfs_state == NL80211_DFS_USABLE) {
				if (wdev->cac_started &&
				    cfg80211_is_sub_chan(chandef, chan, false)) {
					jiffies_passed = jiffies - wdev->cac_start_time;
					wait_time_ms = wdev->cac_time_ms;
					remain_time = (wait_time_ms -
						       jiffies_to_msecs(jiffies_passed));
				}

				if (rdev->background_radar_wdev == wdev &&
				    rdev->background_cac_started &&
				    cfg80211_is_sub_chan(background_chandef, chan, false)) {
					jiffies_passed = jiffies - rdev->background_cac_start_time;
					wait_time_ms = rdev->background_cac_time_ms;
					remain_time = (wait_time_ms -
						       jiffies_to_msecs(jiffies_passed));
					is_background = true;
				}

				if (remain_time > wait_time_ms)
					remain_time = 0;

			} else {
				if (rdev->background_radar_wdev == wdev &&
				    cfg80211_is_sub_chan(background_chandef, chan, false))
					is_background = true;
			}

			offset = dfs_print_chan(chan, remain_time / 1000, wait_time_ms / 1000,
						buf, buf_size, offset, is_background);
			remain_time = 0;
		}
	}

	return offset;
}

static ssize_t dfs_status_read(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct wiphy *wiphy = file->private_data;
	struct wireless_dev *wdev;
	char *buf;
	unsigned int offset = 0, buf_size = PAGE_SIZE, r;
	const char * const iftype_str[] = {
		[NL80211_IFTYPE_UNSPECIFIED] = "unspecified",
		[NL80211_IFTYPE_ADHOC] = "adhoc",
		[NL80211_IFTYPE_STATION] = "station",
		[NL80211_IFTYPE_AP] = "ap",
		[NL80211_IFTYPE_AP_VLAN] = "ap vlan",
		[NL80211_IFTYPE_WDS] = "wds",
		[NL80211_IFTYPE_MONITOR] = "monitor",
		[NL80211_IFTYPE_MESH_POINT] = "mesh point",
		[NL80211_IFTYPE_P2P_CLIENT] = "p2p client",
		[NL80211_IFTYPE_P2P_GO] = "p2p go",
		[NL80211_IFTYPE_P2P_DEVICE] = "p2p device",
		[NL80211_IFTYPE_OCB] = "ocb",
		[NL80211_IFTYPE_NAN] = "nan",
	};

	buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	list_for_each_entry(wdev, &wiphy->wdev_list, list) {
		offset += scnprintf(buf + offset, buf_size - offset,
				    "wdev 0x%x\n"
				    "interface type %s\n",
				    wdev->identifier, iftype_str[wdev->iftype]);
		offset = dfs_status_read_wdev(wiphy, wdev, buf, buf_size, offset);
	}

	r = simple_read_from_buffer(user_buf, count, ppos, buf, offset);

	kfree(buf);

	return r;
}

static const struct file_operations dfs_status_ops = {
	.read = dfs_status_read,
	.open = simple_open,
	.llseek = default_llseek,
};

static int
dfs_nop_skip(void *data, u64 val)
{
	struct wiphy *wiphy = data;
	struct cfg80211_registered_device *rdev = wiphy_to_rdev(wiphy);
	bool en = !!val;
	enum nl80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	u32 nop_time = IEEE80211_DFS_MIN_NOP_TIME_MS;
	int i;

	if (!en)
		return 0;

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++) {
			chan = &sband->channels[i];

			if (!(chan->flags & IEEE80211_CHAN_RADAR))
				continue;

			if (chan->dfs_state == NL80211_DFS_UNAVAILABLE) {
				// Let current jiffies > dfs_state_entered_jiffies + NOP time
				chan->dfs_state_entered = jiffies -
						       msecs_to_jiffies(nop_time + 1);
			}
		}
	}

	cfg80211_sched_dfs_chan_update(rdev);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(dfs_skip_nop_ops, NULL,
			 dfs_nop_skip, "0x%08llx\n");

static int
dfs_cac_skip(void *data, u64 val)
{
#define CAC_SKIP_MASK			BIT(0)
#define CAC_SKIP_BACKGROUND_MASK	BIT(1)
	struct wiphy *wiphy = data;
	struct cfg80211_registered_device *rdev = wiphy_to_rdev(wiphy);
	struct wireless_dev *wdev;
	struct cfg80211_chan_def *chandef;
	unsigned int skip_mode = val;
	unsigned long cac_time;
	struct ieee80211_channel *chan;

	if (!skip_mode || skip_mode > (CAC_SKIP_MASK | CAC_SKIP_BACKGROUND_MASK))
		return 0;

	list_for_each_entry(wdev, &wiphy->wdev_list, list) {
		if ((skip_mode & CAC_SKIP_MASK) && wdev->links[0].ap.chandef.chan) {
			chandef = &wdev->links[0].ap.chandef;
			chan = chandef->chan;

			if ((cfg80211_chandef_dfs_required(wiphy, chandef, wdev->iftype) > 0) &&
			    cfg80211_chandef_dfs_usable(wiphy, chandef) && wdev->cac_started) {
				rdev_skip_cac(rdev, wdev);
			}
		}

		if ((skip_mode & CAC_SKIP_BACKGROUND_MASK) &&
		    rdev->background_radar_wdev == wdev &&
		    rdev->background_radar_chandef.chan) {
			chandef = &rdev->background_radar_chandef;
			chan = chandef->chan;

			if ((cfg80211_chandef_dfs_required(wiphy, chandef, wdev->iftype) > 0) &&
			    cfg80211_chandef_dfs_usable(wiphy, chandef) &&
			    rdev->background_cac_started) {
				// Let current jiffies > dfs_state_entered_jiffies + CAC time
				cac_time = rdev->background_cac_time_ms;
				rdev->background_cac_start_time = jiffies -
								  msecs_to_jiffies(cac_time + 1);
				cancel_delayed_work(&rdev->background_cac_done_wk);
				queue_delayed_work(cfg80211_wq, &rdev->background_cac_done_wk, 0);
			}
		}
	}

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(dfs_skip_cac_ops, NULL,
			 dfs_cac_skip, "0x%08llx\n");

static int
dfs_available_reset(void *data, u64 val)
{
	struct wiphy *wiphy = data;
	struct cfg80211_registered_device *rdev = wiphy_to_rdev(wiphy);
	bool en = !!val;
	enum nl80211_band band;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	int i;

	if (!en)
		return 0;

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;
		for (i = 0; i < sband->n_channels; i++) {
			chan = &sband->channels[i];

			if (!(chan->flags & IEEE80211_CHAN_RADAR))
				continue;

			if (chan->dfs_state == NL80211_DFS_AVAILABLE) {
				chan->dfs_state = NL80211_DFS_USABLE;
				chan->dfs_state_entered = jiffies;
			}
		}
	}

	cfg80211_sched_dfs_chan_update(rdev);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(dfs_available_reset_ops, NULL,
			 dfs_available_reset, "0x%08llx\n");

#define DEBUGFS_ADD(name, chmod)						\
	debugfs_create_file(#name, chmod, phyd, &rdev->wiphy, &name## _ops)

void cfg80211_debugfs_rdev_add(struct cfg80211_registered_device *rdev)
{
	struct dentry *phyd = rdev->wiphy.debugfsdir;

	DEBUGFS_ADD(rts_threshold, 0444);
	DEBUGFS_ADD(fragmentation_threshold, 0444);
	DEBUGFS_ADD(short_retry_limit, 0444);
	DEBUGFS_ADD(long_retry_limit, 0444);
	DEBUGFS_ADD(ht40allow_map, 0444);
	DEBUGFS_ADD(dfs_status, 0444);
	DEBUGFS_ADD(dfs_skip_nop, 0600);
	DEBUGFS_ADD(dfs_skip_cac, 0600);
	DEBUGFS_ADD(dfs_available_reset, 0600);
}
