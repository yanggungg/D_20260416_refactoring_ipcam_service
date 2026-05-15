# nf_ipcam 리팩토링 컨텍스트 노트

작업 기간: 2026-05 ~ (진행 중). 본 문서는 결정과 그 근거를 누적 기록한다.

---

## 1. 목적 (확정)

`nf_ipcam_init()`을 진입점으로 하는 IPCAM 모듈 전체의 **동작 보존 리팩토링**.

- 1차 목표: 기능별 응집도 향상 + 파일 크기 축소.
- 비-목표: 기능 추가, 동작 변경, 버그 수정.

대상 파일(분량):
- `src/service/nf_api_ipcam.c` 13,451 줄
- `src/service/nf_api_openmode.c` 9,964 줄
- `src/service/nf_ipcam_setter.c` 5,949 줄
- `src/service/nf_ipcam_pnd.c` 3,992 줄
- 합계 33K+ 줄.

## 2. 아키텍처 방향 (확정, 2026-05-13)

현대 NVR/VMS 표준 5원칙 채택. 점진 마이그레이션.

1. 모드 분류축 변경: 인터페이스(내부 PoE/외부 LAN) × 디스커버리 프로토콜 직교축.
2. 카메라 드라이버 추상화 인터페이스 도입(connect/disconnect/get_caps/...).
3. Capability 런타임 negotiation(컴파일 타임 매크로 의존 축소).
4. 채널 상태 머신 명시화(흩어진 플래그 → FSM).
5. 통합 디스커버리 풀(검색 ≠ 할당).

**현실 제약**: 외부 API(`nf_api_ipcam.h`) 시그니처 가능한 유지(18+ 외부 호출자).

## 3. 분류 2축 (확정)

- **세로축(도메인)**: 검색 / 채널 할당 / 연결·유지 / 설정 / 펌웨어 / 이벤트 / 라이프사이클 / 유틸
- **가로축(모드)**: CCTV(PnD) / OPEN / 자사 직결 / 외부 벤더(ONVIF·드라이버)

4개 도메인 카테고리만으론 33K 줄 분류에 부족.

## 4. 개발 환경 (확정, 2026-05-13)

