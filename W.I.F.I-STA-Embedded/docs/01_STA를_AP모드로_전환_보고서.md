# STA 모드 → AP 모드 전환 보고서

- **대상 프로젝트**: `W.I.F.I-STA-Embedded` (ESP-IDF v5.5.4 / 타겟 esp32)
- **작업 범위**: 수신기(STA) 기기의 **WiFi 네트워킹 역할만** STA → AP로 전환
- **유지 원칙**: CSI 수신·계산·baseline 필터 등 기능 로직은 일절 변경하지 않음

---

## 1. 배경 / 목적

WiFi CSI 기반 센싱 시스템에서 두 기기의 역할(AP-송신 ↔ STA-수신)을 서로 바꾸기로 결정.
그중 **STA 기기 파트**를 담당하여, 기존 "공유기에 STA로 접속하던 동작"을
"자체 AP를 띄우는 동작"으로 전환했다. CSI 캡처는 promiscuous 모드 기반이라
AP 모드에서도 동일하게 동작하므로 수신 파이프라인은 그대로 두었다.

| 구분 | 전환 전 | 전환 후 |
|------|---------|---------|
| WiFi 모드 | `WIFI_MODE_STA` | `WIFI_MODE_AP` |
| 네트워크 동작 | 외부 공유기에 접속(DHCP IP 획득) | 자체 AP 생성, 클라이언트 접속 수신 |
| CSI 수신 기능 | 유지 | **그대로 유지** |

---

## 2. 변경 파일

| 파일 | 변경 요약 |
|------|-----------|
| `components/wifimodule/wifi.c` | `wifiInit()` / 이벤트 핸들러를 STA → AP로 전환 |
| `components/wifimodule/wifi.h` | `esp_mac.h` 인클루드 추가(MACSTR/MAC2STR 사용) |

---

## 3. 변경 상세

### 3-1. WiFi 초기화 `wifiInit()`
- `esp_netif_create_default_wifi_sta()` → **`esp_netif_create_default_wifi_ap()`**
- 접속용 `.sta` 설정 → **AP용 `.ap` 설정**으로 교체
  - SSID/PW(`WIFI_SSID`/`WIFI_PASS`), `channel = 6`, `max_connection = 2`, `authmode = WPA_WPA2_PSK`
  - 비밀번호 길이 0이면 `WIFI_AUTH_OPEN`으로 자동 전환
- `esp_wifi_set_mode(WIFI_MODE_STA)` / `WIFI_IF_STA`
  → **`WIFI_MODE_AP` / `WIFI_IF_AP`**
- IP 획득 대기 로직(`xEventGroupWaitBits(GOT_IP_BIT | FAIL_BIT ...)`) 제거
  → AP는 IP를 받지 않으므로 `esp_wifi_start()` 후 즉시 `ESP_OK` 반환
- 자기 MAC 출력 인터페이스 `WIFI_IF_STA` → **`WIFI_IF_AP`**

### 3-2. 이벤트 핸들러 `wifiHandler()`
- 제거된 STA 이벤트: `WIFI_EVENT_STA_START`(→connect), `STA_CONNECTED`,
  `STA_DISCONNECTED`(재시도 로직), `IP_EVENT_STA_GOT_IP`
- 추가된 AP 이벤트: `WIFI_EVENT_AP_START`, `WIFI_EVENT_AP_STACONNECTED`,
  `WIFI_EVENT_AP_STADISCONNECTED` (접속/해제 기기 MAC 로깅)
- `IP_EVENT` 핸들러 등록 제거
- STA 전용으로 더 이상 쓰이지 않는 `retryCounts` 변수 정리

### 3-3. 헤더 `wifi.h`
- AP 이벤트 로그의 `MACSTR` / `MAC2STR` 매크로를 위해 `#include "esp_mac.h"` 추가

---

## 4. 변경하지 않은 것 (의도적 유지)

- `csi_callback()`, `csi_data_calculate()` — CSI 수신·진폭 계산 로직
- `TX_MAC_ADDRESS` 송신기 필터 (`34:85:18:AA:BB:CC`)
- baseline 필터 호출부
- `main.c`의 실행 흐름 (promiscuous 활성, 채널 6 고정, CSI 콜백 등록/활성)
  - 채널은 AP 설정(`.ap.channel = 6`)과 `main.c`의 `esp_wifi_set_channel(6)`을 일치시켜 충돌 없음

---

## 5. 검증

- ESP-IDF v5.5.4 / esp32 대상 **빌드 성공** (종료코드 0, 관련 경고·에러 0건)
- 산출물 `build/W.I.F.I-STA-Embedded.bin` 정상 생성
- ※ 실제 보드 플래시 후 동작 검증(상대 기기와의 페어링·CSI 수신)은 별도 필요

---

## 6. 실기 검증 시 확인 포인트

- 상대 기기(AT→STA)의 **실제 MAC**과 `TX_MAC_ADDRESS` 필터 값 일치 여부
  (불일치 시 CSI가 전부 필터링되어 출력 없음)
- AP 채널(6)과 상대 기기 송신 채널 일치
- `WIFI_SSID`/`WIFI_PASS`의 의미가 "접속 대상"에서 "내가 띄우는 AP"로 바뀐 점 인지
