#include "../include/ft_malloc.h"


FREEDATA_t manage_free;

void show_alloc_mem(){
  size_t total_size = 0;
  ft_printf("TINY : %p\n", manage_free._tiny);
  unsigned char find, cnt, cnt2 = 0x00;
    for (int idx = 0x00; idx < 0x0D; idx++) {
      for (int fos = 0x00; fos < 0x08 ; fos++) {
        if (cnt >= 100) {
          break;
        }
        find = manage_free.tiny[idx] & (1 << fos);
        if (find != 0x00){
          METADATA_t *meta = (METADATA_t *)&manage_free._tiny[(idx * 8) + fos];
          ft_printf("%P - %P : %d byte tiny zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, (meta->size & ~INDEX_MASK));
        // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
          total_size += meta->client_size;
        }
        cnt++;
      }
    }
  ft_printf("SMALL : %p\n", manage_free._small);
  for (int idx = 0x00; idx < 0x0D; idx++) {
    for (int fos = 0x00; fos < 0x08 ; fos++) {
      if (cnt2 >= 100) {
        break;
      }
      find = manage_free.small[idx] & (1 << fos);
      if (find != 0x00){
        METADATA_t *meta = (METADATA_t *)&manage_free._small[(idx * 8) + fos];
        ft_printf("%P - %P : %d byte small zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, (meta->size & ~INDEX_MASK));
        // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
        total_size += meta->client_size;
      }
      cnt2++;
    }
  }
  ft_printf("LARGE : %P\n", manage_free.large);
  for (int idx = 0; idx < 100; idx++) {
    if (manage_free.large[idx] != 0) {
      METADATA_t *meta = (METADATA_t *)manage_free.large[idx];
      ft_printf("%P - %P : %d byte Large zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, meta->size);
      // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
      total_size += meta->client_size;
    }
  }
  ft_printf("Total size: %d byte\n", total_size);
}

void print_error(char *str){
  ft_fprintf(2, "Error %s : %s (errno = %d)\n", str, strerror(errno), errno);
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
      tmp->client_size = 0;
    }
  }
  else if (size == SMALL) {
    BLOCK512_t *ptr_SAMLL = (BLOCK512_t *)ptr;
    short index;
    for (int idx = 0; idx < 100; idx++) {
      METADATA_t *tmp = (METADATA_t *)(ptr_SAMLL + idx);
      index = (idx >> 3);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
      tmp->client_size = 0;
    }
  }
  else if(size > 512) {
    int idx = 0;
    for (; idx < 100; idx++) {
      if(manage_free.large[idx] == 0){
        manage_free.large[idx] = (unsigned long)ptr;
        break;
      }
    }
    METADATA_t *tmp = (METADATA_t *)ptr;
    tmp->size = size;
    tmp->position = idx;
    tmp->client_size = 0;
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
  static char tiny_flag, small_flag, large_flag;
  long page_size;

  if (size <  1){
    size = 1;
  }

  //debug
  // static int cnt = 0;
  // ft_printf("cnt : %d\n", cnt);
  // cnt++;
  //debug
  
  // show_alloc_mem();
  page_size = calculater_page_size(size); //debug
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
    zone = find_free_zone(size);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._tiny[index];
    temp->client_size = size;
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
    zone = find_free_zone(size);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._small[index];
    temp->client_size = size;
    return ((void *)(temp + 1));
  }
  else  {
    if (manage_free.large_cnt < 100){
      if (large_flag == 0){
        ft_memset(manage_free.large, 0, sizeof(manage_free.large));
        large_flag |= (0x01);
      }
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
      temp->client_size = size;
      manage_free.large_cnt++;
      return ((void *)(temp + 1));
    }
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
    // meta->size = 0;
  }
  else if (size_type == SMALL) {
    manage_free.small[index] &= ~(1 << meta->position);
    // meta->size = 0;
  }
  else if (size_type > 0x200){
    int flag = 0;
    int pos = meta->position;
    size_t len = meta->size;
    manage_free.large[pos] = 0;
    flag = munmap((void *)meta, len);
    if (flag < 0) {
      print_error("munmap Fail");
    }
    manage_free.large_cnt -= 1;
  }
  else {
    return ;
  }
}

/*
realloc() 함수는 ptr이 가리키는 메모리 블록의 크기를 size 바이트로 변경합니다. 
내용은 영역의 시작점부터 이전 크기와 새 크기 중 더 작은 값(minimum)까지의 범위에서 변경되지 않은 상태로 유지됩니다. 
만약 새 크기가 이전 크기보다 크다면, 추가된 메모리는 초기화되지 않습니다. 

만약 ptr이 NULL이라면, 그 호출은 모든 size 값에 대해 malloc(size)와 동일합니다;
만약 size가 0이고 ptr이 NULL이 아니라면, 
그 호출은 free(ptr)과 동일합니다. 
ptr이 NULL이 아니라면, 그것은 이전에 malloc(), calloc(), 또는 realloc() 호출에 의해 반환된 값이어야 합니다.
만약 가리키던 영역이 이동되었다면, free(ptr)이 수행됩니다.

같은 zone 이라면 meta data만 변경됩니다.
만약 small zone 에서 tiny 존의 변경이라면 그냥 small zone을 사용하고 meta data 만 변경됩니다.
만약 작은 zone 에서 더 큰 존으로 간다면 data는 보존이되고 작은 zone 의 memory 는 free(*beforezone) 이 되어야한다.

 |  ptr  | size |       do      |
 | NULL  | ?    |  malloc(size) |
 |!=NULL | ==0  |  free(ptr)    |
 
 반환 값
 realloc() 함수는 새로 할당된 메모리에 대한 포인터를 반환하며, 
 이 포인터는 어떤 내장 타입(built-in type)에도 적합하게 정렬(aligned)되어 있습니다. 
 요청이 실패하면 NULL을 반환합니다. 
 반환된 포인터는, 만약 할당이 이동되지 않았다면 (예: 그 자리에서(in-place) 할당을 확장할 공간이 있었다면) ptr과 같을 수도 있고, 
 할당이 새 주소로 이동되었다면 ptr과 다를 수도 있습니다. 
 만약 size가 0이었다면, NULL 또는 free()에 전달하기 적합한 포인터가 반환됩니다. 
 만약 realloc()이 실패하면, 원본 블록은 건드려지지 않은 상태로 남습니다; 그것은 해제(freed)되거나 이동되지 않습니다.

 |  size | meta   | return |
 | tiny  | tiny   | origin |
 | tiny  | small  | origin |
 | tiny  | large  | origin | //최적화 할거면 반납후 사용해도 되는데 굳이?
 | small | tiny   | small  |
 | small | small  | origin |
 | small | large  | origin |
 | large | tiny   | large  |
 | large | small  | large  |
 | large | large  | origin |
*/

void *ft_realloc(void *ptr, size_t size){
  METADATA_t *meta;
  void *temp;
  size_t zone;
  
  if (ptr != NULL && size == 0) {
    ft_free(ptr);
    return (NULL);
  }
  if (ptr == NULL) {
    return (ft_malloc(size));
  }
  meta = (METADATA_t *)ptr - 1;
  zone = (meta->size & ~INDEX_MASK);
  if (size < (TINY - METASIZE)) {
    meta->client_size = size;
    return ((void *)(meta + 1));
  }
  else if (size < (SMALL - METASIZE)) {
    if (zone != TINY) {
      meta->client_size = size;
      return ((void *)(meta + 1));
    }
    temp = ft_malloc(size);
    if (temp == NULL) {
      return (NULL);
    }
    temp = ft_memmove(temp, ptr, meta->client_size);
    ft_free(ptr);
    return (temp);
  }
  else {
    if (zone < size + 1) {
      temp = ft_malloc(size);
      if (temp == NULL) {
        return (NULL);
      }
      temp = ft_memmove(temp, ptr, meta->client_size);
      ft_free(ptr);
      return (temp);
    }
    meta->client_size = size;
    return ((void *)(meta + 1));
  }
  return (NULL);
}

void	*ft_calloc2(size_t nmemb, size_t size){
  void	*reptr;
	size_t	total;

  if (nmemb == 0 || size == 0)
    return (ft_malloc(0));
	total = nmemb * size;
	if (total != 0 && total / nmemb != size)
		return (NULL);
	reptr = (void *)ft_malloc(size * nmemb);
	if (reptr == NULL) {
    return (NULL);
  };
	ft_memset(reptr, 0, size * nmemb);
	return (reptr);
}
