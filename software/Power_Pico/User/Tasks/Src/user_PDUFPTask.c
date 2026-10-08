/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_PDUFPTask.h"
#include "main.h"
#include "fusb302_dev.h"
#include "user_FixedVoltage.h"
#include "pico_diag.h"
#include "user_AdcDataStrategy.h"
#include <string.h>
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

uint8_t _pd_working_status = 0;
osTimerId_t PD_UFP_Task_timer_id;
static fixed_voltage_control_t probe;
static fixed_voltage_control_status_t probe_snapshot;
static uint8_t probe_request;
static bool probe_owned;
static bool probe_phy_dirty;
static uint32_t probe_polled_at;
static uint8_t pd_timer_ticks;
static volatile bool pd_startup_done;
static bool sink_enabled, upgrade_pending, legacy_start_pending;
static uint32_t legacy_requested_at;
static fixed_voltage_control_status_t power_snapshot;
static uint16_t power_request_mv, power_request_ma;
static bool power_request_pending, power_request_pps;

void user_pd_power_status(fixed_voltage_control_status_t *status)
{
    int32_t lock = osKernelLock();
    *status = power_snapshot;
    if (lock == 0) osKernelRestoreLock(lock);
}

bool user_pd_power_request(uint16_t millivolts, uint16_t milliamps, bool pps)
{
    if (millivolts < 5000 || millivolts > 20000 ||
        (pps && (!milliamps || milliamps > 3000 ||
                 millivolts % 20 || milliamps % 50))) return false;
    int32_t lock = osKernelLock();
    bool allowed = !power_request_pending && !_pd_working_status &&
        !legacy_start_pending && !probe_owned && !probe.awaiting_crc &&
        (power_snapshot.state == fixed_voltage_control_READY ||
         (power_snapshot.contract_mv && (power_snapshot.state == fixed_voltage_control_REJECTED ||
          power_snapshot.state == fixed_voltage_control_WAIT || power_snapshot.state == fixed_voltage_control_TIMEOUT ||
          power_snapshot.state == fixed_voltage_control_NO_9V))) &&
        fixed_voltage_control_power_supported(&power_snapshot, millivolts, milliamps, pps);
    if (allowed) {
        power_request_mv = millivolts;
        power_request_ma = milliamps;
        power_request_pps = pps;
        power_request_pending = true;
        power_snapshot.state = fixed_voltage_control_ACCEPT;
    }
    if (lock == 0) osKernelRestoreLock(lock);
    return allowed;
}

void user_pd_boot_init(void)
{
    pico_diag_arm_ram();
    uint8_t result = fusb302_dev_init();
    pico_diag_log(DIAG_PD_BOOT, result);
    sink_enabled = result == 0;
    probe_phy_dirty = true;
    if (sink_enabled) {
        fusb302_dev.strict_attach = 1;
        fixed_voltage_control_start_5v(&probe, &fusb302_dev, HAL_GetTick());
        probe_polled_at = HAL_GetTick() - 20U;
    }
    pd_startup_done = true;
}

void user_fixed_voltage_control_request(bool start)
{
    int32_t lock = osKernelLock();
    probe_request = start ? 1 : 2;
    if (start) {
        memset(&probe_snapshot, 0, sizeof(probe_snapshot));
        probe_snapshot.state = fixed_voltage_control_ATTACH;
    }
    osKernelRestoreLock(lock);
}

void user_fixed_voltage_control_status(fixed_voltage_control_status_t *status)
{
    int32_t lock = osKernelLock();
    *status = probe_snapshot;
    osKernelRestoreLock(lock);
}

static void probe_publish(void)
{
    int32_t lock = osKernelLock();
    probe_snapshot = probe.status;
    if (upgrade_pending && probe.target_voltage == PD_V(5.0) &&
        fixed_voltage_control_active(&probe)) {
        /* Report foreground request progress, not the background 5V contract. */
        probe_snapshot.state = probe.status.state == fixed_voltage_control_ATTACH ?
            fixed_voltage_control_ATTACH : fixed_voltage_control_CAPS;
    }
    osKernelRestoreLock(lock);
}

