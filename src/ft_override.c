#include "../include/ft_malloc.h"

void  *malloc(size_t size){
  return (ft_malloc(size));
}

void  *realloc(void *ptr, size_t size){
  return (ft_realloc(ptr, size));
}

void	*calloc(size_t nmemb, size_t size){
  return (ft_calloc2(nmemb, size));
}


void free(void *ptr){
  ft_free(ptr);
}

