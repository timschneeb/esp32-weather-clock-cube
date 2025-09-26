//
// Created by tim on 24.09.25.
//

#ifndef CONFIG_H
#define CONFIG_H

// Touch button
#define TOUCH_PIN 9
#define TOUCH_THRESHOLD 1500
#define TOUCH_LONG_PRESS_MS 500

// Backlight configuration
#define TFT_BL 14
#define TFT_BL_PWM_CHANNEL 0

// AP WiFi mode
#define DEFAULT_SSID "ESP32_AP"
#define DEFAULT_PASSWORD "admin1234"

// Timeouts
#define KEEPALIVE_TIMEOUT (5 * 60 * 1000) /* 5 minutes */

// QuarkTS
#define Q_PRIORITY_LEVELS           ( 3 )
#define Q_PRIO_QUEUE_SIZE           ( 10 )
#define Q_ALLOW_SCHEDULER_RELEASE   ( 1 )
#define Q_PRESERVE_TASK_ENTRY_ORDER ( 1 )
#define Q_BYTE_ALIGNMENT            ( 8 )
#define Q_DEFAULT_HEAP_SIZE         ( 0 )
#define Q_FSM                       ( 1 )
#define Q_FSM_MAX_NEST_DEPTH        ( 5 )
#define Q_FSM_MAX_TIMEOUTS          ( 3 )
#define Q_FSM_PS_SIGNALS_MAX        ( 8 )
#define Q_FSM_PS_SUB_PER_SIGNAL_MAX ( 4 )
#define Q_TRACE_BUFSIZE             ( 36 )
#define Q_CLI                       ( 1 )
#define Q_QUEUES                    ( 1 )
#define Q_LOGGER_COLORED            ( 0 )

#endif //CONFIG_H