static void probe_release(void)
{
    /* Cached status from the last poll; no extra read-to-clear register access. */
    pico_diag_log(DIAG_PROBE_PHY_STATUS,
        (uint32_t)fusb302_dev.reg_status[0] |
        ((uint32_t)fusb302_dev.reg_status[4] << 8) |
        ((uint32_t)fusb302_dev.interrupta << 16) |
        ((uint32_t)fusb302_dev.interruptb << 24));
    pico_diag_log(DIAG_PROBE_RELEASE_ATTACHED, probe.status.state);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    /* Releasing the page must not destroy a live contract or pending reply. */
    probe_owned = false;
    upgrade_pending = false;
    probe_phy_dirty = true;
}

static void probe_service(void)
{
    int32_t lock = osKernelLock();
    uint8_t request = probe_request;
    probe_request = 0;
    osKernelRestoreLock(lock);
    if (request == 2) {
        if (probe_owned) probe_release();
        lock = osKernelLock();
        memset(&probe_snapshot, 0, sizeof(probe_snapshot));
        osKernelRestoreLock(lock);
    } else if (request == 1) {
        if (_pd_working_status || legacy_start_pending) {
            lock = osKernelLock();
            probe_snapshot.state = fixed_voltage_control_BUSY;
            osKernelRestoreLock(lock);
            return;
        }
        osTimerStop(PD_UFP_Task_timer_id);
        pd_timer_ticks = 0;
        uint8_t stale_event;
        while (osMessageQueueGet(PD_handle_event_MsgQueue, &stale_event, NULL, 0) == osOK) {}
        probe_owned = true;
        /* No blocking EEPROM writes while a background PD handshake is live. */
        pico_diag_arm_ram();
        pico_diag_log(DIAG_PD_HANDOFF,
            ((uint32_t)probe.contract_voltage << 16) | probe.status.state);
        float voltage, current_ua;
        Data_Monitor_Get_Values(&voltage, &current_ua);
        pico_diag_log(DIAG_VOLTAGE,
            voltage >= 0.0f && voltage < 100.0f ? (uint32_t)(voltage * 1000.0f) : 0U);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
        pico_diag_log(DIAG_CC_ON, 0);
        if (!sink_enabled || !fixed_voltage_control_active(&probe)) {
            FUSB302_ret_t init_result = !sink_enabled ?
                FUSB302_init(&fusb302_dev) : FUSB302_probe_standby(&fusb302_dev);
            pico_diag_log(DIAG_PHY_INIT, init_result);
            fixed_voltage_control_start_5v(&probe, &fusb302_dev, HAL_GetTick());
            fusb302_dev.strict_attach = 1;
            sink_enabled = init_result == FUSB302_SUCCESS;
            if (!sink_enabled) probe.status.state = fixed_voltage_control_IO_ERROR;
            probe_polled_at = HAL_GetTick() - 20U;
        }
        upgrade_pending = probe.target_voltage == PD_V(5.0) ||
            probe.status.state == fixed_voltage_control_READY;
    }
    if (_pd_working_status) return;
    if (sink_enabled &&
        (HAL_GetTick() - probe_polled_at >= 20U ||
         HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET)) {
        probe_polled_at = HAL_GetTick();
        FUSB302_event_t events = 0;
        FUSB302_ret_t result = FUSB302_alert(&fusb302_dev, &events);
        if (result != FUSB302_SUCCESS && result != FUSB302_BUSY)
            probe.status.state = fixed_voltage_control_IO_ERROR;
        else {
            if (events & (FUSB302_EVENT_DETACHED | FUSB302_EVENT_HARD_RESET)) {
                if (probe_owned) {
                    probe.status.state = events & FUSB302_EVENT_DETACHED ?
                        fixed_voltage_control_DISCONNECTED : fixed_voltage_control_RESET;
                    probe_publish();
                    probe_release();
                }
                fusb302_dev.state = 0;
                fixed_voltage_control_start_5v(&probe, &fusb302_dev, HAL_GetTick());
                power_request_pending = false;
                lock = osKernelLock();
                power_snapshot = probe.status;
                osKernelRestoreLock(lock);
                return;
            }
            if (!fixed_voltage_control_active(&probe) && (events & FUSB302_EVENT_ATTACHED)) {
                fixed_voltage_control_start_5v(&probe, &fusb302_dev, HAL_GetTick());
            }
            if (!fixed_voltage_control_active(&probe) && (events & FUSB302_EVENT_RX_SOP)) {
                uint16_t h = fusb302_dev.rx_header;
                if (((h & 31U) == 1U && ((h >> 12) & 7U)) ||
                    ((h & 0x701FU) == 13U)) {
                    /* A late source advertisement/reset can revive 5V listening.
                     * Keep MessageIDs until an actual reset/detach is observed. */
                    probe.target_voltage = probe.protocol.fixed_probe_voltage = PD_V(5.0);
                    probe.pps_voltage = probe.pps_current = 0;
                    probe.protocol.fixed_9v_probe = true;
                    probe.protocol.PPS_voltage = probe.protocol.PPS_current = 0;
                    probe.status.state = fixed_voltage_control_CAPS;
                    probe.started_at = probe.retry_at = HAL_GetTick();
                    probe.retries = 0;
                    probe.soft_reset_tried = probe.waiting_soft_reset = false;
                }
            }
            fixed_voltage_control_events(&probe, events, HAL_GetTick());
        }
    }
    if (sink_enabled) fixed_voltage_control_tick(&probe, HAL_GetTick());
    if (power_request_pending && !_pd_working_status && !probe_owned) {
        if (!probe.awaiting_crc && (probe.status.state == fixed_voltage_control_READY ||
            (probe.status.contract_mv && (probe.status.state == fixed_voltage_control_REJECTED ||
             probe.status.state == fixed_voltage_control_WAIT || probe.status.state == fixed_voltage_control_TIMEOUT ||
             probe.status.state == fixed_voltage_control_NO_9V)))) {
            pico_diag_arm_ram();
            bool sent = power_request_pps ?
                fixed_voltage_control_request_pps(&probe, power_request_mv / 20U,
                                    power_request_ma / 50U, HAL_GetTick()) :
                fixed_voltage_control_request_fixed(&probe, power_request_mv / 50U, HAL_GetTick());
            power_request_pending = false;
            if (!sent) {
                probe.status.state = fixed_voltage_control_NO_9V;
                probe.status.last_error = fixed_voltage_control_NO_9V;
            }
        } else if (!fixed_voltage_control_active(&probe)) {
            power_request_pending = false;
        }
    }
    lock = osKernelLock();
    power_snapshot = probe.status;
    if (_pd_working_status || legacy_start_pending)
        power_snapshot.state = fixed_voltage_control_BUSY;
    else if (power_request_pending)
        power_snapshot.state = fixed_voltage_control_ACCEPT;
    osKernelRestoreLock(lock);
    if (probe_owned) {
        if (upgrade_pending && fixed_voltage_control_request_9v(&probe, HAL_GetTick())) {
            upgrade_pending = false;
        }
        probe_publish();
        if (!fixed_voltage_control_active(&probe)) probe_release();
    }
}

