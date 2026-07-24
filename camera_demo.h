/*****************************************************************/ /**
* @file camera_demo.h
* @brief
* @author bronson.zhan@quectel.com
* @date 2025-06-30
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2025-06-30 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#ifndef __CAMERA_DEMO_H__
#define __CAMERA_DEMO_H__

#include "qosa_def.h"
#include "qosa_sys.h"

/*===========================================================================
 *  Macro Definition
 ===========================================================================*/
#define CONFIG_UNIRTOS_CAMERA_DEMO_TASK_STACK_SIZE (220*1024)                  // demo task stack size configuration
#define UNIR_CAMERA_DEMO_TASK_PRIO                 QOSA_PRIORITY_NORMAL  // demo task priority configuration
#define UNIR_CAMERA_DEMO_CHANNEL                   0                     // camera channel configuration

// Select corresponding I2C channel, PIN, and function configuration based on hardware connection
#if 1
#define UNIR_EC800ZCNLC_I2C0_SCL_PIN  57
#define UNIR_EC800ZCNLC_I2C0_SCL_FUNC (2)

#define UNIR_EC800ZCNLC_I2C0_SDA_PIN  58
#define UNIR_EC800ZCNLC_I2C0_SDA_FUNC (2)
#else
#define UNIR_EC800ZCNLC_I2C0_SCL_PIN  67
#define UNIR_EC800ZCNLC_I2C0_SCL_FUNC (2)

#define UNIR_EC800ZCNLC_I2C0_SDA_PIN  66
#define UNIR_EC800ZCNLC_I2C0_SDA_FUNC (2)
#endif

// USP1 PIN and corresponding function configuration
#define UNIR_EC800ZCNLC_USP1_MCLK_PIN  54
#define UNIR_EC800ZCNLC_USP1_MCLK_FUNC (1)

#define UNIR_EC800ZCNLC_USP1_BCLK_PIN  80
#define UNIR_EC800ZCNLC_USP1_BCLK_FUNC (1)

#define UNIR_EC800ZCNLC_USP1_LRCK_PIN  81
#define UNIR_EC800ZCNLC_USP1_LRCK_FUNC (1)

#define UNIR_EC800ZCNLC_USP1_DIN_PIN   55
#define UNIR_EC800ZCNLC_USP1_DIN_FUNC  (1)

#define UNIR_EC800ZCNLC_USP1_DOUT_PIN  56
#define UNIR_EC800ZCNLC_USP1_DOUT_FUNC (1)

/*===========================================================================
 *  Function Declaration
 ===========================================================================*/
void unir_camera_demo_init(void);

#endif /* __CAMERA_DEMO_H__ */
