#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
  int client_size;
  char position;
} METADATA_t;

typedef struct FREEDATA_s {
  BLOCK96_t *_tiny;
  BLOCK512_t *_small;
  char tiny[16];
  char small[16];
} FREEDATA_t;

/*
static 을 사용 해서 cache 를 만들었기때문에 단 한번 systemcall 을 사용한다.
*/
long return_sc_pagesize(void);
// 다시 내가 error 를 출력하는 print 를 재정의 할 것.
void print_error(char *str);
//page를 카운트 하기위한
long calculater_page_size(size_t size);
//meta data를 할당받은 곳에 넣기 위한 함수.
void init_meta_data(void *ptr ,size_t size);
//mmap을 받기위함 함수
void *ft_malloc(size_t size);
//비어있는 zone 을 찾기 위한 함수