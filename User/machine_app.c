#include "lvgl_demo.h"
#include "./BSP/LED/led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "LVGL/GUI_APP/lv_ui.h"

#define START_TASK_PRIO          1
#define START_STK_SIZE           128

#define UI_TASK_PRIO             3
#define UI_STK_SIZE              1024

#define LED_TASK_PRIO            1
#define LED_STK_SIZE             128

#define MACHINE_TASK_PRIO        4
#define MACHINE_STK_SIZE         256

#define MOTION_TASK_PRIO         3
#define MOTION_STK_SIZE          256

#define IO_SCAN_TASK_PRIO        3
#define IO_SCAN_STK_SIZE         256

#define ALARM_TASK_PRIO          3
#define ALARM_STK_SIZE           256

#define STORAGE_TASK_PRIO        2
#define STORAGE_STK_SIZE         256

#define COMM_TASK_PRIO           2
#define COMM_STK_SIZE            256

#define MACHINE_EVT_ESTOP        (1U << 0)
#define MACHINE_EVT_AUTO_MODE    (1U << 1)
#define MACHINE_EVT_RUNNING      (1U << 2)
#define MACHINE_EVT_ALARM        (1U << 3)
#define MACHINE_EVT_HOME_DONE    (1U << 4)
#define MACHINE_EVT_NETWORK_OK   (1U << 5)

typedef enum
{
    MACHINE_STATE_IDLE = 0,
    MACHINE_STATE_RUNNING,
    MACHINE_STATE_STOPPING,
    MACHINE_STATE_ALARM
} machine_state_t;

typedef enum
{
    MOTION_CMD_NONE = 0,
    MOTION_CMD_HOME_X,
    MOTION_CMD_HOME_Y,
    MOTION_CMD_HOME_Z,
    MOTION_CMD_STOP_ALL,
    MOTION_CMD_MOVE_X,
    MOTION_CMD_MOVE_Y,
    MOTION_CMD_MOVE_Z
} motion_cmd_type_t;

typedef struct
{
    motion_cmd_type_t type;
    int32_t position;
    uint32_t speed;
} motion_cmd_t;

typedef enum
{
    MACHINE_EVENT_NONE = 0,
    MACHINE_EVENT_START_BUTTON,
    MACHINE_EVENT_STOP_BUTTON,
    MACHINE_EVENT_ESTOP_ON,
    MACHINE_EVENT_ESTOP_OFF,
    MACHINE_EVENT_HOME_DONE,
    MACHINE_EVENT_LIMIT_TRIGGERED
} machine_event_type_t;

typedef struct
{
    machine_event_type_t type;
    uint32_t data;
} machine_event_t;

typedef enum
{
    ALARM_NONE = 0,
    ALARM_ESTOP,
    ALARM_LIMIT,
    ALARM_MOTION_TIMEOUT,
    ALARM_SENSOR_ERROR,
    ALARM_COMM_ERROR
} alarm_code_t;

typedef struct
{
    alarm_code_t code;
    uint32_t data;
} alarm_msg_t;

typedef enum
{
    PARAM_SAVE_ALL = 0,
    PARAM_SAVE_PRODUCT,
    PARAM_SAVE_MACHINE
} param_save_type_t;

typedef struct
{
    param_save_type_t type;
} param_save_msg_t;

static TaskHandle_t start_task_handler;
static TaskHandle_t ui_task_handler;
static TaskHandle_t led_task_handler;
static TaskHandle_t machine_task_handler;
static TaskHandle_t motion_task_handler;
static TaskHandle_t io_scan_task_handler;
static TaskHandle_t alarm_task_handler;
static TaskHandle_t storage_task_handler;
static TaskHandle_t comm_task_handler;

static QueueHandle_t ui_cmd_queue;
static QueueHandle_t motion_cmd_queue;
static QueueHandle_t machine_event_queue;
static QueueHandle_t alarm_queue;
static QueueHandle_t param_save_queue;
static EventGroupHandle_t machine_event_group;

