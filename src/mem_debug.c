/*
 * mem_debug.c
 * - 메모리 누수 탐지를 위한 Wrapper 함수 구현부
 * - 다양한 할당 함수(malloc, imalloc, g_malloc, soap_malloc)를 통합 추적
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem_debug.h" 

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef g_malloc
#undef g_malloc0
#undef g_free
#undef soap_malloc


#define MAX_TRACK_ALLOCS 2048

typedef struct {
    void*       ptr;        /* 할당된 메모리 주소 */
    size_t      size;       /* 할당 크기 */
    const char* file;       /* 할당된 파일명 (__FILE__) */
    int         line;       /* 할당된 라인 (__LINE__) */
    const char* type;       /* 할당 타입 ("MALLOC", "IMALLOC", "GLIB", "SOAP") */
    int         active;     /* 1=사용중, 0=비어있음 */
} AllocInfo;

static AllocInfo tracker[MAX_TRACK_ALLOCS];
static int       tracker_initialized = 0;

/* 초기화 함수 (최초 1회 실행) */
static void init_tracker(void) {
    if (!tracker_initialized) {
        printf("[MEM_DEBUG] Initializing memory tracker...\n");
        memset(tracker, 0, sizeof(tracker));
        tracker_initialized = 1;
    }
}

/* 
 * [내부 함수] 추적 리스트에 기록 추가 
 */
static void add_track(void* ptr, size_t size, const char* file, int line, const char* type) {
    if (!ptr) return;
    if (!tracker_initialized) init_tracker();

    /* 빈 슬롯 검색 */
    for (int i = 0; i < MAX_TRACK_ALLOCS; i++) {
        if (tracker[i].active == 0) {
            tracker[i].ptr    = ptr;
            tracker[i].size   = size;
            tracker[i].file   = file;
            tracker[i].line   = line;
            tracker[i].type   = type;
            tracker[i].active = 1;
            //printf("[MEM_DEBUG] Tracked %s of size %zu at %p (%s:%d)\n", type, size, ptr, file, line);
            return;
        }
    }
    
    /* 슬롯이 꽉 찼을 경우: 치명적 에러보다는 로그만 남김 */
    // printf("[MEM_DEBUG] Tracker Full! Could not track allocation at %p (%s)\n", ptr, type);
}

/* 
 * [내부 함수] 추적 리스트에서 기록 삭제 
 */
static void remove_track(void* ptr) {
    if (!ptr) return;
    if (!tracker_initialized) return;

    for (int i = 0; i < MAX_TRACK_ALLOCS; i++) {
        if (tracker[i].active && tracker[i].ptr == ptr) {
            tracker[i].active = 0;  /* 슬롯 비움 */
            tracker[i].ptr    = NULL;
            tracker[i].size   = 0;
            //printf("[MEM_DEBUG] Untracked memory at %p\n", ptr);
            return;
        }
    }
    
    /* 
     * 여기서 찾지 못했다면?
     * 1. 래퍼가 적용되지 않은 코드에서 할당된 메모리를 해제하려는 경우
     * 2. 이미 해제된 메모리를 또 해제하려는 경우 (Double Free)
     * 이 경우는 조용히 넘어갑니다.
     */
}


/* ----------------------------------------------------------------------------------
   Wrapper 함수 구현 (실제 할당/해제 가로채기)
   ---------------------------------------------------------------------------------- */

/* 1. 표준 C malloc 계열 */
void* my_malloc(size_t size, const char* file, int line) {
    void* p = malloc(size);
    printf("[my_malloc] Allocated %zu bytes at %p (%s:%d)\n", size, p, file, line);
    add_track(p, size, file, line, "MALLOC");
    return p;
}

void* my_calloc(size_t num, size_t size, const char* file, int line) {
    void* p = calloc(num, size);
    printf("[my_calloc] Allocated %zu bytes at %p (%s:%d)\n", num * size, p, file, line);
    add_track(p, num * size, file, line, "CALLOC");
    return p;
}

void* my_realloc(void* ptr, size_t size, const char* file, int line) {
    /* realloc은 복잡함: 기존 ptr 해제 + 새 ptr 할당 */
    if (ptr) remove_track(ptr);
    printf("[my_realloc] Reallocating to %zu bytes from %p (%s:%d)\n", size, ptr, file, line);
    void* p = realloc(ptr, size);
    add_track(p, size, file, line, "REALLOC");
    return p;
}

void my_free(void* ptr) {
    remove_track(ptr);
    printf("[my_free] Freeing memory at %p\n", ptr);
    free(ptr);
}

/* 3. GLib 계열 */
void* my_g_malloc(size_t size, const char* file, int line) {
    void* p = g_malloc(size);
    printf("[my_g_malloc] Allocated %zu bytes at %p (%s:%d)\n", size, p, file, line);
    add_track(p, size, file, line, "GMALLOC");
    return p;
}

void* my_g_malloc0(size_t size, const char* file, int line) {
    void* p = g_malloc0(size);
    printf("[my_g_malloc0] Allocated %zu bytes at %p (%s:%d)\n", size, p, file, line);
    add_track(p, size, file, line, "GMALLOC0");
    return p;
}

void my_g_free(void* ptr) {
    remove_track(ptr);
    printf("[my_g_free] Freeing memory at %p\n", ptr);
    g_free(ptr);
}

/* 4. gSOAP 계열 */
void* my_soap_malloc(void* soap, size_t size, const char* file, int line) {
    /* soap_malloc은 soap_end() 호출 시 일괄 해제되므로 
       개별 free 호출이 없어 누수로 잡힐 가능성이 높습니다.
       리포트 확인 시 "SOAP" 태그는 별도로 필터링해서 봐야 합니다. */
    void* p = soap_malloc(soap, size);
    printf("[my_soap_malloc] Allocated %zu bytes at %p (%s:%d)\n", size, p, file, line);
    add_track(p, size, file, line, "SOAP");
    return p;
}


/* ----------------------------------------------------------------------------------
   리포트 출력 함수
   - 프로그램 종료 시점이나, 특정 커맨드 입력 시 호출하세요.
   ---------------------------------------------------------------------------------- */
void print_leak_report(void) {
    printf("\n========================================================================\n");
    printf("                  MEMORY LEAK DETECTION REPORT                          \n");
    printf("========================================================================\n");
    
    int total_leaks = 0;
    size_t total_size = 0;
    int soap_leaks = 0;

    for (int i = 0; i < MAX_TRACK_ALLOCS; i++) {
        if (tracker[i].active) {
            /* SOAP 할당은 soap_end() 호출 여부에 따라 다르므로 별도 카운팅 추천 */
            if (strcmp(tracker[i].type, "SOAP") == 0) {
                soap_leaks++;
            } else {
                printf("[LEAK] %-8s Size:%-6zu Addr:%p | %s:%d\n", 
                       tracker[i].type, 
                       tracker[i].size, 
                       tracker[i].ptr, 
                       tracker[i].file, 
                       tracker[i].line);
                
                total_leaks++;
                total_size += tracker[i].size;
            }
        }
    }

    printf("------------------------------------------------------------------------\n");
    if (total_leaks == 0) {
        printf("RESULT: No standard memory leaks detected! (Good Job)\n");
    } else {
        printf("RESULT: Detected %d leaks (Total %zu bytes)\n", total_leaks, total_size);
    }
    
    if (soap_leaks > 0) {
        printf("NOTE  : %d allocations from soap_malloc found. \n", soap_leaks);
        printf("        Make sure 'soap_destroy(soap); soap_end(soap);' is called.\n");
    }
    printf("========================================================================\n");
}
