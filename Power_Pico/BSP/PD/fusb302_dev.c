#include "fusb302_dev.h"
#include "i2c.h"
#include "cmsis_os2.h"

#define FUSB302_USE_OS 1

#if FUSB302_USE_OS == 1
    #define clock_ms() osKernelGetTickCount()
    #define delay_ms(ms) osDelay(ms)
#else
    #define clock_ms() HAL_GetTick()
    #define delay_ms(ms) HAL_Delay(ms)
#endif



#define PD_POLLING              100
#define TYPEC_SINK_WAIT_CAP     350
#define REQUEST_TO_PS_READY     580
#define PPS_REQUEST             5000

FUSB302_dev_t fusb302_dev;
App_PD_t app_pd;

void fusb302_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    HAL_I2C_Mem_Read(&hi2c1, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, data, count, 10);
}

void fusb302_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t count)
{
    HAL_I2C_Mem_Write(&hi2c1, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, data, count, 10);
}


void status_power_ready(status_power_t status, uint16_t voltage, uint16_t current)
{
    app_pd.ready_voltage = voltage;
    app_pd.ready_current = current;
    app_pd.status_power = status;
}

void set_default_power(void)
{
    status_power_ready(STATUS_POWER_TYP, PD_V(5.0), PD_A(1.0));
}

void handle_protocol_event(PD_protocol_event_t events)
{
    if (events & PD_PROTOCOL_EVENT_SRC_CAP) {
        app_pd.wait_src_cap = 0;
        app_pd.get_src_cap_retry_count = 0;
        app_pd.wait_ps_rdy = 1;
        app_pd.time_wait_ps_rdy = clock_ms();
        app_pd.state = PD_STATE_GET_CAPS; // 更新状态：已收到能力，可以请求了
    }
    if (events & PD_PROTOCOL_EVENT_REJECT) {
        if (app_pd.wait_ps_rdy) {
            app_pd.wait_ps_rdy = 0;
            app_pd.state = PD_STATE_FAILED; // 更新状态：请求被拒绝
        }
    }
    if (events & PD_PROTOCOL_EVENT_PS_RDY) {
        PD_power_info_t p;
        uint8_t i, selected_power = PD_protocol_get_selected_power(&app_pd.protocol);
        PD_protocol_get_power_info(&app_pd.protocol, selected_power, &p);
        app_pd.wait_ps_rdy = 0;
        if (p.type == PD_PDO_TYPE_AUGMENTED_PDO) {
            // PPS mode
            FUSB302_set_vbus_sense(&fusb302_dev, 0);
            if (app_pd.PPS_voltage_next) {
                // Two stage startup for PPS voltage < 5V
                PD_protocol_set_PPS(&app_pd.protocol, app_pd.PPS_voltage_next, app_pd.PPS_current_next, false);
                app_pd.PPS_voltage_next = 0;
                app_pd.send_request = 1;
            } else {
                app_pd.time_PPS_request = clock_ms();
                status_power_ready(STATUS_POWER_PPS,
                PD_protocol_get_PPS_voltage(&app_pd.protocol), PD_protocol_get_PPS_current(&app_pd.protocol));
                app_pd.state = PD_STATE_SUCCESS_PPS; // 更新状态：成功协商到PPS
            }
        } else {
            FUSB302_set_vbus_sense(&fusb302_dev, 1);
            status_power_ready(STATUS_POWER_TYP, p.max_v, p.max_i);
            app_pd.state = PD_STATE_SUCCESS_FIXED; // 更新状态：协商到了一个固定电压
        }
    }
}

