#include "wifi.h"
#include "baseline_filter.h"
#include <math.h>

static const char* TAG = "WiFi";
QueueHandle_t csi_queue;

// AT(STA) 접속 여부 — wifiHandler 가 갱신, wait_at_task 가 대기 로그 출력에 사용.
volatile bool at_connected = false;

//printf된 TX MAC ADDRESS 값 삽입
static const uint8_t TX_MAC_ADDRESS[6] = {
    0x78, 0x1C, 0x3C, 0xF4, 0xAF, 0xF4
};

static void wifiHandler(void *args, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    if (eventId == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "WiFi AP모드 시작");
    }
    else if (eventId == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) eventData;
        at_connected = true;
        ESP_LOGI(TAG, "장치 접속됨 MAC: " MACSTR ", AID: %d", MAC2STR(event->mac), event->aid);
    }
    else if (eventId == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) eventData;
        at_connected = false;
        ESP_LOGI(TAG, "장치 연결 끊김 MAC: " MACSTR ", AID: %d", MAC2STR(event->mac), event->aid);
    }
}

esp_err_t wifiInit(void) {
    esp_err_t err;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

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

    esp_wifi_get_mac(WIFI_IF_AP, mac);

    printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    
    esp_event_handler_instance_t instance_any_id;

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiHandler, NULL, &instance_any_id);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "핸들러 등록 실패 (handler.1)");
        return err;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 6,
            .password = WIFI_PASS,
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi(AP) 초기화 성공");
    return ESP_OK;
}

void csi_callback(void *ctx, wifi_csi_info_t *data) {
    if (!data || !data->buf || csi_queue == NULL) {
        return;
    }

    // 빈/비정상 길이 패킷(amplitude 한 쌍도 못 만드는 것) 드롭 — 빈 값 방지
    if (data->len < 2) {
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

            // 유효 데이터가 없으면 빈 줄을 찍지 않고 버린다
            if (packet.len < 2) {
                continue;
            }

            for(int i = 0; i+1 < packet.len; i+=2) {
                int8_t real = packet.raw_data[i];
                int8_t imaginary = packet.raw_data[i + 1];      

                float amplitude = sqrt((real * real) + (imaginary * imaginary));

                if (!baseline_is_ready()) {
                    baseline_update(amplitude);
                    printf("%.2f,", amplitude);
                } else {
                    float filtered_amplitude = baseline_apply(amplitude);
                    printf("%.2f,", filtered_amplitude);
                }
            }
            
            printf("\n");
        }
    }
}

void wait_at_task(void* pvParameters) {
    while (1) {
        if (!at_connected) {
            ESP_LOGI(TAG, "AT 연결 대기 중...");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}