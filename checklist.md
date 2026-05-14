# nf_ipcam 리팩토링 체크리스트

원칙: 동작 보존, 외부 API 시그니처 유지, phase 내부에서 의미 단위 commit, 각 phase 끝에 빌드 + 수동 시나리오 체크.

---

## Phase 0 — 안전망

- [x] **롤백 매체 합의 + GitHub repo 연결**: `main` 브랜치, `src/+include/+.md` 추적. 베이스라인 commit `06eacfc`, `cffa389`. (완료 2026-05-13)
- [x] **외부 API 헤더 위치 재탐색**: 실제 위치 `src/include/` (메모리 보정 완료). 상세 `docs/phase0_inventory.md` 1절. (완료 2026-05-13)
- [x] **베이스라인 메트릭 측정**: 1차 타겟 4파일 = 29,110줄 / 893KB / 456함수 / 440 static. 단일 최대 `nf_ipcam_models.c` 27,422줄. 상세 `docs/phase0_inventory.md` 3절. (완료 2026-05-13)
- [x] **모드 플래그 참조 파일 목록 확정**: 고유 16파일 (접근자 7 + sysdb 12 - 중복 3). 상세 `docs/phase0_inventory.md` 2절. (완료 2026-05-13)
- [x] **공개 API 표면 + 호출자 분포(개략)**: `nf_api_ipcam.h` 2,496줄/~100 proto/109 호출자, `nf_api_openmode.h` 210줄/~34 proto/19 호출자. nfgui 59가 가장 큰 호출자. 상세 `docs/phase0_inventory.md` 4절. 정확 시그니처 추출은 Phase 1 진입 직전. (완료 2026-05-13)
- [x] **빌드 절차 + 변형 합의**: 풀 빌드 `src/make_host.sh`, 증분 빌드 `src/make`. 검증 변형 = `_IPX_32P5 + VIDECON + VA + NABTO` 단일. 상세 `docs/phase0_inventory.md` 5절. (완료 2026-05-13)
- [x] **수동 검증 시나리오 정의**: 32CH 실효 범위 기준 4종 — A 듀얼랜 ON/OFF, B 디스커버리, C 라이브 영상, D 재연결. phase별 필수/선택 분배. 상세 `docs/phase0_inventory.md` 6절. (완료 2026-05-13)
- [x] **테스트 셋업 합의**: 자사 N대(PoE) + 외부 1대. CCTV 모드는 32CH 변형에서 미지원. 상세 `docs/phase0_inventory.md` 6.1절. (완료 2026-05-13)
- [x] **Phase 0 완료 — 베이스라인 빌드 1회 통과 확인**: 사용자가 개발서버에서 `make_host.sh`로 빌드 통과 확인. trunk_32 사본 + .gitignore 추가만 있는 베이스라인 상태가 빌드 가능함을 검증. (완료 2026-05-14)

> **Phase 0 종결.** 안전망 9/9, GitHub 베이스라인 7 commits 푸시 완료. Phase 1 진입 가능.

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
