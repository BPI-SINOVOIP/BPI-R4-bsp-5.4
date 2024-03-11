// SPDX-License-Identifier: GPL-2.0-only

#ifndef __MTK_WED_MCU_H
#define __MTK_WED_MCU_H

#define EXCEPTION_LOG_SIZE		32768
#define WOCPU_MCUSYS_RESET_ADDR		0x15194050
#define WOCPU_WO0_MCUSYS_RESET_MASK 	0x20
#define WOCPU_WO1_MCUSYS_RESET_MASK 	0x1

#define WARP_INVALID_LENGTH_STATUS (-2)
#define WARP_NULL_POINTER_STATUS (-3)
#define WARP_INVALID_PARA_STATUS (-4)
#define WARP_NOT_HANDLE_STATUS (-5)
#define WARP_FAIL_STATUS (-1)
#define WARP_OK_STATUS (0)
#define WARP_ALREADY_DONE_STATUS (1)

#define MT7981_FIRMWARE_WO		"mediatek/mt7981_wo.bin"
#define MTK_FIRMWARE_WO_0		"mediatek/mtk_wo_0.bin"
#define MTK_FIRMWARE_WO_1		"mediatek/mtk_wo_1.bin"
#define MTK_FIRMWARE_WO_2		"mediatek/mtk_wo_2.bin"

#define WOCPU_EMI_DEV_NODE		"mediatek,wocpu_emi"
#define WOCPU_ILM_DEV_NODE		"mediatek,wocpu_ilm"
#define WOCPU_DLM_DEV_NODE		"mediatek,wocpu_dlm"
#define WOCPU_DATA_DEV_NODE		"mediatek,wocpu_data"
#define WOCPU_BOOT_DEV_NODE		"mediatek,wocpu_boot"

#define FW_DL_TIMEOUT		((3000 * HZ) / 1000)
#define WOCPU_TIMEOUT		((1000 * HZ) / 1000)

#define MAX_REGION_SIZE	3

#define WOX_MCU_CFG_LS_BASE	0 /*0x15194000*/

#define WOX_MCU_CFG_LS_HW_VER_ADDR		(WOX_MCU_CFG_LS_BASE + 0x000) // 4000
#define WOX_MCU_CFG_LS_FW_VER_ADDR		(WOX_MCU_CFG_LS_BASE + 0x004) // 4004
#define WOX_MCU_CFG_LS_CFG_DBG1_ADDR		(WOX_MCU_CFG_LS_BASE + 0x00C) // 400C
#define WOX_MCU_CFG_LS_CFG_DBG2_ADDR 		(WOX_MCU_CFG_LS_BASE + 0x010) // 4010
#define WOX_MCU_CFG_LS_WF_MCCR_ADDR		(WOX_MCU_CFG_LS_BASE + 0x014) // 4014
#define WOX_MCU_CFG_LS_WF_MCCR_SET_ADDR		(WOX_MCU_CFG_LS_BASE + 0x018) // 4018
#define WOX_MCU_CFG_LS_WF_MCCR_CLR_ADDR		(WOX_MCU_CFG_LS_BASE + 0x01C) // 401C
#define WOX_MCU_CFG_LS_WF_MCU_CFG_WM_WA_ADDR	(WOX_MCU_CFG_LS_BASE + 0x050) // 4050
#define WOX_MCU_CFG_LS_WM_BOOT_ADDR_ADDR 	(WOX_MCU_CFG_LS_BASE + 0x060) // 4060
#define WOX_MCU_CFG_LS_WA_BOOT_ADDR_ADDR	(WOX_MCU_CFG_LS_BASE + 0x064) // 4064

#define WOX_MCU_CFG_LS_WF_MCU_CFG_WM_WA_WM_CPU_RSTB_MASK	BIT(5)
#define WOX_MCU_CFG_LS_WF_MCU_CFG_WM_WA_WA_CPU_RSTB_MASK	BIT(0)


enum wo_event_id {
	WO_EVT_LOG_DUMP = 0x1,
	WO_EVT_PROFILING = 0x2,
	WO_EVT_RXCNT_INFO = 0x3
};

enum wo_state {
	WO_STATE_UNDEFINED 	= 0x0,
	WO_STATE_INIT 		= 0x1,
	WO_STATE_ENABLE		= 0x2,
	WO_STATE_DISABLE	= 0x3,
	WO_STATE_HALT		= 0x4,
	WO_STATE_GATING		= 0x5,
	WO_STATE_SER_RESET 	= 0x6,
	WO_STATE_WF_RESET	= 0x7,
	WO_STATE_END
};

enum wo_done_state {
	WOIF_UNDEFINED		= 0,
	WOIF_DISABLE_DONE 	= 1,
	WOIF_TRIGGER_ENABLE	= 2,
	WOIF_ENABLE_DONE	= 3,
	WOIF_TRIGGER_GATING	= 4,
	WOIF_GATING_DONE	= 5,
	WOIF_TRIGGER_HALT	= 6,
	WOIF_HALT_DONE		= 7,
};

enum wed_dummy_cr_idx {
	WED_DUMMY_CR_FWDL = 0,
	WED_DUMMY_CR_WO_STATUS = 1
};

struct mtk_wed_fw_trailer {
	u8 chip_id;
	u8 eco_code;
	u8 n_region;
	u8 format_ver;
	u8 format_flag;
	u8 reserved[2];
	char fw_ver[10];
	char build_date[15];
	u32 crc;
};

#endif
