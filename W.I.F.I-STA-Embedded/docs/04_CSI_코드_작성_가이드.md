# CSI 코드 작성 가이드 (AT = 송신 / STA = 수신)

- **대상**: ESP-IDF v5.5.x, ESP32
- **목적**: WiFi CSI를 직접 구현/수정할 때 "어느 쪽에서 무엇을, 어떤 순서로" 짜야 하는지
  정리한 실무 가이드. 본 프로젝트의 실제 코드(`espnow/espnowAP.c` 송신,
  `wifimodule/wifi.c` 수신) 패턴 기준.

---

## 0. CSI가 동작하는 원리 (먼저 이해할 것)

CSI(Channel State Information)는 **수신기의 WiFi 하드웨어가 받은 패킷으로부터 채널 상태를
추정**해서 만들어진다. 따라서 두 역할이 필요하다.

```
[송신기 = AT 보드]  ──(WiFi 패킷을 계속 흘림)──►  [수신기 = STA 보드]
   - 패킷만 주기적으로 송신                          - CSI 캡처 활성화
   - CSI 설정 안 함                                  - 콜백에서 CSI 원시데이터 수신 → 파싱
```

> ⚠️ **CSI의 송신/수신 역할은 WiFi의 AP/STA 모드와 별개다.**
> 송신기가 AP든 STA든, 수신기가 AP든 STA든 상관없이 CSI는 동작한다(아래 §1-3).
> "AT/STA"는 본 프로젝트의 보드 이름일 뿐, CSI 역할은 **AT=송신 / STA=수신**으로 본다.

---

## 1. 공통 전제 (양쪽 모두 해당)

### 1-1. sdkconfig에서 CSI 기능 켜기 (수신측 필수)
```
CONFIG_ESP_WIFI_CSI_ENABLED=y
CONFIG_ESP32_WIFI_CSI_ENABLED=y     # esp32 타겟
```
- 안 켜면 `esp_wifi_set_csi(true)`가 `ESP_ERR_NOT_SUPPORTED`를 반환하고,
  `ESP_ERROR_CHECK`로 감싸면 **부팅 즉시 패닉**한다.
- 설정 방법: `idf.py menuconfig` → *Component config → Wi-Fi → WiFi CSI(Channel State Information)*
  체크, 또는 sdkconfig 직접 편집.

### 1-2. 송신기와 수신기는 반드시 **같은 채널**
- CSI는 같은 채널에 있어야만 잡힌다. 본 프로젝트는 채널 **6** 사용.
- 수신측은 `esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE)`,
  송신측은 자신의 운영 채널(AP면 `.ap.channel=6`, STA면 접속 AP의 채널)을 6으로 맞춘다.

### 1-3. WiFi 모드는 자유
- 송신기는 패킷만 흘리면 되므로 AP/STA 무엇이든 가능.
- 수신기도 promiscuous로 캡처하므로 AP/STA 무엇이든 가능.

---

## 2. AT 보드 (CSI **송신** 측) 작성법

**할 일**: 수신기가 CSI를 측정할 수 있도록 **일정 주기로 패킷을 송신**한다.
ESP-NOW가 가볍고 연결 없이 바로 쏠 수 있어 CSI 송신원으로 적합하다.

### 작성 순서
1. WiFi 초기화 (`esp_wifi_init` → `esp_wifi_set_mode` → `esp_wifi_start`)
2. ESP-NOW 초기화 (`esp_now_init`)
3. 송신 대상 peer 등록 (`esp_now_add_peer`) — 보통 브로드캐스트
4. 주기적으로 `esp_now_send` 하는 태스크 생성

### 코드 스니펫
```c
#include "esp_now.h"
#include "esp_wifi.h"

static const uint8_t PEER_MAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // 브로드캐스트

void espnow_tx_setup(void) {
    ESP_ERROR_CHECK(esp_now_init());

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, PEER_MAC, 6);
    peer.channel = 0;            // 0 = 현재 WiFi 채널 사용 (수신기와 동일 채널)
    peer.ifidx   = WIFI_IF_AP;   // 이 보드의 활성 인터페이스 (STA면 WIFI_IF_STA)
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
}

// 주기 송신 태스크: 패킷이 곧 "CSI 측정용 신호"가 된다
void espnow_csi_send(void *arg) {
    uint8_t payload = 0;
    while (1) {
        esp_now_send(PEER_MAC, &payload, sizeof(payload));
        vTaskDelay(pdMS_TO_TICKS(20));   // 50Hz. 샘플레이트는 여기서 조절
    }
}
```

### 송신측 핵심 포인트
- **CSI 관련 API는 송신측에서 호출하지 않는다.** 패킷만 흘리면 됨.
- **송신 주기 = CSI 샘플레이트.** 20ms면 약 50Hz. 너무 빠르면 부하·드롭, 너무 느리면
  움직임 해상도 저하. 보통 20~100ms.
- 페이로드 내용은 CSI 측정엔 무관(빈 패킷도 OK). 다른 정보를 같이 실어보내도 됨.

---

## 3. STA= 보드 (CSI **수신** 측) 작성법

**할 일**: CSI 캡처를 켜고, 콜백에서 원시 CSI를 받아 진폭(amplitude)으로 변환한다.

### 작성 순서 (순서 중요)
1. WiFi 초기화 + start
2. `esp_wifi_set_promiscuous(true)` — CSI는 promiscuous에서 캡처
3. `esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE)` — 송신기와 동일 채널
4. `esp_wifi_set_csi_rx_cb(콜백, ctx)` — 콜백 등록
5. `esp_wifi_set_csi(true)` — CSI 활성화

