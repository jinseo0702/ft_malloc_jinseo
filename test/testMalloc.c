#include "../include/ft_malloc.h"

int main(void) {
  char *tiny_ptr[30];
  char *small_ptr[30];
  char *large_ptr[30];
  
  for (int i = 0; i < 30; i++) {
    ft_printf("tiny malloc allocated %d\n", i);
    tiny_ptr[i] = ft_malloc(50 );
    ft_printf("tiny malloc allocated %p \n", tiny_ptr[i]);
  }
  ft_printf("tiny malloc all\n");

  for (int i = 0; i < 30; i++) {
    ft_printf("small malloc allocated %d\n", i);
    small_ptr[i] = ft_malloc(400 );
    ft_printf("small malloc allocated %p\n", small_ptr[i]);
  }
  ft_printf("small malloc all\n");
  ft_printf("\n");
  ft_printf("\n");
  for (int i = 0; i < 30; i++) {
    ft_printf("large malloc allocated %d\n", i);
    large_ptr[i] = ft_malloc(4097);
    ft_printf("large malloc allocated %p\n", large_ptr[i]);
  }
  ft_printf("large malloc all\n");
  ft_printf("\n");
  ft_printf("\n");

  show_alloc_mem();


  ft_printf("\n");
  ft_printf("\n");
  for (int i = 0; i < 30; i++) {
    ft_printf("small free allocated %p\n", small_ptr[i]);
    ft_free(small_ptr[i]);
  }
  ft_printf("\nsmall free all\n");
  ft_printf("\n");
  for (int i = 0; i < 30; i++) {
    ft_printf("large free allocated %p\n", large_ptr[i]);
    ft_free(large_ptr[i]);
  }
  ft_printf("\nlarge free all\n");
  ft_printf("\n");
  for (int i = 0; i < 30; i++) {
    ft_printf("tiny free allocated %p\n", tiny_ptr[i]);
    ft_free(tiny_ptr[i]);
  }
  ft_printf("\ntiny free all\n");
  show_alloc_mem();


  return (0);
}

