#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "string.h"
#include "ring_buf.h"
#include "UART_lib.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"


static const char *TAG = "UART2";

#define UART_NUM       UART_NUM_2 
#define UART_TX_PIN    GPIO_NUM_17
#define UART_RX_PIN    GPIO_NUM_16
#define UART_BAUD      115200
#define BUF_SIZE       1024

#define WIFI_SSID      "TÀI VUONG HOME P209"
#define WIFI_PASS      "taivuongp209"

esp_mqtt_client_handle_t client;
QueueHandle_t uart_queue;
RingbufHandle_t uart_rb = NULL;

QueueHandle_t datetime_queue;

CIRC_BBUF_DEF(my_circ_buf, 32);

typedef struct{
	uint8_t second;
	uint8_t min;
	uint8_t hour;
	uint8_t day;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	float temp;
}DateTime_t;
DateTime_t *dt;

typedef struct {
    uint8_t cmd;
    DateTime_t dt;
    uint8_t  LED_state;
} MqttMsg_t;

static void uart_event_task(void *pvParameters) {
    uint8_t rxByte;
    uint8_t rxFlag = 0;
    for (;;) {
        int len = uart_read_bytes(UART_NUM_2, &rxByte, 1, portMAX_DELAY);
        if (len > 0) {
            if(rxByte == START_BYTE){
                rxFlag = 1;
            }
            if(rxFlag == 1){
                // push vào ring buffer riêng
                circ_bbuf_push(&my_circ_buf, rxByte);
            }
            if(rxByte == END_BYTE){
                UART_ReceiveFrame(&my_circ_buf);
                rxFlag = 0;
            }
        }
    }
}

static void mqtt_send_data(void *pvParameters) {
    MqttMsg_t msg;
    for (;;) {
        if (xQueueReceive(datetime_queue, &msg, portMAX_DELAY) == pdTRUE){
            switch (msg.cmd) {
                case CMD_TIME_RESP:{
                    char payload[128];
                    snprintf(payload, sizeof(payload),
                    "{ \"hour\": %d, \"min\": %d, \"sec\": %d, "
                    "\"day\": %d, \"date\": %d, \"month\": %d, \"year\": %d }",
                    msg.dt.hour, msg.dt.min, msg.dt.second,
                    msg.dt.day, msg.dt.date, msg.dt.month, 2000 + msg.dt.year);
                    int msg_id = esp_mqtt_client_publish(client,
                                    "esp32/datetime", payload, 0, 1, 0);
                    if (msg_id == -1) {
                        ESP_LOGW(TAG, "Publish failed");
                    } else {
                        ESP_LOGI(TAG, "Published: %s", payload);
                    }
                    break;
                }
                case CMD_TEMP_RESP:{
                    char payload[64];
                    snprintf(payload, sizeof(payload),
                            "{\"temp\": %.2f}", msg.dt.temp);
                    int msg_id = esp_mqtt_client_publish(client,
                                    "esp32/temperature", payload, 0, 1, 0);
                    if (msg_id == -1) {
                        ESP_LOGW(TAG, "Publish failed");
                    } else {
                        ESP_LOGI(TAG, "Published: %s", payload);
                    }
                    break;
                }
                case CMD_LED_RESP:{
                    char payload[64];
                    snprintf(payload, sizeof(payload),
                            "{\"LED\": %d}", msg.LED_state);
                    int msg_id = esp_mqtt_client_publish(client,
                                    "esp32/LEDState", payload, 0, 1, 0);
                    if (msg_id == -1) {
                        ESP_LOGW(TAG, "Publish failed");
                    } else {
                        ESP_LOGI(TAG, "Published: %s", payload);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void MyFrameHandler(CMD_RESP_t cmd, uint8_t *data, uint8_t len){
    MqttMsg_t msg;
    msg.cmd = cmd;
    switch (cmd) {
		case CMD_TIME_RESP:
            memcpy(&msg.dt, data,sizeof(DateTime_t));
            xQueueOverwrite(datetime_queue, &msg);
            ESP_LOGI("UART", "Receive: %02d:%02d:%02d",
                 msg.dt.hour, msg.dt.min, msg.dt.second);
			break;
        case CMD_TEMP_RESP:
            memcpy(&msg.dt, data,sizeof(DateTime_t));
            xQueueOverwrite(datetime_queue, &msg);
            ESP_LOGI("UART", "Receive Temperature: %.2f", msg.dt.temp);
            break;
        case CMD_LED_RESP:
            msg.LED_state = data[0];
            xQueueOverwrite(datetime_queue, &msg);
            ESP_LOGI("UART", "Receive LED State: %d", msg.LED_state);
            break;
		default:
			break;
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            // Đăng ký lắng nghe lệnh từ server
            esp_mqtt_client_subscribe(client, "esp32/cmd", 1);

            // yêu cầu STM32 trả lại trạng thái LED hiện tại
            const char *cmd = "GET_LED_STATE";
            UART_SendFrame(CMD_LED, (const uint8_t*)cmd, strlen(cmd));
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed to topic, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "Unsubscribed from topic, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data received");
            if(strncmp(event->data, "UPDATE_DATETIME", event->data_len) == 0){
                char* test_str = "UPDATE_DATETIME\n";
                UART_SendFrame(CMD_TIME, (uint8_t*)test_str, strlen(test_str));
            }else if(strncmp(event->data, "UPDATE_TEMP", event->data_len) == 0){
                char* test_str = "UPDATE_TEMP\n";
                UART_SendFrame(CMD_TEMP, (uint8_t*)test_str, strlen(test_str));
            }else if(strncmp(event->data, "UPDATE_LED", 10) == 0){
                //LED CMD: OFF
                char* payload = memchr(event->data, ':', event->data_len);
                if(payload != NULL){
                    payload++;
                    char cmd[5];
                    uint8_t len = event->data_len - (payload - event->data);
                    memcpy(cmd, payload, len);
                    cmd[len] = '\0';
                    ESP_LOGI(TAG, "LED CMD: %s", cmd);
                    UART_SendFrame(CMD_LED, (uint8_t*)cmd, len);
                }
            }else if (strncmp(event->data, "GET_LED_STATE", event->data_len) == 0) {
                const char *cmd = "GET_LED_STATE\n";
                UART_SendFrame(CMD_LED, (uint8_t*)cmd, strlen(cmd));
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            ESP_LOGI(TAG, "Other event id: %d", event->event_id);
            break;
    }
}


void wifi_init(void){
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}


void mqtt_init(){
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com:1883",
    };


    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void uart_init(){
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        // .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // Configure UART parameters
 
    uart_driver_install(UART_NUM_2, BUF_SIZE*2, BUF_SIZE*2, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    UART_RegisterCallback(MyFrameHandler);
}

void app_main(void)
{

    wifi_init();
    mqtt_init();
    uart_init();

    datetime_queue = xQueueCreate( 1, sizeof( MqttMsg_t ) );

    // xTaskCreate(uart_send_task, "uart_send_task", 4096, NULL, 12, NULL);

    // Task nhận UART và push vào ring buffer
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

    xTaskCreate(mqtt_send_data, "mqtt_send_data", 2048, NULL, 12, NULL);
    
}