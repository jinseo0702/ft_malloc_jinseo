#include "./testMetadata.h"
#include <stdio.h>

int main(void){

  long page_tiny, page_small, page_large = 0;
  page_tiny = calculater_page_size(TINY);
  page_small = calculater_page_size(SMALL);
  page_large = calculater_page_size(4097);

  printf("page_tiny = %ld \n page_small = %ld \n page_large = %ld \n", page_tiny, page_small, page_large);

  /*
  error calculate twice
  */
  printf("-----------------------------------------------------\n");
  page_tiny = calculater_page_size(TINY);
  page_small = calculater_page_size(SMALL);
  page_large = calculater_page_size(4097);
  printf("page_tiny = %ld \n page_small = %ld \n page_large = %ld \n", page_tiny, page_small, page_large);
  printf("-----------------------------------------------------\n");
  printf("metadata size = %ld \n", sizeof(METADATA_t));

  return (0);
}