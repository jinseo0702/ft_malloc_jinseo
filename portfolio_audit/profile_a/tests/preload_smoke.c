#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
  volatile unsigned char *ptr = malloc(32);
  static const char success[] = "preload_smoke_ok\n";

  if (ptr == NULL)
    return 1;
  for (size_t index = 0; index < 32; ++index)
    ptr[index] = (unsigned char)(index ^ 0x5aU);
  for (size_t index = 0; index < 32; ++index) {
    if (ptr[index] != (unsigned char)(index ^ 0x5aU))
      return 1;
  }
  free((void *)ptr);
  (void)write(STDOUT_FILENO, success, sizeof(success) - 1);
  return 0;
}
