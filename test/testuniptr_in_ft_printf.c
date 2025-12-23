#include "../include/ft_malloc.h"
#include <stdint.h>

int main(void){

  char *ptr = "Hello Wrold\n";

  uintptr_t addressInfo = (uintptr_t)ptr;
  ft_printf("addressInfo is : %P\n", addressInfo);
  ft_printf("ptraddress  is : %P\n", ptr);
  /* 
  그럼 내가  (char *)addressInfo 하면 문자열이 보일까?
  정답은 그렇다. 이다
  addressInfo is : 0x5F5E44F14008
  ptraddress  is : 0x5F5E44F14008
  casting addressInfo to char * is : Hello Wrold
  */
  ft_printf("casting addressInfo to char * is : %s\n", (char *)addressInfo);

  return  (0);
}