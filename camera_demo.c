/*****************************************************************/ /**
* @file camera_demo.c
* @brief
* @author bronson.zhan@quectel.com
* @date 2025-06-30
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description
* <tr><td>2025-06-30 <td>1.0 <td>Bronson.Zhan <td> Init
* </table>
**********************************************************************/
#include "qosa_sys.h"
#include "qosa_camera.h"
#include "qosa_gpio.h"
#include "qosa_def.h"
#include "qosa_log.h"
#include "camera_demo.h"
#include "qosa_uart.h"
#include "qosa_dev_eigen.h"
#include "qosa_rtc.h"
#include "qosa_pinctrl.h"
#include "unirtos_app_init_registry.h"
#ifdef CONFIG_QOSA_CODE_DECODER_SUPPORT
#include "qosa_cam_decoder.h"
#endif /* CONFIG_QOSA_CODE_DECODER_SUPPORT */

/*===========================================================================
 *  Macro Definition
 ===========================================================================*/
#define QOS_LOG_TAG LOG_TAG_CAMERA_API

#ifdef CONFIG_QOSA_CODE_DECODER_SUPPORT
#define QOSA_CAMERA_DECODER_DEBUG
#endif

/*===========================================================================
 *  Variate
 ===========================================================================*/
static qosa_task_t g_unir_camera_demo_task = QOSA_NULL;
//The decoding library will define local arrays, please pay attention to the stack size of the decoding thread
static unsigned int g_unir_camera_demo_task_stack[CONFIG_UNIRTOS_CAMERA_DEMO_TASK_STACK_SIZE/4] = {0};
static qosa_uint8_t camera_tcb[200]={0};


#ifdef QOSA_CAMERA_DECODER_DEBUG
//Whether to enable camera debug mode
static int camera_debug=0;
#endif

#define CAMERA_TCB_SIZE                         200
#define UNIR_UART1_TX_PIN                        (18)
#define UNIR_UART1_RX_PIN                        (17)
#define UNIR_UART1_PIN_FUNC                      (1)

/*===========================================================================
 *  Static API Functions
 ===========================================================================*/
/**
 * @brief Camera demo function, initializes camera related pins and starts the image preview process.
 *
 * @param ctx Task context pointer (not used)
 *
 * This function first configures pin functions for I2C and USP1 interfaces, then initializes
 * the camera module, and enters a continuous loop to acquire preview image data.
 */
 #ifdef QOSA_CAMERA_DECODER_DEBUG
/**
 * @brief UART event callback handling function
 *
 * This function is used to handle various event indicators of UART ports, including receiving data, sending completion, and sending buffer low water level events.
 * When an event occurs, the event information will be sent out through UART.
 *
 * @param cb_param Pointer to the UART callback parameter structure, which contains information such as port number, event ID, and user data.
 */
static void unir_usb_cdc_cb(qosa_uart_cb_param_t *cb_param)
{
    qosa_uart_port_number_e port = cb_param->port;
    qosa_uint32_t           event_id = cb_param->event_id;
    char                    data[128] = {0};
    qosa_snprintf(data, sizeof(data), "port=%d, event_id=%d, user_data=%s", port, event_id, (unsigned char *)cb_param->user_data);

    if (cb_param->event_id & QOSA_UART_EVENT_RX_INDICATE)//DEBUG
    {
        camera_debug=1;
        QLOGI("UART QOSA_UART_EVENT_RX_INDICATE");
    }
    else if (cb_param->event_id & QOSA_UART_EVENT_TX_COMPLETE)
    {
        QLOGI("UART QOSA_UART_EVENT_TX_COMPLETE");
    }
    else if (cb_param->event_id & QOSA_UART_EVENT_TX_LOW)
    {
        QLOGI("UART QOSA_UART_EVENT_TX_LOW");
    }
}
#endif /* QOSA_CAMERA_DECODER_DEBUG */


