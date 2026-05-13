# nf_ipcam 리팩토링 체크리스트

원칙: 동작 보존, 외부 API 시그니처 유지, phase 내부에서 의미 단위 commit, 각 phase 끝에 빌드 + 수동 시나리오 체크.

---

## Phase 0 — 안전망

- [x] **롤백 매체 합의 + GitHub repo 연결**: `main` 브랜치, `src/+include/+.md` 추적. 베이스라인 commit `06eacfc`, `cffa389`. (완료 2026-05-13)
- [x] **외부 API 헤더 위치 재탐색**: 실제 위치 `src/include/` (메모리 보정 완료). 상세 `docs/phase0_inventory.md` 1절. (완료 2026-05-13)
- [x] **베이스라인 메트릭 측정**: 1차 타겟 4파일 = 29,110줄 / 893KB / 456함수 / 440 static. 단일 최대 `nf_ipcam_models.c` 27,422줄. 상세 `docs/phase0_inventory.md` 3절. (완료 2026-05-13)
- [x] **모드 플래그 참조 파일 목록 확정**: 고유 16파일 (접근자 7 + sysdb 12 - 중복 3). 상세 `docs/phase0_inventory.md` 2절. (완료 2026-05-13)
- [ ] **외부 API 시그니처 인벤토리(공개 함수 목록 + 외부 호출자 매핑)**: 헤더 위치는 확정. `nf_api_ipcam.h`/`nf_api_openmode.h`의 공개 함수 목록 + `#include` 호출자 매핑 표 작성(별도 단계).
- [ ] **빌드 절차 문서화**: 개발서버 접속 → 빌드 명령 → 결과 확인 + 빌드 변형(`_IPX_32P5` / `ENABLE_PROJECT_KMW` / `USE_PND` 등 매크로) 중 검증 대상 N개 합의.
- [ ] **수동 검증 시나리오 정의**: CCTV 모드 카메라 연결 / OPEN 모드 카메라 연결 / 듀얼랜 모드 전환 / 디스커버리 1회 / 펌업·이벤트 수신 1회. 각각 통과/실패 판정 기준 명문화. 1회 수행 소요 시간 견적.
- [ ] **테스트 셋업 합의**: 회귀 시나리오를 돌릴 최소 장비/카메라 셋업(자사 1대 + 외부 1대 등)과 sysdb 초기 상태.
- [ ] Phase 0 완료 — 빌드 1회 통과 확인 (변경 없음, 베이스라인 빌드 산출물 + 로그 보존)

## Phase 1 — 명명 부채 정리

세부 작업은 phase 0 완료 후 commit 단위로 구체화. 임시 윤곽:

- [ ] 1.1 접근자 추가: `nf_get_dual_lan_mode()` 새 함수 정의(기존 `nf_get_custom_mode()` 래핑) + 헤더 노출
- [ ] 1.2 호출자 교체: `nf_get_custom_mode()` 호출 → `nf_get_dual_lan_mode()` (파일 단위로 작은 commit)
- [ ] 1.3 직접 sysdb 호출(`cam.install.dual_lan` 검사 코드) → 접근자 사용으로 단일화
- [ ] 1.4 정적 변수 캐시 정리: `is_custom_mode` 변수명 → `is_dual_lan` (정의/할당/참조 일괄)
- [ ] 1.5 구형 접근자 `nf_get_custom_mode()` 제거 + 외부 헤더 정리
- [ ] 1.6 동일하게 `is_open_mode` 측 정리(필요 시 `nf_get_install_mode()` 단일 진실 출처화 — 합의 후)
- [ ] Phase 1 완료 — 빌드 통과 + 수동 회귀 체크 (CCTV/OPEN/듀얼랜 전환)

## Phase 2 — 모드 enum 도입

세부 작업은 phase 1 완료 후 구체화. 초안:

- [ ] 2.1 enum 정의 (예: `IPCAM_MODE_CCTV` / `IPCAM_MODE_OPEN` / `IPCAM_MODE_OPEN_DUALLAN`) — 헤더 위치, 명칭, prefix 합의
- [ ] 2.2 sysdb 2-flag → enum 변환 함수 도입 (`nf_get_install_mode_enum()` 등)
- [ ] 2.3 호출자에서 2-flag 조합 검사 → enum switch로 점진 교체
- [ ] 2.4 무의미 조합(CCTV + dual_lan) 컴파일 타임/런타임 차단
- [ ] Phase 2 완료 — 빌드 + 수동 회귀

## Phase 3 — 큰 파일 도메인 split

**분할 축(도메인 우선 vs 모드 우선)은 phase 3 시작 직전 재합의.** Phase 2 결과(enum) 반영해 코드 구조를 다시 보고 판단.

세부 작업은 그때 구체화.

## Phase 4 — 드라이버 추상화

세부 작업 미정. Phase 3 결과 반영 후 작성.

## Phase 5 — 채널 FSM 명시화

세부 작업 미정.

## Phase 6 — 통합 디스커버리 풀

세부 작업 미정.

## Phase 7 — 런타임 capability negotiation

세부 작업 미정.

---

진행 규칙:
- 각 체크 항목은 1 commit 또는 1 검토 단위.
- 의미 단위로 쪼개고 통과되면 즉시 체크 + commit 메시지에 phase/항목 번호 표기.
- 합의되지 않은 항목은 작업 시작 전 별도 질문.
