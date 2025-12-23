#include "./testMetadata.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

FREEDATA_t manage_free;

void print_error(char *str){
  fprintf(stderr, "Error %s : %s (errno = %d)\n", str ,strerror(errno), errno);
  exit(EXIT_FAILURE);
}

long return_sc_pagesize() {
  static long _sc_page_size_cache;
  int flag = 0x00;
  if (_sc_page_size_cache == 0x00) {
    flag = sysconf(_SC_PAGESIZE);
    if (flag == -1){
      print_error("return_sc_pagesize");
    }
    _sc_page_size_cache = flag;
  }
  return (_sc_page_size_cache);
}

/*
make catch unkwon error flag
flag_tiny and falg_small
use usefull
#define DEFAULTSIZE 0x10 //16
#define TINY 0x60 //96
#define SMALL 0x200 //512
*/
long calculater_page_size(size_t size) {
  static int flag_tiny;
  static int flag_small;
  long PAGE_SIZE;
  long PAGE_COUNT;
  long PAGE_MASK;

  if (!(~flag_tiny & size) || !(~flag_small & size) || size < 1) {
    return (-1);
  }
  PAGE_SIZE = return_sc_pagesize();
  PAGE_MASK = ~(PAGE_SIZE - 1);
  if (size == TINY && !flag_tiny) {
    PAGE_COUNT = ((TINY * 0x64) + (PAGE_SIZE - 1)) & PAGE_MASK;
    flag_tiny |= TINY;
  }
  else if (size == SMALL && !flag_small) {
    PAGE_COUNT = ((SMALL * 0x64) + (PAGE_SIZE - 1)) & PAGE_MASK;
    flag_small |= SMALL;
  }
  else {
    PAGE_COUNT = (size + (PAGE_SIZE - 1)) & PAGE_MASK;
  }
  return (PAGE_COUNT);
}

/*
확장성을 고려해서 아직 error 플레그는 만들지 말자.
페이지 단편화를 해결 할 수 있으니까.
void init_meta_data(void *ptr ,size_t size, int multiful_of_size);
단편화 해결 못할 것 같다. 난 딱 100까지만 보장한다.
idx 의 하위 3개의 비트만 보면 position 이 어디인지 알수 있다.
*/
void init_meta_data(void *ptr ,size_t size){
  if (size == TINY) {
    short index;
    BLOCK96_t *ptr_TINY = (BLOCK96_t *)ptr;
    for (int idx = 0; idx < 100; idx++) {
      METADATA_t *tmp = (METADATA_t *)(ptr_TINY + idx);
      index = (idx >> 3);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
    }
  }
  else if (size == SMALL) {
    BLOCK512_t *ptr_SAMLL = (BLOCK512_t *)ptr;
    short index;
    for (int idx = 0; idx < 100; idx++) {
      METADATA_t *tmp = (METADATA_t *)(ptr_SAMLL + idx);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
    }
  }
  else if(size > 512) {
    METADATA_t *tmp = (METADATA_t *)ptr;
    tmp->size = size;
    tmp->position = 0x00;
  }
  else {
    print_error("do Error");
  }
}

/*
나중에 이해하기 쉽고 다른 사람들이 이해하기 쉽도록 변수를 많이 선언했습니다.
result 가 0xFF 이라는건 지금 할당 할 수 있는 공간 이 없다는 의미 입니다.
*/
unsigned char find_free_zone(size_t size_type){
  unsigned char front, back, find, cnt = 0x00;
  unsigned char result = 0xFF;

  if (size_type < TINY) {
    for (int idx = 0x00; idx < 0x0D; idx++) {
      for (int fos = 0x00; fos < 0x08 ; fos++) {
        if (cnt >= 100) {
          return(result);
        }
        find = manage_free.tiny[idx] & (1 << fos);
        if (find == 0x00){
          front = idx;
          back = fos;
          result = (front << 0x04) | back;
          manage_free.tiny[idx] |= (1 << fos);
          return (result);
        }
        cnt++;
      }
    }
  }
  else if(size_type < SMALL){
      for (int idx = 0x00; idx < 0x0D; idx++) {
        for (int fos = 0x00; fos < 0x08 ; fos++) {
          if (cnt >= 100) {
            return(result);
          }
          find = manage_free.small[idx] & (1 << fos);
          if (find == 0x00){
            front = idx;
            back = fos;
            result = (front << 0x04) | back;
            manage_free.small[idx] |= (1 << fos);
            return (result);
          }
          cnt++;
        }
    } 
  }
  return (result);
}

