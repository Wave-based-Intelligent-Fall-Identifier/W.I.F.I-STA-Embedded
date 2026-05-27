#include "wifi.h"
#include "baseline_filter.h"
#include <math.h>

static EventGroupHandle_t wifiEventGroup;
static int retryCounts = 0;
static const char* TAG = "WiFi";
QueueHandle_t csi_queue;

//printf된 TX MAC ADDRESS 값 삽입
static const uint8_t TX_MAC_ADDRESS[6] = {
    0x34, 0x85, 0x18, 0xAA, 0xBB, 0xCC
};

static void wifiHandler(void *args, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    switch(eventId) {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(TAG, "WiFi STA 시작 중...");
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        break;

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG, "WiFi STA 연결됨 ...");
            xEventGroupSetBits(wifiEventGroup, CONNECTED_BIT);
            retryCounts = 0;
        }   
        break;

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            ESP_LOGW(TAG, "연결 재시도... (횟수 : %d)", retryCounts);
            esp_wifi_connect();
            retryCounts++;
        }
        break;

        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t*) eventData;
            ESP_LOGI(TAG, "WiFi STA (DHCP IP : )" IPSTR,IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(wifiEventGroup, GOT_IP_BIT);
        }
        break;
        default: break;

    }
}

esp_err_t wifiInit(void) {
    esp_err_t err;
    wifiEventGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if(err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi 초기화 실패");
        return err;
    }

    wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&wifiInitConfig);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi 기본 초기화 실패");
        return err;
    }

    
    // static bool is_paired = false;
    uint8_t mac[6];

    esp_wifi_get_mac(WIFI_IF_STA, mac);

    printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiHandler, NULL, &instance_any_id);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "핸들러 등록 실패 (handler.1)");
        return err;
    }

    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiHandler, NULL, &instance_got_ip);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "핸들러 등록 실패 (handler.2)");
        return err;
    }

    wifi_config_t wifi_config = {0};    
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi 초기화 성공, 연결 대기 중");
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup, GOT_IP_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & GOT_IP_BIT) {
        ESP_LOGI(TAG, "WiFi 연결 성공, IP획득");
        return ESP_OK;
    } 
    else if (bits & FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi 연결 실패");
        return ESP_FAIL;
    } 
    else {
        ESP_LOGE(TAG, "예상치 못한 에러 발생");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void csi_callback(void *ctx, wifi_csi_info_t *data) {
    if (!data || !data->buf || csi_queue == NULL) {
        return;
    }

    const uint8_t*sender_mac = data->mac;

    if (memcmp(TX_MAC_ADDRESS, sender_mac, 6) != 0) {
        return; 
    }

    csi_packet_t packet = {0};
    packet.len = data->len;
    
    if (packet.len > 128) {
        packet.len = 128;
    }

    memcpy(packet.raw_data, data->buf, packet.len);
    xQueueSend(csi_queue, &packet, 0);
}

void csi_data_calculate(void* pvParameters) {
    csi_packet_t packet;
    
    while(1) {
        if(xQueueReceive(csi_queue, &packet, portMAX_DELAY)) {

            for(int i = 0; i+1 < packet.len; i+=2) {
                int8_t real = packet.raw_data[i];
                int8_t imaginary = packet.raw_data[i + 1];      

                float amplitude = sqrt((real * real) + (imaginary * imaginary));
                printf("%.2f,", amplitude);

                if (!baseline_is_ready()) {
                    baseline_update(amplitude);
                    continue;
                }

                float filtered_amplitude = baseline_apply(amplitude);

                printf("%.2f,", filtered_amplitude);
            }
            
            printf("\n"); 
        }
    }
}