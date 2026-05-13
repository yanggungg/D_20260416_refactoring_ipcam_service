# Phase 0 인벤토리 — IPCAM 모듈 베이스라인 사실 자료

작성: 2026-05-13. 본 문서는 Phase 0 자동 탐색 결과를 한 곳에 모은 사실 자료. 이후 phase 결정의 근거.

---

## 1. 외부 API 헤더 위치

**메모리에 적힌 `include/nf_api_ipcam.h`는 잘못된 경로.** 실제 위치는 모두 `src/include/` 아래.

### 1.1 IPCAM 직접 관련 공개 헤더(`src/include/`)

| 헤더 | 역할 추정 |
|---|---|
| `nf_api_ipcam.h` | **IPCAM 공통 외부 API**(리팩토링 시 시그니처 보존 1순위) |
| `nf_api_openmode.h` | OPEN 모드 외부 API |
| `nf_api_cam.h` | (확인 필요 — 일반 cam API?) |
| `nf_ipcam_defs.h` | IPCAM 공용 타입/상수 |
| `nf_ipcamcbc.h` | (callback 모음으로 추정) |
| `nf_ipcam_sql_utils.h` | SQL 유틸 |
| `nf_ipcam_zmq_utils.h` | ZMQ 유틸 |

### 1.2 동급 외부 API 헤더(공개 헤더 디렉토리 자체 구성)

`src/include/`에 IPCAM 외 16개 공개 API 헤더가 함께 있음 — 호출자 측 영향 추적의 base:

`nf_api_record.h`, `nf_api_raid.h`, `nf_api_pos_eventlog.h`, `nf_api_archive.h`, `nf_api_eventlog.h`, `nf_api_dva_eventlog.h`, `nf_api_disk.h`, `nf_api_dlva.h`, `nf_api_param_fw.h`, `nf_api_param_fwver.h`, `nf_api_param_hw.h`, `nf_api_param_hw_enc.h`, `nf_api_param_app.h`, `nf_api_play.h`, `nf_api_live.h`.

### 1.3 internal 헤더(`src/service/`)

벤더 드라이버 및 내부 구현 — 리팩토링 자유도 높음:

`nf_ipcam_driver_axis.h`, `nf_ipcam_driver_sony.h`, `nf_ipcam_driver_xiongmai.h`, `nf_ipcam_driver_vivotek.h`, `nf_ipcam_driver_grundig.h`, `nf_ipcam_driver_hdpro.h`, `nf_ipcam_driver_messoa.h`, `nf_ipcam_driver_sunell.h`, `nf_ipcam_driver_s1.h`, `nf_ipcam_driver_itx.h`, `nf_ipcam_driver_itx_md5.h`, `nf_ipcam_driver_itx_ptz.h`, `nf_ipcam_hub.h`, `nf_ipcam_json_utils.h`, `nf_ipcam_utils.h`, `nf_ipcam_capabilities_parser.h`, `nf_ipcam_vabox_data_operator.h`, `nf_api_http.h`(서비스 내 위치, 좀 변칙).

---

## 2. 모드 플래그 참조 분포

전체 **고유 16개 파일**(메모리상 "약 18개"와 거의 일치).

### 2.1 접근자/정적 변수 패턴 (`is_*_mode`, `nf_get_*_mode`, `nf_set_installation_mode`)

**29회 / 7 파일**

| 파일 | 카운트 |
|---|---|
| `src/service/nf_api_ipcam.c` | 11 (진입점 `nf_set_installation_mode` 포함) |
| `src/service/nf_ipcam_discovery_dhcpd.c` | 7 |
| `src/sysman/nf_util_netif.c` | 4 |
| `src/service/nf_onvif_device.c` | 4 |
| `src/service/nf_api_openmode.c` | 1 |
| `src/service/nf_ipcam_pnd.c` | 1 |
| `src/nf_main.c` | 1 |

### 2.2 sysdb 직접 호출 (`cam.install.mode`, `cam.install.dual_lan`)

**24회 / 12 파일**