static void unir_camera_demo_process(void *ctx)
{
    int           ret = 0;
    qosa_uint8_t *pCamDataBuffer = QOSA_NULL;
    int cam_buffer_mode = QOSA_CAMERA_DOUBLE_BUFFER;
    qosa_camera_opt_t   camera_opt = {0};

#ifdef CONFIG_QOSA_CODE_DECODER_SUPPORT
    qosa_decoder_type_e type = QOSA_DECODER_TYPE_NONE;
    qosa_uint8_t        result[256] = {0};
    char uart_decode_msg[256] = {0};
    qosa_uint8_t        version[32] = {0};

#endif /* CONFIG_QOSA_CODE_DECODER_SUPPORT */

    qosa_task_sleep_ms(3000);

 #ifdef QOSA_CAMERA_DECODER_DEBUG
   // Initialize USB CDC port, export image
    qosa_uart_status_monitor_t monitor = {0};
    monitor.callback = unir_usb_cdc_cb;
    monitor.event_mask = QOSA_UART_EVENT_RX_INDICATE | QOSA_UART_EVENT_TX_COMPLETE;
    qosa_uart_register_cb(QOSA_USB_PORT_ACM0, &monitor);

    qosa_uart_config_t dcb_config = {0};
    dcb_config.baudrate = QOSA_UART_BAUD_115200;
    dcb_config.data_bit = QOSA_UART_DATABIT_8;
    dcb_config.flow_ctrl = QOSA_FC_NONE;
    dcb_config.parity_bit = QOSA_UART_PARITY_NONE;
    dcb_config.stop_bit = QOSA_UART_STOP_1;

    qosa_uart_ioctl(QOSA_USB_PORT_ACM0, QOSA_UART_IOCTL_SET_DCB_CFG, (void *)&dcb_config);
    /* Open USB CDC */
    qosa_uart_open(QOSA_USB_PORT_ACM0);
#endif

    // Initialize the main UART to output the decoded results
    qosa_uart_status_monitor_t main_uart_monitor = {0};
    main_uart_monitor.callback = QOSA_NULL;
    main_uart_monitor.event_mask =0;
    /* Register UART event callback */
    qosa_uart_register_cb(QOSA_UART_PORT_1, &main_uart_monitor);

    /* Configure UART communication parameters: baud rate, data bits, stop bits, parity bit, flow control */
    qosa_uart_config_t main_uart_config = {0};
    main_uart_config.baudrate = QOSA_UART_BAUD_115200;
    main_uart_config.data_bit = QOSA_UART_DATABIT_8;
    main_uart_config.flow_ctrl = QOSA_FC_NONE;
    main_uart_config.parity_bit = QOSA_UART_PARITY_NONE;
    main_uart_config.stop_bit = QOSA_UART_STOP_1;

    qosa_uart_ioctl(QOSA_UART_PORT_1, QOSA_UART_IOCTL_SET_DCB_CFG, (void *)&main_uart_config);
    /* Configure the function of UART port pins */
    qosa_pin_set_func(UNIR_UART1_TX_PIN,UNIR_UART1_PIN_FUNC);
    qosa_pin_set_func(UNIR_UART1_RX_PIN,UNIR_UART1_PIN_FUNC);
    /* Open the UART port */
    ret = qosa_uart_open(QOSA_UART_PORT_1);


    // Configure pin functions for I2C interface: SCL and SDA
    qosa_pin_set_func(UNIR_EC800ZCNLC_I2C0_SCL_PIN, UNIR_EC800ZCNLC_I2C0_SCL_FUNC);
    qosa_pin_set_func(UNIR_EC800ZCNLC_I2C0_SDA_PIN, UNIR_EC800ZCNLC_I2C0_SDA_FUNC);

    // Configure pins related to USP1: MCLK, BCLK, DIN, DOUT
    qosa_pin_set_func(UNIR_EC800ZCNLC_USP1_MCLK_PIN, UNIR_EC800ZCNLC_USP1_MCLK_FUNC);
    qosa_pin_set_func(UNIR_EC800ZCNLC_USP1_BCLK_PIN, UNIR_EC800ZCNLC_USP1_BCLK_FUNC);

    /* No Camera currently uses this CLK, and this PIN is used as PWDN. Enabling it will cause PWDN to fail, so it's not recommended to enable */
    // qosa_pin_set_func(UNIR_EC800ZCNLC_USP1_LRCK_PIN, UNIR_EC800ZCNLC_USP1_LRCK_FUNC);
    qosa_pin_set_func(UNIR_EC800ZCNLC_USP1_DIN_PIN, UNIR_EC800ZCNLC_USP1_DIN_FUNC);
    qosa_pin_set_func(UNIR_EC800ZCNLC_USP1_DOUT_PIN, UNIR_EC800ZCNLC_USP1_DOUT_FUNC);

    // Initialize camera related modules and print log
    QLOGI("CAMERA INIT !!!");
    ret = qosa_camera_init(QOSA_CAMERA_1, 0, 0, QOSA_CAMERA_OUTPUT_ONLY_Y);
    if (ret != QOSA_CAMERA_SUCCESS)
    {
        QLOGE("CAMERA INIT FAILED[%d]", ret);
        return;
    }
    qosa_camera_ioctl(QOSA_CAMERA_1, QOSA_CAMERA_IOCTL_SET_BUFFER_OPT, &cam_buffer_mode);

#ifdef CONFIG_QOSA_CODE_DECODER_SUPPORT
    // Initialize decoder library
    qosa_qr_decoder_init();

    // Get decoder library version
    qosa_get_decoder_version(version);
    QLOGI("version=%s", version);
#endif /* CONFIG_QOSA_CODE_DECODER_SUPPORT */

    QLOGI("CAMERA DEMO TESTING...");

    // Start camera and enter image preview loop
    qosa_camera_start(QOSA_CAMERA_1);
    // Get current Camera width and height
    qosa_camera_ioctl(QOSA_CAMERA_1, QOSA_CAMERA_IOCTL_GET_OPT, &camera_opt);

#ifdef QOSA_CAMERA_DECODER_DEBUG
    qosa_uint32_t start_decoder_tick1 = 0;
    qosa_uint32_t end_decoder_tick2   = 0;
    // Sending photos requires sending one packet every 150ms with a 4K interval. The baud rate can be adjusted to reduce the transmission time.
    int write_conut = 0;
    write_conut = camera_opt.camera_image_max_width*camera_opt.camera_image_max_height/4096;
    QLOGI("Camera Info W:%d,H:%d", camera_opt.camera_image_max_width,camera_opt.camera_image_max_height);
#endif /* QOSA_CAMERA_DECODER_DEBUG */

    while (1)
    {
        // Get current preview image data
        qosa_camera_preview_image(QOSA_CAMERA_1, &pCamDataBuffer);

#ifdef CONFIG_QOSA_CODE_DECODER_SUPPORT
        // Try to decode
        #ifdef QOSA_CAMERA_DECODER_DEBUG
        start_decoder_tick1 = qosa_get_system_tick_cnt();
        int iret = qosa_qr_image_decoder(QOSA_CAMERA_1, pCamDataBuffer, (uint32_t)camera_opt.camera_image_max_width, (uint32_t)camera_opt.camera_image_max_height);
        end_decoder_tick2 = qosa_get_system_tick_cnt();
        QLOGI("DECODER time consuming--%dms",end_decoder_tick2-start_decoder_tick1);
        #else
        int iret = qosa_qr_image_decoder(QOSA_CAMERA_1, pCamDataBuffer, (uint32_t)camera_opt.camera_image_max_width, (uint32_t)camera_opt.camera_image_max_height);
        #endif /* QOSA_CAMERA_DECODER_DEBUG */

        if (iret == QOSA_DECODER_SUCCESS)
        {
            iret = qosa_qr_get_decoder_result(&type, result);
            if (iret == QOSA_DECODER_SUCCESS)
            {
                QLOGI("----------DECODER SUccess type=%d, result=%s----------", type, result);
                //Output to the main serial port
                qosa_snprintf(uart_decode_msg, sizeof(uart_decode_msg), "+QCAMPRES: 0,0,%d,%s\r\n", qosa_strlen((char*)result), result);
                qosa_uart_write(QOSA_UART_PORT_1, (unsigned char *)uart_decode_msg,qosa_strlen((const char*)uart_decode_msg));
            }
            else
            {
                //Do Nothing, no need to notify
                QLOGI("decode failed");
            }
        }
#ifdef QOSA_CAMERA_DECODER_DEBUG
        if(QOSA_DECODER_SUCCESS != iret && camera_debug)
        {
            QLOGI("decode failed,Start dump picture");
            if(camera_debug==1)
            {
                // Output image, continuous photo output, add 16 'A' characters as image header here, can use Python script to split files
                qosa_uart_write(QOSA_USB_PORT_ACM0, (unsigned char *)"AAAAAAAAAAAAAAAA", 16);
                for(int i=0;i<write_conut;i++)
                {
                    ret=qosa_uart_write(QOSA_USB_PORT_ACM0, (unsigned char *)(pCamDataBuffer+i*4096), 4096);
                    qosa_task_sleep_ms(150);
                }
            }
        }
#endif /* QOSA_CAMERA_DECODER_DEBUG */

#endif /* CONFIG_QOSA_CODE_DECODER_SUPPORT */

        qosa_camera_free_preview_image(QOSA_CAMERA_1, pCamDataBuffer);
    }
}

/*===========================================================================
 *  Public API Functions
 ===========================================================================*/

void unir_camera_demo_init(void)
{
    QLOGI("enter UniRTOS CAMERA DEMO !!!");

    if (g_unir_camera_demo_task == QOSA_NULL)
    {
        qosa_task_create_static(
            &g_unir_camera_demo_task,
            g_unir_camera_demo_task_stack,
            CONFIG_UNIRTOS_CAMERA_DEMO_TASK_STACK_SIZE,
            camera_tcb,
            CAMERA_TCB_SIZE,
            QOSA_PRIORITY_HIGH,
            "camera_demo",
            unir_camera_demo_process,
            NULL);
    }
}

UNIRTOS_APP_EXPORT(321, "camera_demo", unir_camera_demo_init);