static void legacy_start_service(void)
{
    if (!legacy_start_pending || probe_owned || _pd_working_status) return;
    bool handoff = sink_enabled && probe.status.state == fixed_voltage_control_READY &&
        !probe.awaiting_crc && probe.contract_voltage != 0;
    if (sink_enabled && fixed_voltage_control_active(&probe) && !handoff &&
        HAL_GetTick() - legacy_requested_at < 5000U) return;
    legacy_start_pending = false;
    if (handoff) {
        memset(&app_pd, 0, sizeof(app_pd));
        app_pd.protocol = probe.protocol;
        app_pd.protocol.fixed_9v_probe = false;
        app_pd.cc = probe.status.cc;
        pico_diag_log(DIAG_PD_HANDOFF, 0x80000000U | probe.contract_voltage);
    } else {
        if (FUSB302_init(&fusb302_dev) != FUSB302_SUCCESS) {
            uint8_t failed = PD_EVT_PPS_FAILED;
            osMessageQueuePut(PD_handle_event_MsgQueue, &failed, 0, 0);
            sink_enabled = false;
            return;
        }
        memset(&app_pd, 0, sizeof(app_pd));
        PD_protocol_init(&app_pd.protocol);
    }
    sink_enabled = false;
    fusb302_dev.strict_attach = 0;
    probe_phy_dirty = false;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_5V);
    PD_protocol_set_PPS(&app_pd.protocol, PPS_V(5.0), PPS_A(1.0), false);
    app_pd.send_request = handoff;
    pd_timer_ticks = 0;
    osTimerStop(PD_UFP_Task_timer_id);
    _pd_working_status = 1;
    osTimerStart(PD_UFP_Task_timer_id, 1000);
}

