#ifndef FT_MALLOC_H
# define FT_MALLOC_H

#include "../libft/libft.h"
#include "../printf/libftprintf.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>


#define DEFAULTSIZE 0x10 //16
#define INDEX_MASK 0x0F //15
#define TINY 0x60 //96
#define SMALL 0x200 //512
#define METASIZE 0x10 //64비트 기준 다른 컴퓨터에서는 다를 수 있음
#define POS_MASK 0x07

/*
단위를 계산하는게 어렵기 때문에 내가 원하는 자료형을 만들었다.
*/
typedef struct BLOCK96_s {
  unsigned char ptr[96];
} BLOCK96_t;

typedef struct BLOCK512_s {
  unsigned char ptr[512];
} BLOCK512_t;

typedef struct METADATA_s {
  size_t size;
  char position;
  int client_size;
} METADATA_t;

typedef struct FREEDATA_s {
  BLOCK96_t *_tiny;
  BLOCK512_t *_small;
  char tiny[16];
  char small[16];
  unsigned long large[101];
  int large_cnt;
} FREEDATA_t;

void show_alloc_mem(void);
// 다시 내가 error 를 출력하는 print 를 재정의 할 것.
void print_error(const char *str);
//static 을 사용 해서 cache 를 만들었기때문에 단 한번 systemcall 을 사용한다.
long return_sc_pagesize(void);
//page를 카운트 하기위한
long calculater_page_size(size_t size);
//meta data를 할당받은 곳에 넣기 위한 함수.
void init_meta_data(void *ptr ,size_t size);
unsigned char find_free_zone(size_t size_type);
//mmap을 받기위함 함수
void *ft_malloc(size_t size);
//비어있는 zone 을 찾기 위한 함수
void ft_free(void *ptr);
//재할당하는 함수
void *ft_realloc(void *ptr, size_t size);
//ft_calloc_ver2.0
void	*ft_calloc2(size_t nmemb, size_t size);

//ft_override.c
void  *malloc(size_t size);
void  free(void *ptr);
void  *realloc(void *ptr, size_t size);
void	*calloc(size_t nmemb, size_t size);

//multi thread를 방지하기 위해서 만드는 static inline 
static inline void manage_free_mutex_lock(pthread_mutex_t *m){
  if (pthread_mutex_lock(m) != 0) print_error("pthread_mutex_lock Error");
}

static inline void manage_free_mutex_unlock(pthread_mutex_t *m){
  if (pthread_mutex_unlock(m) != 0) print_error("pthread_mutex_unlock Error");
}

#endif