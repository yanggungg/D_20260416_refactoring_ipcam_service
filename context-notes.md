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

---

(이하 phase 진행에 따라 결정 추가)