### 메인 초기화 코드
```c
ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));
ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&csi_callback, NULL));
ESP_ERROR_CHECK(esp_wifi_set_csi(true));
```

### 콜백 — **가볍게**, 데이터만 큐로 넘김
```c
typedef struct { uint8_t len; int8_t raw[128]; } csi_packet_t;
QueueHandle_t csi_queue;  // xQueueCreate(10, sizeof(csi_packet_t))

static const uint8_t TX_MAC[6] = {0x34,0x85,0x18,0xAA,0xBB,0xCC}; // 송신기 MAC

void csi_callback(void *ctx, wifi_csi_info_t *data) {
    if (!data || !data->buf || csi_queue == NULL) return;

    // 의도한 송신기의 패킷만 처리 (다른 트래픽 CSI는 버림)
    if (memcmp(TX_MAC, data->mac, 6) != 0) return;

    csi_packet_t pkt = {0};
    pkt.len = (data->len > 128) ? 128 : data->len;
    memcpy(pkt.raw, data->buf, pkt.len);
    xQueueSend(csi_queue, &pkt, 0);   // 처리는 별도 태스크에서
}
```

### 처리 태스크 — 원시 CSI → 진폭
```c
void csi_data_calculate(void *arg) {
    csi_packet_t pkt;
    while (1) {
        if (xQueueReceive(csi_queue, &pkt, portMAX_DELAY)) {
            // buf는 [im, re, im, re, ...] 형태의 부호있는 바이트 쌍
            for (int i = 0; i + 1 < pkt.len; i += 2) {
                int8_t real = pkt.raw[i];
                int8_t imag = pkt.raw[i + 1];
                float amplitude = sqrtf((float)(real*real + imag*imag));
                // 여기서 baseline 필터/AI 추론 등으로 전달
                printf("%.2f,", amplitude);
            }
            printf("\n");
        }
    }
}
```

### 수신측 핵심 포인트
- **TX MAC 필터**: 송신기 실제 MAC과 일치해야 데이터가 들어온다. 불일치 시 출력 0.
  (송신기 부팅 시 자기 MAC을 시리얼로 찍어두고 그 값을 넣는다.)
- **콜백은 WiFi 태스크 컨텍스트**에서 돈다 → 무거운 연산/`printf` 금지, 큐로만 넘긴다.
- **버퍼 상한**: `data->len`이 가변이라 고정 버퍼(여기선 128)로 자른다.
- 한 패킷의 buf = 서브캐리어별 복소수(I/Q) 나열. 쌍으로 묶어 |amp|=√(re²+im²) 계산.
  위상이 필요하면 `atan2f(imag, real)`.

---

## 4. CSI 원시 데이터 구조 (참고)

`wifi_csi_info_t`의 주요 필드:
| 필드 | 의미 |
|------|------|
| `mac[6]` | 이 CSI를 만든 패킷의 송신자 MAC (→ 필터 기준) |
| `buf` | CSI 원시 바이트 (서브캐리어별 I/Q 쌍, 부호있는 int8) |
| `len` | `buf` 길이(바이트). 대역폭/HT 모드에 따라 가변 |
| `rx_ctrl` | RSSI, rate, channel 등 부가 정보(`data->rx_ctrl.rssi` 등) |

- I/Q 쌍의 순서(im,re vs re,im)는 자료/칩에 따라 표기가 갈린다. **진폭(√(I²+Q²))은 순서와
  무관**하므로 진폭만 쓰면 안전하다. 위상까지 쓰면 순서를 한번 검증할 것.

---

## 5. 흔한 실수 체크리스트

| 증상 | 원인 | 조치 |
|------|------|------|
| 부팅 즉시 패닉 | sdkconfig CSI 미활성 | `CONFIG_..._CSI_ENABLED=y` |
| 콜백이 아예 안 불림 | 채널 불일치 / promiscuous 미설정 | 같은 채널 + `set_promiscuous(true)` |
| 콜백은 오는데 출력 0 | TX MAC 필터 불일치 | 송신기 실제 MAC으로 수정 |
| 간헐 리셋/WDT | 콜백에서 무거운 작업 | 콜백은 큐 전송만, 계산은 태스크에서 |
| 값이 거의 안 변함 | 송신 주기 과대 / 송신 안 됨 | 송신 주기 단축, 송신 로그 확인 |

---

## 6. 동작 검증 방법

1. **송신기**: 부팅 로그에서 송신 태스크가 도는지, 자기 MAC 확인.
2. **수신기**: 시리얼 모니터(`idf.py monitor`)에서 amplitude가 줄줄이 찍히는지 확인.
3. 송신기-수신기 사이에서 **손을 움직이면 amplitude 값이 흔들리면** CSI가 정상 동작하는 것.
4. 값이 안정되면 baseline 필터/낙상 AI 입력으로 연결.

---

## 부록. 송신/수신 역할 요약

| | AT 보드 (송신) | STA 보드 (수신) |
|---|---|---|
| 핵심 목적 | 패킷 흘리기 | CSI 캡처·파싱 |
| CSI API | **호출 안 함** | `set_csi_rx_cb` + `set_csi(true)` |
| promiscuous | 불필요 | **필요** |
| 채널 | 6 (수신기와 동일) | 6 (`set_channel`) |
| 주요 코드 | `esp_now_send` 주기 송신 | `csi_callback` + 처리 태스크 |
| sdkconfig CSI | 불필요 | **필수** |