| 파일 | 카운트 |
|---|---|
| `src/nfdal/nfdal.c` | 6 |
| `src/sysman/nf_util_netif.c` | 3 |
| `src/extension/nf_onvif_server.c` | 2 |
| `src/service/nf_api_ipcam.c` | 2 |
| `src/sysman/nf_qc_app.c` | 2 |
| `src/sysman/nf_sysdb.c` | 2 |
| `src/sysman/nf_sysman.c` | 2 |
| `src/service/nf_api_openmode.c` | 1 |
| `src/service/nf_ipcam_discovery_onvif.c` | 1 |
| `src/sysman/nf_util_wpa.c` | 1 |
| `src/service/nf_network.c` | 1 |
| `src/service/nf_record.c` | 1 |

### 2.3 양쪽 모두 참조하는 파일

`nf_api_ipcam.c`, `nf_api_openmode.c`, `nf_util_netif.c` — 3개 파일에서 두 패턴이 섞임. Phase 1에서 단일 접근자로 통일 시 검사 우선순위.

---

## 3. 베이스라인 메트릭

### 3.1 리팩토링 1차 타겟 4 파일

| 파일 | 크기(KB) | 라인 수 | 함수 정의 추정 | static 선언 |
|---|---|---|---|---|
| `src/service/nf_api_ipcam.c` | 375 | 11,830 | 182 | 83 |
| `src/service/nf_api_openmode.c` | 247 | 8,644 | 158 | 270 |
| `src/service/nf_ipcam_setter.c` | 168 | 5,146 | 34 | 3 |
| `src/service/nf_ipcam_pnd.c` | 103 | 3,490 | 82 | 84 |
| **합계** | **893** | **29,110** | **456** | **440** |

> 메모리상 33K+ 줄과 차이는 측정 방식(`Measure-Object -Line`) 또는 사본 시점 차이. 실측 기준 29.1K.

### 3.2 IPCAM 관련 단일 최대 파일(메모리에 없던 발견)

- **`src/service/nf_ipcam_models.c`: 27,422줄 / 1028KB** — 모델 데이터 테이블로 보이는 단일 최대 파일. 1차 타겟 4 파일을 합한 것에 근접.
- **`src/service/nf_ipcam_driver_itx.c`: 23,961줄 / 718KB** — 자사 카메라 드라이버.

이 두 파일은 1차 타겟 외였지만 Phase 3(도메인 split) 또는 Phase 4(드라이버 추상화) 시 직접 영향 범위에 들어옴.

### 3.3 IPCAM 관련 2,000줄 이상 파일(참고)

```
27422  nf_ipcam_models.c
23961  nf_ipcam_driver_itx.c
11830  nf_api_ipcam.c            *
 8644  nf_api_openmode.c         *
 6865  nf_ipcam_setup.c
 5146  nf_ipcam_setter.c         *
 5259  nf_onvif_media.c
 3490  nf_ipcam_pnd.c            *
 3703  nf_onvif_device.c
 3536  nf_ipcam_driver_itx_openmode.c
 2802  nf_ipcam_hub.c
 2942  nf_ipcam_driver_itx_ptz.c
 2382  nf_onvif_ptz.c
 2164  nf_ipcam_driver_grundig.c
 2211  nf_ipcam_driver_s1.c
 2111  nf_ipcam_discovery_onvif.c
```
(`*` = 1차 타겟)

---

## 4. 본 인벤토리가 phase 결정에 미치는 영향

1. **외부 API 헤더 위치 보정** — 메모리/이후 문서에서 `include/` → `src/include/`. 외부 호출자 검사 시 `#include "nf_api_ipcam.h"` 또는 `nf_api_openmode.h` 패턴 기반.
2. **Phase 1 변경 범위 = 16 파일** 확정. 도메인별 분포: service 6 / sysman 5 / nfdal 1 / extension 1 / 루트 1 / 그 외(discovery, onvif 등).
3. **Phase 3 분할 시 nf_ipcam_models.c·nf_ipcam_driver_itx.c도 시야 안에** 둘 것. 메모리상 1차 타겟 4 파일만으로는 모듈의 절반을 놓침.
4. **`nf_api_openmode.c`의 static 270개**가 비정상적으로 많음 — 내부 상태 캐시/헬퍼가 무거움. Phase 1·2 시 모드 정리 후 도메인 split 시 가장 큰 구조 변화 후보.
