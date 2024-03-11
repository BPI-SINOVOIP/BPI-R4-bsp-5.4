static const char *pkey_alloc_access_rights[] = {
	[0x1 ? (ilog2(0x1) + 1) : 0] = "DISABLE_ACCESS",
	[0x2 ? (ilog2(0x2) + 1) : 0] = "DISABLE_WRITE",
};
