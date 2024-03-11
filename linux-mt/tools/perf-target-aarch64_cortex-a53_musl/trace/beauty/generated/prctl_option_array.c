static const char *prctl_options[] = {
	[1] = "SET_PDEATHSIG",
	[2] = "GET_PDEATHSIG",
	[3] = "GET_DUMPABLE",
	[4] = "SET_DUMPABLE",
	[5] = "GET_UNALIGN",
	[6] = "SET_UNALIGN",
	[7] = "GET_KEEPCAPS",
	[8] = "SET_KEEPCAPS",
	[9] = "GET_FPEMU",
	[10] = "SET_FPEMU",
	[11] = "GET_FPEXC",
	[12] = "SET_FPEXC",
	[13] = "GET_TIMING",
	[14] = "SET_TIMING",
	[15] = "SET_NAME",
	[16] = "GET_NAME",
	[19] = "GET_ENDIAN",
	[20] = "SET_ENDIAN",
	[21] = "GET_SECCOMP",
	[22] = "SET_SECCOMP",
	[23] = "CAPBSET_READ",
	[24] = "CAPBSET_DROP",
	[25] = "GET_TSC",
	[26] = "SET_TSC",
	[27] = "GET_SECUREBITS",
	[28] = "SET_SECUREBITS",
	[29] = "SET_TIMERSLACK",
	[30] = "GET_TIMERSLACK",
	[31] = "TASK_PERF_EVENTS_DISABLE",
	[32] = "TASK_PERF_EVENTS_ENABLE",
	[33] = "MCE_KILL",
	[34] = "MCE_KILL_GET",
	[35] = "SET_MM",
	[36] = "SET_CHILD_SUBREAPER",
	[37] = "GET_CHILD_SUBREAPER",
	[38] = "SET_NO_NEW_PRIVS",
	[39] = "GET_NO_NEW_PRIVS",
	[40] = "GET_TID_ADDRESS",
	[41] = "SET_THP_DISABLE",
	[42] = "GET_THP_DISABLE",
	[43] = "MPX_ENABLE_MANAGEMENT",
	[44] = "MPX_DISABLE_MANAGEMENT",
	[45] = "SET_FP_MODE",
	[46] = "GET_FP_MODE",
	[47] = "CAP_AMBIENT",
	[50] = "SVE_SET_VL",
	[51] = "SVE_GET_VL",
	[52] = "GET_SPECULATION_CTRL",
	[53] = "SET_SPECULATION_CTRL",
	[54] = "PAC_RESET_KEYS",
	[55] = "SET_TAGGED_ADDR_CTRL",
	[56] = "GET_TAGGED_ADDR_CTRL",
};
static const char *prctl_set_mm_options[] = {
	[1] = "START_CODE",
	[2] = "END_CODE",
	[3] = "START_DATA",
	[4] = "END_DATA",
	[5] = "START_STACK",
	[6] = "START_BRK",
	[7] = "BRK",
	[8] = "ARG_START",
	[9] = "ARG_END",
	[10] = "ENV_START",
	[11] = "ENV_END",
	[12] = "AUXV",
	[13] = "EXE_FILE",
	[14] = "MAP",
	[15] = "MAP_SIZE",
};