/* Private function prototypes -----------------------------------------------*/

static void _timer_callback(void *argument)
{
    if (sink_enabled || probe_owned) return;
    PD_handle_event_t ready;
    pd_timer_ticks++;
    if(is_PPS_ready()) {
        ready = PD_EVT_PPS_READY;
        osMessageQueuePut(PD_handle_event_MsgQueue, &ready, 0, 1);
        pd_timer_ticks = 0;
        osTimerStop(PD_UFP_Task_timer_id);
    } else if (is_PD_Fixed_ready()) {
        ready = PD_EVT_FIXED_READY;
        osMessageQueuePut(PD_handle_event_MsgQueue, &ready, 0, 1);
        pd_timer_ticks = 0;
        osTimerStop(PD_UFP_Task_timer_id);
    } else if(pd_timer_ticks >= 5) { // 5秒超时
        ready = PD_EVT_PPS_FAILED;
        osMessageQueuePut(PD_handle_event_MsgQueue, &ready, 0, 1);
        pd_timer_ticks = 0;
        osTimerStop(PD_UFP_Task_timer_id);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET); // FUSB CC pin disconnect
        _pd_working_status = 0;
    }
}

/**
  * @brief  task for PD_UFP
  * @param  argument: Not used
  * @retval None
  */
void PDUFPTask(void *argument)
{
  // 快充诱骗时需要另外一个端口也供电，因为快充口会有可能断电
  // 可以检测 status_power 来判断是否有PPS供电
  PD_command_msg_t ui_msg;
  while (!pd_startup_done) osDelay(1);
  // 创建定时器
  PD_UFP_Task_timer_id = osTimerNew(_timer_callback, osTimerPeriodic, NULL, NULL);
	while(1)
	{
    probe_service();
    // 非阻塞获取消息
    if(osMessageQueueGet(PD_cmd_MessageQueue, &ui_msg, NULL, 0)==osOK) {
      if (probe_owned) {
        osDelay(1);
        continue;
      }
      if(!_pd_working_status && ui_msg.event == PD_CMD_START) {
        if (!legacy_start_pending) legacy_requested_at = HAL_GetTick();
        legacy_start_pending = true;

      } else if(ui_msg.event == PD_CMD_STOP) {
        legacy_start_pending = false;
        if (!_pd_working_status) continue;
        // FUSB CC pin disconnect
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
        _pd_working_status = 0;

      } else if(_pd_working_status && ui_msg.event == PD_CMD_SET_PPS) {
        // 设置PPS电压电流
        if(ui_msg.pps_set_voltage > 0 && ui_msg.pps_set_current > 0 && ui_msg.pps_set_voltage <= 20.0 && ui_msg.pps_set_current <= 5.0) {
          PD_protocol_set_PPS(&app_pd.protocol, PPS_V(ui_msg.pps_set_voltage), PPS_A(ui_msg.pps_set_current), false);
          send_power_request();
        }
      } else if(_pd_working_status && ui_msg.event == PD_CMD_SET_PD_FIXED) {
        // 设置固定电压电流
        bool need_send = false;
        switch (ui_msg.pd_fixed_level)
        {
          case PD_FIXED_VOL_LEVEL_5V:
            need_send = PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_5V);
            break;
          case PD_FIXED_VOL_LEVEL_9V:
            need_send = PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_9V);
            break;
          case PD_FIXED_VOL_LEVEL_12V:
            need_send = PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_12V);
            break;
          case PD_FIXED_VOL_LEVEL_15V:
            need_send = PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_15V);
            break;
          case PD_FIXED_VOL_LEVEL_20V:
            need_send = PD_protocol_set_power_option(&app_pd.protocol, PD_POWER_OPTION_MAX_20V);
            break;
          default:
            break;
        }

        if (need_send) {
          send_power_request();
        }
      }
    }

    legacy_start_service();
    if(_pd_working_status)
    {
      if (fusb302_timer() || HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET)
      {
          FUSB302_event_t FUSB302_events = 0;
          for (uint8_t i = 0; i < 3 && FUSB302_alert(&fusb302_dev, &FUSB302_events) != FUSB302_SUCCESS; i++) {}
          if (FUSB302_events) {
              handle_FUSB302_event(FUSB302_events);
          }
      }
    }
		osDelay(1);
	}
}
