#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LOCAL_DIR := $(call GET_LOCAL_DIR)

MODULE := gpio

LOCAL_SRCS-y := drivers/gpio/gpio.c
LOCAL_SRCS-y += ${LOCAL_DIR}/mtgpio_common.c
LOCAL_SRCS-y += ${LOCAL_DIR}/${MTK_SOC}/mtgpio.c

PLAT_INCLUDES += -I${LOCAL_DIR}
PLAT_INCLUDES += -I${LOCAL_DIR}/${MTK_SOC}

$(eval $(call MAKE_MODULE,$(MODULE),$(LOCAL_SRCS-y),$(MTK_BL)))
