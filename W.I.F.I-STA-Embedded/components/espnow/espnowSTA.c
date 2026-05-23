#include "espnowSTA.h"

// temp MAC addr
static const char *TAG = "ESPNOW-RX";

esp_err_t espnow_init_setup(void) {
    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW 초기화 실패");
        return err;
    }
    
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_LOGI(TAG, "ESP-NOW 단독 세팅 완료!");
    
    // wifi_csi_config_t csi_config = {
    //     .lltf_en           = true,
    //     .htltf_en          = true,
    //     .stbc_htltf2_en    = true,
    //     .ltf_merge_en      = true,
    //     .channel_filter_en = true,
    //     .manu_scale        = false,
    //     .shift             = false,
    // };
    
    // ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
    ESP_LOGI(TAG, "CSI 파동 수집 세팅 완료!");
    
    return ESP_OK;
}