- 편집: 로컬 Windows에서 NFS 마운트(`X:\filesys_ipxp5\`).
- 빌드: 개발서버에서만 가능. 로컬 컴파일 환경 없음.
- 자동 테스트 없음. CI 없음.
- 회귀 검증: **수동 시나리오 체크만 가능**.

→ 함의: phase를 작게 쪼개야 수동 검증 부담이 작아짐.

## 5. Phase 분해 (확정, 2026-05-13)

| Phase | 내용 | 위험 |
|---|---|---|
| 0 | 안전망 — 외부 API 인벤토리, 수동 검증 시나리오 정의, 빌드 절차 확인 | 없음 |
| 1 | 명명 부채 정리 — `is_custom_mode` → `is_dual_lan`, 모드 접근자 단일화 | 저 |
| 2 | 모드 enum 도입 — 2 bool → enum(CCTV/OPEN/OPEN_DUALLAN) | 저~중 |
| 3 | 큰 파일 도메인 split — **분할 축은 phase 3 직전에 재합의** | 중 |
| 4 | 드라이버 추상화 — 공통 IF, itx_cam_*/ONVIF/벤더 어댑터화 | 중~고 |
| 5 | 채널 FSM 명시화 — 흩어진 플래그 → 명시 FSM, 3중 recovery 통합 | 고 |
| 6 | 통합 디스커버리 풀 | 고 |
| 7 | 런타임 capability negotiation — 컴파일 매크로 의존 축소 | 고 |

## 6. 결정의 근거(요약)

- **Phase 1과 2 분리**: 검증 부담 분산. 문제 발생 시 원인 분리 용이.
- **Phase 3 분할 축 보류**: 도메인 우선 / 모드 우선의 trade-off를 phase 3 직전 코드 구조를 다시 보고 결정. 지금 결정해도 phase 2 결과(enum)가 분할 모양에 영향을 줌.
- **Commit 단위는 phase 내부에서 잘게**: CLAUDE.md 룰 9 semantic commit 준수. 한 줄 요약 가능한 의미 단위 = 1 commit. 롤백 자유도 확보.
- **외부 API 시그니처 유지**: 18+ 외부 호출자(녹화/UI/ONVIF/sysman) 보호. 내부 헤더 분리는 자유.

## 7. 명명 부채 메모(Phase 1에서 정리)

- `is_custom_mode` ← 실제로는 `cam.install.dual_lan` (의미 불일치)
- `nf_get_custom_mode()` → `nf_get_dual_lan_mode()` 후보
- 모드 검사를 직접 sysdb 호출 / 정적 변수 캐시 / 접근자 호출 3가지 경로로 함 → 단일 진실 출처(접근자만)로 통일
- 약 18개 파일 영향, 변경 패턴 단순(이름 치환).

---

## 8. Phase 0 자동 탐색 결과 반영 (2026-05-13)

`docs/phase0_inventory.md` 작성. 핵심 보정:

- **외부 API 헤더 경로**: 메모리상 `include/nf_api_ipcam.h` → **실제 `src/include/nf_api_ipcam.h`**. 모든 `nf_api_*.h` 공개 헤더가 `src/include/`에 있음. 이후 phase 결정 시 이 경로 기준.
- **모드 플래그 참조 16 파일 확정**: 메모리 "약 18" → 실측 16. 분포: service 6 / sysman 5 / nfdal 1 / extension 1 / nf_main 1.
- **1차 타겟 4 파일 실측**: 29,110줄 (메모리 33K+ 보다 작음). 측정 방식 차이로 보임. 정량 비교 기준은 본 실측치.
- **Phase 3 시야 확대 필요**: `nf_ipcam_models.c`(27,422줄)와 `nf_ipcam_driver_itx.c`(23,961줄)가 1차 타겟 외였지만 합쳐 51K줄. 도메인 split 시 함께 검토.
- **`nf_api_openmode.c`의 static 270개**가 비정상적으로 많음 → 모드 정리 후 내부 상태 캐시/헬퍼 응집 재검토 필요.
- **공개 API 표면 확장 발견**: `nf_api_ipcam.h` 호출자 109 파일 — 메모리 "18+"는 모드 플래그 기준이었음. 시그니처 보존이 깨지면 UI(nfgui 59)부터 무너짐. Phase별 빌드 검증 1차 대상에 nfgui 포함 필수.

## 9. 빌드 환경 합의(2026-05-13)

- **빌드 명령**: 풀 빌드 `src/make_host.sh`(대화형), 증분 빌드 `src/make`. 둘 다 개발서버에서 실행.
- **검증 변형**: `_IPX_32P5 + VIDECON + VA + NABTO` 단일. 다른 모델/벤더 변형은 본 리팩토링 검증 밖.
- **결과물**: `nf_host`(메인), `webra.made`(UI). `nf_host`는 빌드 후 작업 폴더 루트로 자동 cp.
- **매크로 의존 인사이트**: 코드 분기 핫스팟은 모델 매크로(`_IPX_32P5` 등)가 아니라 디버그 출력(`PRINT_HTTP_API_SEND` 384회), UI 채널 수(`GUI_*CH_SUPPORT`), 기능 토글(`USE_PROXY_SYSTEM`, `SUPPORT_VCA_CAMERA`)에 있음. → Phase 7 "런타임 capability"는 단순 모델 매크로 제거가 아니라 기능 토글까지 데이터 기반화해야 의미.

## 10. 32CH 변형의 검증 실효 범위(2026-05-13)

- 사용자 확인: **`_IPX_32P5`는 CCTV 모드를 지원하지 않음**. Makefile의 `-DNO_PND=1`이 PnD 비활성. 코드는 컴파일되지만 실행 경로엔 없음.
- 검증 가능 모드 = OPEN 단일랜 + OPEN+듀얼랜.
- → 회귀 시나리오의 "모드 전환"은 **듀얼랜 ON/OFF**로 한정. CCTV ↔ OPEN 전환은 불가.
- Phase 1·2(rename·enum) 작업 시 CCTV 분기 코드는 **빌드만 보장**, 동작 회귀 검증 불가능. 빌드 통과 + 정적 분석으로 만족.

## 11. 회귀 시나리오 정의(2026-05-13 합의)

- 4종 시나리오 채택: A 듀얼랜 ON/OFF, B 디스커버리(외부 ONVIF + 내부 PoE), C 라이브 영상 1채널(자사/외부 각각), D 카메라 재연결.
- phase별 필수/선택 분배는 `docs/phase0_inventory.md` 6.3절.
- 정밀 임계값(시간/개수)은 phase 진행 중 보정. 본 시점은 수행 단계 + 통과 신호 초안만 합의.
- 한 phase 검증 부담 추정 10-20분.

## 12. Phase 0 종결(2026-05-14)

- 베이스라인 빌드(`make_host.sh`, _IPX_32P5 + VIDECON + VA + NABTO) 통과 확인. trunk_32 사본 + .gitignore 추가만 있는 상태가 정상 빌드됨을 검증.
- Phase 0 항목 9/9 완료. GitHub `main` 브랜치 baseline 7 commits 푸시 완료.
- 다음: Phase 1(`is_custom_mode` → `is_dual_lan` rename + 모드 접근자 단일화) 진입 합의 대기.

## 13. Phase 1 세부 합의(2026-05-14)

사용자 결정으로 Phase 1 형태가 정해짐.

- **범위 = A. 단순 rename만**: 정적 캐시/sysdb 직접 호출 등 3경로 단일화는 본 phase 밖. 동작 보존 우선.
- **is_open_mode 측 정리는 Phase 1.5로 분리**: Phase 1 완료/회귀 통과 후. 변경 범위 2배 방지, 회귀 원인 분리 용이.
- **단일 진실 출처화(B 안)는 미정**: Phase 2 진입 시 재합의 대상. 일단 phase plan 밖.
- **Phase 1.0 시그니처 인벤토리 추가**: `nf_get_custom_mode()`가 공개 API(`nf_api_ipcam.h`)인지 내부인지에 따라 1.1·1.4 형태 달라짐. 인벤토리 결과를 토대로 다음 단계 형태 결정.
- **시나리오 보정**: Phase 1 회귀는 **시나리오 A(듀얼랜 ON/OFF) + C(라이브 영상)** 두 가지. 메모리상 "CCTV 전환"은 32CH에서 실행 불가.
- **commit 단위**: 1.2 호출자 일괄 교체는 의미 단위 1 commit 기본. 디렉토리 분할은 1.0 결과 보고 결정(7 파일이 너무 크면 service/sysman/nf_main 3 commit).

## 14. Phase 1.0 결과(2026-05-14)

- 정의: `src/service/nf_api_ipcam.c:413` 단일. 시그니처 `gboolean nf_get_custom_mode(void)`.
- 정적 변수 `is_custom_mode` 4위치(`nf_api_ipcam.c` 71/79/405/415) — 단일 파일.
- **공개 API 아님**: `src/include/` 어느 헤더에도 prototype 없음. 외부 109 호출자 영향 0.
- **호출자 18라인 / 6 파일**: 모두 `if(!?nf_get_custom_mode())` 또는 단순 대입. 인자 변환 없음.
- **Prototype 부재** = C implicit declaration. 호출자에 일관 warning 가능성. → 1.1 시 prototype 위치 결정 필요(별도 합의 진행).
- **결정적 신호**: `nf_util_netif.c:320, 477`의 호출자 주석이 `//nf_sysdb_get_bool("cam.install.dual_lan");` — 코드베이스 내에서 이미 함수의 실체가 dual_lan임을 알고 있음. rename은 잠재 의도의 표면화.
- 1 commit 단위 적정성: 18라인/6파일 작은 규모. 의미 단위 1 commit으로 충분.
- 1.4 구형 제거: 공개 API 아니므로 **즉시 제거 가능**(deprecation alias 불필요).

## 15. Phase 1 코드 작업 완료(2026-05-15)

4단계 commits 적용:
- `72ffa64` 1.1 새 접근자 `nf_get_dual_lan_mode()` 추가 (prototype 없음, A 옵션).
- `e1773db` 1.2 호출자 17곳 일괄 교체 (5 파일 +17/-17).
- `d9e0e5c` 1.3 정적 변수 `is_custom_mode` → `is_dual_lan` + doxygen 의미 정정.
- 본 commit 1.4 구형 `nf_get_custom_mode()` 정의 제거.

C 코드 내 `nf_get_custom_mode` 매치 0, `nf_get_dual_lan_mode` 매치 18(정의 1 + 호출자 17). 동작 영향 0 (이름과 doxygen만 변경).

다음: 사용자가 `make`로 _IPX_32P5 변형 빌드 + 시나리오 A 듀얼랜 ON/OFF + C 라이브 영상 회귀 확인. 통과 시 Phase 1 종결, Phase 1.5(is_open_mode 정리) 진입.

---

(이하 phase 진행에 따라 결정 추가)
