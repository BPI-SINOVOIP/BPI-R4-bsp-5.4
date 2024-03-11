static const char *move_mount_flags[] = {
	[ilog2(0x00000001) + 1] = "F_SYMLINKS",
	[ilog2(0x00000002) + 1] = "F_AUTOMOUNTS",
	[ilog2(0x00000004) + 1] = "F_EMPTY_PATH",
	[ilog2(0x00000010) + 1] = "T_SYMLINKS",
	[ilog2(0x00000020) + 1] = "T_AUTOMOUNTS",
	[ilog2(0x00000040) + 1] = "T_EMPTY_PATH",
};
