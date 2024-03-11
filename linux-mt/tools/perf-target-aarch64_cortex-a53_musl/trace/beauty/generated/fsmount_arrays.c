static const char *fsmount_attr_flags[] = {
	[ilog2(0x00000001) + 1] = "RDONLY",
	[ilog2(0x00000002) + 1] = "NOSUID",
	[ilog2(0x00000004) + 1] = "NODEV",
	[ilog2(0x00000008) + 1] = "NOEXEC",
	[ilog2(0x00000010) + 1] = "NOATIME",
	[ilog2(0x00000020) + 1] = "STRICTATIME",
	[ilog2(0x00000080) + 1] = "NODIRATIME",
};
