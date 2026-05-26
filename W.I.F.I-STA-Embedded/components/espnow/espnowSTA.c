#include "espnowSTA.h"

#define PAIRING_REQUEST 1
#define PAIRING_RESPONSE 2

// temp MAC addr
static const char *TAG = "ESPNOW-RX";

static const uint8_t BROADCAST_MAC[6] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static uint8_t paired_rx_mac[6] = {0};
static bool is_paired = false;

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

    ESP_ERROR_CHECK(espnow_add_peer(BROADCAST_MAC));
    
    return ESP_OK;
}

esp_err_t espnow_add_peer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) {
        return ESP_OK;
    }
    
    esp_now_peer_info_t peer = {0};

    memcpy(peer.peer_addr, mac, 6);

    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    return esp_now_add_peer(&peer);
}

esp_err_t espnow_send_pairing_request() {
    uint8_t packet = PAIRING_REQUEST;

    return esp_now_send(BROADCAST_MAC, (uint8_t *)&packet, sizeof(packet));
}

void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len) {
    if (!recv_info || !data || data_len != sizeof(uint8_t)) {
        return;
    }

    uint8_t packet_type = data[0];

    if (packet_type == PAIRING_RESPONSE) {
        memcpy(paired_rx_mac, recv_info->src_addr, 6);

        esp_err_t add_err = espnow_add_peer(paired_rx_mac);
        if (add_err != ESP_OK) {
            ESP_LOGE(TAG, "RX 피어 등록 실패 : %s", esp_err_to_name(add_err));
            return;
        }
        
        is_paired = true;

        ESP_LOGI(TAG, "RX 페어링 완료");
    }
}