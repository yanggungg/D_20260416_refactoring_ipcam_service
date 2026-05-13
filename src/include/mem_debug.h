#ifndef MEM_DEBUG_H
#define MEM_DEBUG_H

/* 1. 기본 타입 정의용 헤더 */
#include <stddef.h>

/* * [핵심 해결책]
 * 매크로(#define g_malloc ...)가 정의되기 전에 
 * 원본 함수 선언이 있는 헤더를 '먼저' 포함해야 합니다.
 * 그래야 원본 선언이 깨지지 않습니다.
 */
#include <glib.h>       /* gmem.h 에러 해결을 위해 필수 */
#include <stdsoap2.h>   /* soap_malloc 래핑을 위해 필수 */

/* * 주의: <sys/socket.h>, <netdb.h> 등의 
 * OS 시스템 헤더는 여기서 포함하지 마세요. (FTP 충돌 방지)
 */

#define DEBUG_LEAK

#ifdef DEBUG_LEAK

/* 이제 원본 선언이 로드되었으므로, 안전하게 매크로로 덮어쓸 수 있습니다. */
#define malloc(s)          my_malloc(s, __FILE__, __LINE__)
#define calloc(n, s)       my_calloc(n, s, __FILE__, __LINE__)
#define realloc(p, s)      my_realloc(p, s, __FILE__, __LINE__)
#define free(p)            my_free(p)

#define g_malloc(s)        my_g_malloc(s, __FILE__, __LINE__)
#define g_malloc0(s)       my_g_malloc0(s, __FILE__, __LINE__)
#define g_free(p)          my_g_free(p)

#define soap_malloc(soap, s) my_soap_malloc(soap, s, __FILE__, __LINE__)

/* 함수 프로토타입 */
/* C++ 호환성 추가 (선택사항이지만 권장) */
#ifdef __cplusplus
extern "C" {
#endif

void* my_malloc(size_t size, const char* file, int line);
void* my_calloc(size_t num, size_t size, const char* file, int line);
void* my_realloc(void* ptr, size_t size, const char* file, int line);
void  my_free(void* ptr);

void* my_g_malloc(size_t size, const char* file, int line);
void* my_g_malloc0(size_t size, const char* file, int line);
void  my_g_free(void* ptr);

#define g_new(type, count)  ((type*)my_g_malloc(sizeof(type) * (count), __FILE__, __LINE__))
#define g_new0(type, count) ((type*)my_g_malloc0(sizeof(type) * (count), __FILE__, __LINE__))

void* my_soap_malloc(void* soap, size_t size, const char* file, int line);

void print_leak_report(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_LEAK */
#endif /* MEM_DEBUG_H */