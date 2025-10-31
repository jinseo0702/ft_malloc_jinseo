#include "../include/ft_malloc.h"

int main(void){
  char *ptr = ft_malloc(0);
  show_alloc_mem();
  ft_free(ptr);
  show_alloc_mem();
  return (0);
}