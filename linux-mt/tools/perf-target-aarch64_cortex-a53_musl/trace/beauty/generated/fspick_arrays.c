static const char *fspick_flags[] = {
	[ilog2(0x00000001) + 1] = "CLOEXEC",
	[ilog2(0x00000002) + 1] = "SYMLINK_NOFOLLOW",
	[ilog2(0x00000004) + 1] = "NO_AUTOMOUNT",
	[ilog2(0x00000008) + 1] = "EMPTY_PATH",
};