void handle_FUSB302_event(FUSB302_event_t events)
{
    if (events & FUSB302_EVENT_DETACHED) {
        PD_protocol_reset(&app_pd.protocol);
        app_pd.cc = 0;
        return;
    }
    if (events & FUSB302_EVENT_ATTACHED) {
        uint8_t cc1 = 0, cc2 = 0, cc = 0;
        FUSB302_get_cc(&fusb302_dev, &cc1, &cc2);
        PD_protocol_reset(&app_pd.protocol);
        if (cc1 && cc2 == 0) {
            cc = cc1;
            app_pd.cc = 1;
        } else if (cc2 && cc1 == 0) {
            cc = cc2;
            app_pd.cc = 2;
        }
        /* TODO: handle no cc detected error */
        if (cc > 1) {
            app_pd.wait_src_cap = 1;
        } else {
            set_default_power();
        }
    }
    if (events & FUSB302_EVENT_RX_SOP) {
        PD_protocol_event_t protocol_event = 0;
        uint16_t header;
        uint32_t obj[7];
        FUSB302_get_message(&fusb302_dev, &header, obj);
        PD_protocol_handle_msg(&app_pd.protocol, header, obj, &protocol_event);
        if (protocol_event) {
            handle_protocol_event(protocol_event);
        }
    }
    if (events & FUSB302_EVENT_GOOD_CRC_SENT) {
        uint16_t header;
        uint32_t obj[7];
        delay_ms(4);  /* Delay respond in case there are retry messages */
        if (PD_protocol_respond(&app_pd.protocol, &header, obj)) {
            FUSB302_tx_sop(&fusb302_dev, header, obj);
        }
    }
}

bool fusb302_timer(void)
{
    uint32_t t = clock_ms();
    if (app_pd.wait_src_cap && t - app_pd.time_wait_src_cap > TYPEC_SINK_WAIT_CAP) {
        app_pd.time_wait_src_cap = t;
        if (app_pd.get_src_cap_retry_count < 3) {
            uint16_t header;
            app_pd.get_src_cap_retry_count += 1;
            /* Try to request soruce capabilities message (will not cause power cycle VBUS) */
            PD_protocol_create_get_src_cap(&app_pd.protocol, &header);
            FUSB302_tx_sop(&fusb302_dev, header, 0);
        } else {
            app_pd.get_src_cap_retry_count = 0;
            /* Hard reset will cause the source power cycle VBUS. */
            FUSB302_tx_hard_reset(&fusb302_dev);
            PD_protocol_reset(&app_pd.protocol);
        }
    }
    if (app_pd.wait_ps_rdy) {
        if (t - app_pd.time_wait_ps_rdy > REQUEST_TO_PS_READY) {
            app_pd.wait_ps_rdy = 0;
            set_default_power();
        }
    } else if (app_pd.send_request || (app_pd.status_power == STATUS_POWER_PPS && t - app_pd.time_PPS_request > PPS_REQUEST)) {
        app_pd.wait_ps_rdy = 1;
        app_pd.send_request = 0;
        app_pd.state = PD_STATE_NEGOTIATING; // 更新状态：正在协商中
        app_pd.time_PPS_request = t;
        uint16_t header;
        uint32_t obj[7];
        /* Send request if option updated or regularly in PPS mode to keep power alive */
        PD_protocol_create_request(&app_pd.protocol, &header, obj);
        app_pd.time_wait_ps_rdy = clock_ms();
        FUSB302_tx_sop(&fusb302_dev, header, obj);
    }
    if (t - app_pd.time_polling > PD_POLLING) {
        app_pd.time_polling = t;
        return true;
    }
    return false;
}

bool is_power_ready(void) { return app_pd.status_power == STATUS_POWER_TYP; }
bool is_PPS_ready(void)   { return app_pd.status_power == STATUS_POWER_PPS; }
void send_power_request(void) { app_pd.send_request = 1; }

uint8_t fusb302_dev_init(void) {
    fusb302_dev.i2c_address = 0x22<<1; // FUSB302 I2C address
    fusb302_dev.i2c_read = fusb302_i2c_read;
    fusb302_dev.i2c_write = fusb302_i2c_write;
    #if FUSB302_USE_OS == 1
        fusb302_dev.delay_ms = osDelay;
    #else
        fusb302_dev.delay_ms = HAL_Delay;
    #endif
    if (FUSB302_init(&fusb302_dev) == FUSB302_SUCCESS && FUSB302_get_ID(&fusb302_dev, 0, 0) == FUSB302_SUCCESS) {
        PD_protocol_init(&app_pd.protocol);
        app_pd.state = PD_STATE_IDLE; // 初始状态
        return 0;
    }
    return 1;
}