static void start_task(void *pvParameters);
static void ui_task(void *pvParameters);
static void led_task(void *pvParameters);
static void machine_ctrl_task(void *pvParameters);
static void motion_task(void *pvParameters);
static void io_scan_task(void *pvParameters);
static void alarm_task(void *pvParameters);
static void storage_task(void *pvParameters);
static void comm_task(void *pvParameters);
static void machine_create_ipc(void);

uint8_t machine_ui_send_cmd(machine_ui_cmd_type_t type, int32_t value)
{
    machine_ui_cmd_t cmd;

    if (ui_cmd_queue == NULL)
    {
        return 0;
    }

    cmd.type = type;
    cmd.value = value;

    return (xQueueSend(ui_cmd_queue, &cmd, 0) == pdPASS) ? 1U : 0U;
}

void lvgl_demo(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    xTaskCreate(start_task,
                "start_task",
                START_STK_SIZE,
                NULL,
                START_TASK_PRIO,
                &start_task_handler);

    vTaskStartScheduler();
}

static void machine_create_ipc(void)
{
    ui_cmd_queue = xQueueCreate(10, sizeof(machine_ui_cmd_t));
    motion_cmd_queue = xQueueCreate(10, sizeof(motion_cmd_t));
    machine_event_queue = xQueueCreate(10, sizeof(machine_event_t));
    alarm_queue = xQueueCreate(10, sizeof(alarm_msg_t));
    param_save_queue = xQueueCreate(5, sizeof(param_save_msg_t));
    machine_event_group = xEventGroupCreate();
}

static void start_task(void *pvParameters)
{
    (void)pvParameters;

    taskENTER_CRITICAL();

    machine_create_ipc();

    xTaskCreate(ui_task, "ui_task", UI_STK_SIZE, NULL, UI_TASK_PRIO, &ui_task_handler);
    xTaskCreate(led_task, "led_task", LED_STK_SIZE, NULL, LED_TASK_PRIO, &led_task_handler);
    xTaskCreate(machine_ctrl_task, "machine_ctrl", MACHINE_STK_SIZE, NULL, MACHINE_TASK_PRIO, &machine_task_handler);
    xTaskCreate(motion_task, "motion_task", MOTION_STK_SIZE, NULL, MOTION_TASK_PRIO, &motion_task_handler);
    xTaskCreate(io_scan_task, "io_scan", IO_SCAN_STK_SIZE, NULL, IO_SCAN_TASK_PRIO, &io_scan_task_handler);
    xTaskCreate(alarm_task, "alarm_task", ALARM_STK_SIZE, NULL, ALARM_TASK_PRIO, &alarm_task_handler);
    xTaskCreate(storage_task, "storage_task", STORAGE_STK_SIZE, NULL, STORAGE_TASK_PRIO, &storage_task_handler);
    xTaskCreate(comm_task, "comm_task", COMM_STK_SIZE, NULL, COMM_TASK_PRIO, &comm_task_handler);

    taskEXIT_CRITICAL();

    vTaskDelete(start_task_handler);
}

static void ui_task(void *pvParameters)
{
    (void)pvParameters;

    lv_ui();

    while (1)
    {
        lv_timer_handler();
        vTaskDelay(5);
    }
}

static void led_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        LED0_TOGGLE();
        vTaskDelay(1000);
    }
}

