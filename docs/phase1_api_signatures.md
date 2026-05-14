# Phase 1.0 — nf_get_custom_mode 시그니처/호출자 정밀 매핑

작성: 2026-05-14. Phase 1.1~1.4의 형태 결정 근거.

---

## 1. 정의

**위치**: [src/service/nf_api_ipcam.c:413](src/service/nf_api_ipcam.c:413)

```c
gboolean nf_get_custom_mode(void)
{
    return is_custom_mode;
}
```

- 시그니처: `gboolean nf_get_custom_mode(void)` — 인자 없음, `gboolean` 반환.
- 본문: file-scope static 변수 `is_custom_mode` 단순 반환.

## 2. `is_custom_mode` 정적 변수

같은 파일에서 정의·갱신·반환:

| 위치 | 역할 |
|---|---|
| `nf_api_ipcam.c:71` | `@var is_custom_mode` doxygen 주석 |
| `nf_api_ipcam.c:79` | `static gint is_custom_mode = 0;` — file-scope 정의 |
| `nf_api_ipcam.c:405` | `is_custom_mode = nf_sysdb_get_bool("cam.install.dual_lan");` — `nf_set_installation_mode()` 안에서 갱신 |
| `nf_api_ipcam.c:415` | `return is_custom_mode;` — `nf_get_custom_mode()` 본문 |

## 3. 공개 API 여부 — **내부 함수**

- `src/include/` 모든 헤더에서 `nf_get_custom_mode` 매치 0건.
- `nf_api_ipcam.h`에도 prototype 없음.
- 즉 **공개 API 아님**. 외부 109 호출자(`nf_api_ipcam.h` includer)에 노출 안 됨.
- → Phase 1.4(구형 접근자 제거)는 **즉시 제거 가능**, deprecation alias 불필요.

## 4. 호출자 — 18 라인 / 6 파일

| 파일 | 라인 | 컨텍스트 |
|---|---|---|
| `src/service/nf_api_openmode.c` | 565 | `if(nf_get_custom_mode()){` |
| `src/service/nf_ipcam_discovery_dhcpd.c` | 265, 566, 598, 749, 906, 937, 1034 | 7회 모두 `if(!nf_get_custom_mode())` 형태 |
| `src/service/nf_ipcam_pnd.c` | 1579 | `if(nf_get_custom_mode())` |
| `src/sysman/nf_util_netif.c` | 320, 477 | `is_custom = nf_get_custom_mode();//nf_sysdb_get_bool("cam.install.dual_lan");` — **호출자가 이미 함수의 실체를 주석으로 공개** |
| `src/sysman/nf_util_netif.c` | 820, 1163 | `if(nf_get_custom_mode()){` |
| `src/service/nf_onvif_device.c` | 2230, 2372, 2555, 3001 | 4회 모두 `if(!nf_get_custom_mode())` 형태 |

**모든 호출이 `if` 조건식 또는 단순 대입** — 인자 변환·전달 없음. rename 안전.

## 5. Prototype 부재 — implicit function declaration

- `nf_get_custom_mode`는 정의된 `nf_api_ipcam.c` **밖의 5 파일에서 호출**되지만 어떤 헤더에도 prototype 없음.
- 호출자는 C의 implicit declaration 규칙(`int nf_get_custom_mode()`)으로 동작 — `gboolean`이 `typedef int gboolean`이라 우연히 일치.
- 빌드 워닝(`-Wimplicit-function-declaration`)이 발생할 가능성이 높지만 동작 결과는 정상.
- **Phase 1.1에서 새 접근자 `nf_get_dual_lan_mode()` 도입 시 prototype 위치 결정 필요** — §7 참조.

## 6. 호출자 의도 신호

`nf_util_netif.c:320, 477`의 주석은 결정적:

```c
is_custom = nf_get_custom_mode();//nf_sysdb_get_bool("cam.install.dual_lan");
```

호출자 측에서도 이미 함수가 사실은 `dual_lan` 플래그임을 인지하고 있음. rename은 코드의 잠재 의도를 표면화하는 동작 보존 변경.

또 다른 신호: 일부 파일은 결과를 받는 로컬 변수를 `is_custom`이 아닌 `is_duallan`으로 받음(메모리 기록 — `nf_util_wpa.c`). rename 후 일관성 ↑.

## 7. Phase 1.1~1.4 형태 결정

### 7.1 1.1 새 접근자 추가

- 위치: `src/service/nf_api_ipcam.c`의 기존 `nf_get_custom_mode()` 바로 옆(:413 근처).
- 시그니처: `gboolean nf_get_dual_lan_mode(void)`
- 본문 옵션 (모두 동작 보존, 결과 동일):
  - (a) `return nf_get_custom_mode();` — 구형 호출(가장 작은 변경)
  - (b) `return is_custom_mode;` — 정적 변수 직접 접근 (구형과 동일 로직 복제)
- → **(b) 권장**: 1.4에서 구형 함수를 제거할 때 dangling 호출 없음.

**prototype 위치 옵션(별도 합의):**
- A. **현 패턴 유지(prototype 없음)** — `nf_get_custom_mode`와 똑같이 implicit declaration. 변경 최소. 위험 패턴 유지.
- B. **`src/include/nf_ipcam_defs.h`에 prototype 추가** — 깔끔. 호출자도 implicit warning 사라짐. 변경 살짝 큼.
- C. **새 내부 헤더(`nf_ipcam_install.h` 등) 신설** — 가장 깨끗하지만 새 파일 도입. Phase 3(헤더 분할)에서 자연스러움 — 지금은 과함.

### 7.2 1.2 호출자 교체

- 18 라인 / 6 파일 단순 텍스트 치환 (`nf_get_custom_mode` → `nf_get_dual_lan_mode`).
- 의미 단위 1 commit이 자연스러움(7 파일 합쳐도 의미상 단일 변경).
- 단, **`nf_util_netif.c`의 주석 `//nf_sysdb_get_bool("cam.install.dual_lan");`은 함께 정리 가능**: 새 이름이 곧 dual_lan을 의미하므로 주석 제거 또는 갱신.

### 7.3 1.3 정적 변수 rename

- `nf_api_ipcam.c` 안의 4 위치(주석, 정의, 할당, 반환) 일괄 치환:
  - `static gint is_custom_mode = 0;` → `static gint is_dual_lan = 0;`
  - `is_custom_mode = nf_sysdb_get_bool(...)` → `is_dual_lan = nf_sysdb_get_bool(...)`
  - `return is_custom_mode;` → `return is_dual_lan;` (이건 1.4에서 함수 본문 통째 교체될 수도)
  - doxygen `@var` 주석 갱신.
- 단일 파일, 작은 commit.

### 7.4 1.4 구형 함수 제거

- `nf_get_custom_mode()` 정의(`nf_api_ipcam.c:413-416`) 제거.
- 공개 API 아님 → deprecation alias 불필요, **즉시 제거**.
- 단, 1.2가 완료된 후(호출자 0이 된 상태) 진행.

## 8. 검증

- 빌드: `_IPX_32P5 + VIDECON + VA + NABTO` 변형으로 `make` 통과. implicit warning 변동만 점검.
- 회귀: **시나리오 A 듀얼랜 ON/OFF + C 라이브 영상**. 32CH에서 의미 있는 두 가지.
