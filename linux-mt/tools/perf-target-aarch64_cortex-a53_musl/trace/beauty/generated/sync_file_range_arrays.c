static const char *sync_file_range_flags[] = {
	[ilog2(1) + 1] = "WAIT_BEFORE",
	[ilog2(2) + 1] = "WRITE",
	[ilog2(4) + 1] = "WAIT_AFTER",
};