/*
메모리의 욺직임은 내가 만든 BLOCK 의 크기만큼 욺직이기때문에 행복하다.
*/
void *ft_malloc(size_t size){
  static char tiny_flag, small_flag;
  long page_size;

  page_size = calculater_page_size(size);
  if(page_size == -1){
    return (NULL);
  }

  if (size < (TINY - METASIZE)) {
    if (!tiny_flag){
      page_size = calculater_page_size(TINY);
      if(page_size == -1){
        return (NULL);
      }
      manage_free._tiny = (BLOCK96_t *)mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (manage_free._tiny == MAP_FAILED) {
        return (NULL);
      }
      init_meta_data((void *)manage_free._tiny, TINY);
      tiny_flag = 0x01;
    }
    unsigned char zone, front, back, index = 0x00;
    zone = find_free_zone(TINY);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._tiny[index];
    return ((void *)(temp + 1));
  }
  else if (size < (SMALL - METASIZE)) {
    if (!small_flag) {
      page_size = calculater_page_size(SMALL);
      if(page_size == -1){
        return (NULL);
      }
      manage_free._small = (BLOCK512_t *)mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (manage_free._small == MAP_FAILED) {
        return (NULL);
      }
      init_meta_data((void *)manage_free._small, SMALL);
      small_flag = 0x01;
    }
    unsigned char zone, front, back, index= 0x00;
    zone = find_free_zone(TINY);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._small[index];
    return ((void *)(temp + 1));
  }
  else  {
    page_size = calculater_page_size(size + METASIZE);
    if(page_size == -1){
      return (NULL);
    }
    void *ptr = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
      return (NULL);
    }
    init_meta_data(ptr, page_size);
    METADATA_t *temp = (METADATA_t *)ptr;
    return ((void *)(temp + 1));
  }
    return (NULL);
}

/*
*/
void ft_free(void *ptr){
  int size_type, index = 0;

  if (ptr == NULL){
    return;
  }
  METADATA_t *meta;
  meta = (METADATA_t *)ptr - 1;
  if(meta == NULL){
    return;
  } 
  index = meta->size & INDEX_MASK;
  size_type = meta->size & ~INDEX_MASK;
  if (size_type == TINY) {
    manage_free.tiny[index] &= ~(1 << meta->position);
  }
  else if (size_type == SMALL) {
    manage_free.small[index] &= ~(1 << meta->position);
  }
  else if (size_type > 0x200){
    int flag = 0;
    // int PAGE_SIZE, PAGE_MASK = 0;

    // PAGE_SIZE = return_sc_pagesize();
    // PAGE_MASK = ~(PAGE_SIZE - 1);
    // size_type = meta->size & ~INDEX_MASK;
    flag = munmap((void *)meta, meta->size);
    if (flag < 0) {
      print_error("munmap Fail");
    }
  }
  else {
    return ;
  }
}

/*
int main(void) {

  t_METADATA data;
  memset(&data, 0, sizeof(t_METADATA));
  long pagesize = return_sc_pagesize();
  printf("meta Data size is %lu %lu\n", sizeof(t_METADATA), pagesize);
  void *ptr = mmap(NULL, (size_t)pagesize, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (ptr == MAP_FAILED) {
    print_error("mmap");
  }
  printf("struct t_FREEDATA %lu\n", sizeof(t_FREEDATA));
  munmap(ptr, pagesize);
  return (0);
}
  */