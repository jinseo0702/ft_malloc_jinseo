#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
  size_t sizes[] = {1,15,16,95,96,97,472,511,512,513};
  for (int k=0;k<10;k++){
    for (int i=0;i<100000;i++){
      void *p = malloc(sizes[k]);
      if (!p) { fprintf(stderr,"NULL @%zu\n", sizes[k]); return 1; }
      if (((uintptr_t)p & 0xF) != 0) { fprintf(stderr,"misalign @%zu\n", sizes[k]); return 2; }
      free(p);
    }
  }
  return 0;
}