static void machine_ctrl_task(void *pvParameters)
{
    machine_state_t state = MACHINE_STATE_IDLE;
    machine_ui_cmd_t ui_cmd;
    machine_event_t event;
    motion_cmd_t motion_cmd;
    param_save_msg_t save_msg;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(ui_cmd_queue, &ui_cmd, 10) == pdPASS)
        {
            switch (ui_cmd.type)
            {
                case MACHINE_UI_CMD_AUTO_START:
                    state = MACHINE_STATE_RUNNING;
                    xEventGroupSetBits(machine_event_group, MACHINE_EVT_RUNNING);
                    break;

                case MACHINE_UI_CMD_AUTO_STOP:
                    state = MACHINE_STATE_STOPPING;
                    motion_cmd.type = MOTION_CMD_STOP_ALL;
                    motion_cmd.position = 0;
                    motion_cmd.speed = 0;
                    xQueueSend(motion_cmd_queue, &motion_cmd, 0);
                    xEventGroupClearBits(machine_event_group, MACHINE_EVT_RUNNING);
                    state = MACHINE_STATE_IDLE;
                    break;

                case MACHINE_UI_CMD_MODE_SINGLE:
                    xEventGroupClearBits(machine_event_group, MACHINE_EVT_AUTO_MODE);
                    break;

                case MACHINE_UI_CMD_MODE_CONTINUOUS:
                    xEventGroupSetBits(machine_event_group, MACHINE_EVT_AUTO_MODE);
                    break;

                case MACHINE_UI_CMD_MANUAL_HOME_X:
                case MACHINE_UI_CMD_MANUAL_HOME_Y:
                case MACHINE_UI_CMD_MANUAL_HOME_Z:
                    motion_cmd.position = 0;
                    motion_cmd.speed = 0;
                    motion_cmd.type = (ui_cmd.type == MACHINE_UI_CMD_MANUAL_HOME_X) ? MOTION_CMD_HOME_X :
                                      (ui_cmd.type == MACHINE_UI_CMD_MANUAL_HOME_Y) ? MOTION_CMD_HOME_Y :
                                                                                      MOTION_CMD_HOME_Z;
                    xQueueSend(motion_cmd_queue, &motion_cmd, 0);
                    break;

                case MACHINE_UI_CMD_PARAM_SAVE:
                    save_msg.type = PARAM_SAVE_ALL;
                    xQueueSend(param_save_queue, &save_msg, 0);
                    break;

                default:
                    break;
            }
        }

        while (xQueueReceive(machine_event_queue, &event, 0) == pdPASS)
        {
            if (event.type == MACHINE_EVENT_ESTOP_ON || event.type == MACHINE_EVENT_LIMIT_TRIGGERED)
            {
                alarm_msg_t alarm;

                state = MACHINE_STATE_ALARM;
                xEventGroupSetBits(machine_event_group, MACHINE_EVT_ALARM);

                motion_cmd.type = MOTION_CMD_STOP_ALL;
                motion_cmd.position = 0;
                motion_cmd.speed = 0;
                xQueueSend(motion_cmd_queue, &motion_cmd, 0);

                alarm.code = (event.type == MACHINE_EVENT_ESTOP_ON) ? ALARM_ESTOP : ALARM_LIMIT;
                alarm.data = event.data;
                xQueueSend(alarm_queue, &alarm, 0);
            }
        }

        if (state == MACHINE_STATE_RUNNING)
        {
            /* Add the automatic snap-fit machine sequence here. */
        }

        vTaskDelay(10);
    }
}

static void motion_task(void *pvParameters)
{
    motion_cmd_t cmd;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(motion_cmd_queue, &cmd, portMAX_DELAY) == pdPASS)
        {
            switch (cmd.type)
            {
                case MOTION_CMD_HOME_X:
                case MOTION_CMD_HOME_Y:
                case MOTION_CMD_HOME_Z:
                case MOTION_CMD_MOVE_X:
                case MOTION_CMD_MOVE_Y:
                case MOTION_CMD_MOVE_Z:
                case MOTION_CMD_STOP_ALL:
                    /* Add motor, cylinder, and driver control here. */
                    break;

                default:
                    break;
            }
        }
    }
}

static void io_scan_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        /* Scan E-stop, limit switches, home sensors, pressure, and material sensors here. */
        vTaskDelay(10);
    }
}

static void alarm_task(void *pvParameters)
{
    alarm_msg_t alarm;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(alarm_queue, &alarm, portMAX_DELAY) == pdPASS)
        {
            (void)alarm;
            /* Update alarm records, buzzer, indicator light, and UI alarm state here. */
        }
    }
}

static void storage_task(void *pvParameters)
{
    param_save_msg_t msg;

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(param_save_queue, &msg, portMAX_DELAY) == pdPASS)
        {
            (void)msg;
            /* Save product and machine parameters to Flash or 24CXX here. */
        }
    }
}

static void comm_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        /* Add Ethernet, WiFi, MES, or upper-computer communication here. */
        vTaskDelay(100);
    }
}
