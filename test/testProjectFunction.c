// mmap(2)
// munmap(2)
// getpagesize under OSX or sysconf(_SC_PAGESIZE) under linux
// getrlimit(2)

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>


int main(void){
  printf("RLIM_INFINITY = %lu\n", (unsigned long)RLIM_INFINITY);
  long page_size = sysconf(_SC_PAGE_SIZE);
  struct rlimit rlim;
  memset(&rlim, 0, sizeof(rlim));
  int flag = getrlimit(RLIMIT_DATA, &rlim);
  printf("flag: %d\n", flag);
  printf("Page size: %lu\n", page_size);
  printf("Data size: %lu\n", rlim.rlim_cur);
  printf("Data limit: %lu\n", rlim.rlim_max);
  int flag2 = getrlimit(RLIMIT_AS, &rlim);
  printf("flag2: %d\n", flag2);
  printf("Address space size: %lu\n", rlim.rlim_cur);
  printf("Address space limit: %lu\n", rlim.rlim_max);
  return (0);
}

