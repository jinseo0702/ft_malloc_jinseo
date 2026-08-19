#define _GNU_SOURCE

#include "ft_malloc.h"

#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

static int case_fail(const char *message)
{
  fprintf(stderr, "ASSERTION_FAILED=%s\n", message);
  return 1;
}

static void fill_bytes(void *ptr, size_t size, unsigned char value)
{
  unsigned char *bytes = ptr;

  for (size_t index = 0; index < size; ++index)
    bytes[index] = (unsigned char)(value + (unsigned char)index);
}

static int check_bytes(const void *ptr, size_t size, unsigned char value)
{
  const unsigned char *bytes = ptr;

  for (size_t index = 0; index < size; ++index) {
    if (bytes[index] != (unsigned char)(value + (unsigned char)index))
      return 0;
  }
  return 1;
}

static int allocation_extent_case(size_t size, const char *zone)
{
  unsigned char *ptr = ft_malloc(size);

  if (ptr == NULL)
    return case_fail("allocation returned NULL");
  fill_bytes(ptr, size, 0x31);
  if (!check_bytes(ptr, size, 0x31))
    return case_fail("written extent did not retain pattern");
  printf("zone=%s\nsize=%zu\nptr_nonnull=1\nextent_ok=1\n", zone, size);
  ft_free(ptr);
  return 0;
}

static int case_a01(void)
{
  return allocation_extent_case(32, "TINY");
}

static int case_a03(void)
{
  void *ptr = ft_malloc(0);
  size_t alignment = _Alignof(max_align_t);

  if (ptr != NULL && (uintptr_t)ptr % alignment != 0)
    return case_fail("non-NULL zero-size result is misaligned");
  printf("zero_result=%s\nalignment=%zu\n", ptr == NULL ? "NULL" : "NONNULL", alignment);
  ft_free(ptr);
  return 0;
}

static int case_a09(void)
{
  const size_t sizes[] = {1, 80, 81, 496, 497, 4097};
  size_t alignment = _Alignof(max_align_t);

  for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index) {
    void *ptr = ft_malloc(sizes[index]);
    if (ptr == NULL)
      return case_fail("alignment allocation returned NULL");
    printf("size=%zu remainder=%zu\n", sizes[index], (size_t)((uintptr_t)ptr % alignment));
    if ((uintptr_t)ptr % alignment != 0)
      return case_fail("allocation does not satisfy max_align_t");
    ft_free(ptr);
  }
  printf("alignment=%zu\n", alignment);
  return 0;
}

static int case_a10(void)
{
  const size_t count = 17;
  const size_t width = 13;
  const size_t total = count * width;
  unsigned char *ptr = ft_calloc2(count, width);

  if (ptr == NULL)
    return case_fail("calloc returned NULL");
  for (size_t index = 0; index < total; ++index) {
    if (ptr[index] != 0)
      return case_fail("calloc result contains a non-zero byte");
  }
  fill_bytes(ptr, total, 0x42);
  if (!check_bytes(ptr, total, 0x42))
    return case_fail("calloc extent is not writable");
  printf("zero_bytes=%zu\nwritable=1\n", total);
  ft_free(ptr);
  return 0;
}

static int case_a11(void)
{
  void *ptr = ft_calloc2(SIZE_MAX, 2);

  printf("overflow_result=%s\n", ptr == NULL ? "NULL" : "NONNULL");
  if (ptr != NULL) {
    ft_free(ptr);
    return case_fail("overflowing calloc did not return NULL");
  }
  return 0;
}

static int case_a12(void)
{
  void *first = ft_calloc2(0, 9);
  void *second = ft_calloc2(9, 0);
  size_t alignment = _Alignof(max_align_t);

  if ((first != NULL && (uintptr_t)first % alignment != 0)
      || (second != NULL && (uintptr_t)second % alignment != 0))
    return case_fail("zero-operand calloc returned misaligned storage");
  printf("first=%s\nsecond=%s\n", first == NULL ? "NULL" : "NONNULL",
      second == NULL ? "NULL" : "NONNULL");
  ft_free(first);
  ft_free(second);
  return 0;
}

static int case_a13(void)
{
  ft_free(NULL);
  puts("free_null_completed=1");
  return 0;
}

static int reuse_case(size_t size, const char *zone)
{
  void *first = ft_malloc(size);
  void *second = ft_malloc(size);
  void *reused;

  if (first == NULL || second == NULL)
    return case_fail("reuse setup allocation failed");
  ft_free(first);
  reused = ft_malloc(size);
  printf("zone=%s\nreused=%d\n", zone, reused == first);
  if (reused != first)
    return case_fail("first free slot was not reused");
  ft_free(reused);
  ft_free(second);
  return 0;
}

static int fixed_capacity_case(size_t size, const char *zone)
{
  void *ptrs[100];
  void *overflow;
  void *replacement;

  for (size_t index = 0; index < 100; ++index) {
    ptrs[index] = ft_malloc(size);
    if (ptrs[index] == NULL)
      return case_fail("allocation failed before fixed-zone capacity");
  }
  overflow = ft_malloc(size);
  if (overflow != NULL)
    return case_fail("101st fixed-zone allocation unexpectedly succeeded");
  ft_free(ptrs[37]);
  replacement = ft_malloc(size);
  printf("zone=%s\nfirst_100=100\nallocation_101=null\nrecovered=%d\n",
      zone, replacement == ptrs[37]);
  if (replacement != ptrs[37])
    return case_fail("freed fixed-zone slot was not recovered");
  ptrs[37] = replacement;
  for (size_t index = 0; index < 100; ++index)
    ft_free(ptrs[index]);
  return 0;
}

static int case_a18(void)
{
  void *ptrs[100];
  void *overflow;
  void *replacement;

  for (size_t index = 0; index < 100; ++index) {
    ptrs[index] = ft_malloc(497);
    if (ptrs[index] == NULL)
      return case_fail("LARGE allocation failed before capacity");
  }
  overflow = ft_malloc(497);
  if (overflow != NULL)
    return case_fail("101st LARGE allocation unexpectedly succeeded");
  ft_free(ptrs[37]);
  replacement = ft_malloc(497);
  printf("first_100=100\nallocation_101=null\nrecovered=%d\n", replacement != NULL);
  if (replacement == NULL)
    return case_fail("LARGE capacity did not recover after free");
  ptrs[37] = replacement;
  for (size_t index = 0; index < 100; ++index)
    ft_free(ptrs[index]);
  return 0;
}

static int case_a19(void)
{
  unsigned char *ptr = ft_realloc(NULL, 64);

  if (ptr == NULL)
    return case_fail("realloc(NULL,n) returned NULL");
  fill_bytes(ptr, 64, 0x25);
  if (!check_bytes(ptr, 64, 0x25))
    return case_fail("realloc(NULL,n) extent failed");
  puts("realloc_null_extent=64");
  ft_free(ptr);
  return 0;
}

static int case_a20(void)
{
  void *ptr = ft_malloc(32);
  void *result;
  void *replacement;

  if (ptr == NULL)
    return case_fail("realloc-zero setup failed");
  result = ft_realloc(ptr, 0);
  replacement = ft_malloc(32);
  printf("result_null=%d\nslot_reused=%d\n", result == NULL, replacement == ptr);
  if (result != NULL || replacement != ptr)
    return case_fail("realloc(p,0) did not free-and-return-NULL");
  ft_free(replacement);
  return 0;
}

static int realloc_case(size_t old_size, size_t new_size, int expect_same,
    const char *transition)
{
  unsigned char *old_ptr = ft_malloc(old_size);
  unsigned char *new_ptr;
  size_t prefix = old_size < new_size ? old_size : new_size;

  if (old_ptr == NULL)
    return case_fail("realloc setup allocation failed");
  fill_bytes(old_ptr, old_size, 0x53);
  new_ptr = ft_realloc(old_ptr, new_size);
  if (new_ptr == NULL)
    return case_fail("realloc returned NULL");
  printf("transition=%s\nprefix=%zu\nsame_pointer=%d\n", transition, prefix,
      new_ptr == old_ptr);
  if (!check_bytes(new_ptr, prefix, 0x53))
    return case_fail("realloc prefix was not preserved");
  if (expect_same >= 0 && (new_ptr == old_ptr) != expect_same)
    return case_fail("realloc pointer-identity policy mismatch");
  fill_bytes(new_ptr, new_size, 0x67);
  if (!check_bytes(new_ptr, new_size, 0x67))
    return case_fail("reallocated extent is not writable");
  ft_free(new_ptr);
  return 0;
}

static int case_a22(void)
{
  void *old_ptr = ft_malloc(50);
  void *new_ptr;
  void *reuse;

  if (old_ptr == NULL)
    return case_fail("TINY-to-SMALL setup failed");
  fill_bytes(old_ptr, 50, 0x21);
  new_ptr = ft_realloc(old_ptr, 200);
  if (new_ptr == NULL || !check_bytes(new_ptr, 50, 0x21))
    return case_fail("TINY-to-SMALL prefix failed");
  reuse = ft_malloc(50);
  printf("prefix=50\nold_slot_reused=%d\n", reuse == old_ptr);
  if (reuse != old_ptr)
    return case_fail("old TINY slot was not reusable");
  ft_free(reuse);
  ft_free(new_ptr);
  return 0;
}

static int case_a23(void)
{
  void *old_ptr = ft_malloc(400);
  void *new_ptr;
  void *reuse;

  if (old_ptr == NULL)
    return case_fail("SMALL-to-LARGE setup failed");
  fill_bytes(old_ptr, 400, 0x19);
  new_ptr = ft_realloc(old_ptr, 600);
  if (new_ptr == NULL || !check_bytes(new_ptr, 400, 0x19))
    return case_fail("SMALL-to-LARGE prefix failed");
  reuse = ft_malloc(400);
  printf("prefix=400\nold_slot_reused=%d\n", reuse == old_ptr);
  if (reuse != old_ptr)
    return case_fail("old SMALL slot was not reusable");
  ft_free(reuse);
  ft_free(new_ptr);
  return 0;
}

static int case_a25(void)
{
  unsigned char *original = ft_malloc(32);
  void *large[100];
  void *result;

  if (original == NULL)
    return case_fail("failure-preservation setup failed");
  fill_bytes(original, 32, 0x71);
  for (size_t index = 0; index < 100; ++index) {
    large[index] = ft_malloc(600);
    if (large[index] == NULL)
      return case_fail("could not fill LARGE capacity");
  }
  result = ft_realloc(original, 600);
  printf("realloc_result_null=%d\noriginal_intact=%d\n", result == NULL,
      check_bytes(original, 32, 0x71));
  if (result != NULL || !check_bytes(original, 32, 0x71))
    return case_fail("failed realloc modified original allocation");
  ft_free(original);
  for (size_t index = 0; index < 100; ++index)
    ft_free(large[index]);
  return 0;
}

static int case_a26(void)
{
  static const char begin[] = "AUDIT_A26_BEGIN\n";
  static const char end[] = "AUDIT_A26_END\n";

  (void)write(STDERR_FILENO, begin, sizeof(begin) - 1);
  for (size_t index = 0; index < 1000; ++index) {
    void *tiny = ft_malloc(1);
    void *small = ft_malloc(100);
    if (tiny == NULL || small == NULL)
      return case_fail("sequential reuse allocation failed");
    ft_free(tiny);
    ft_free(small);
  }
  (void)write(STDERR_FILENO, end, sizeof(end) - 1);
  puts("iterations=1000");
  return 0;
}

static int case_a27(void)
{
  static const char begin[] = "AUDIT_A27_BEGIN\n";
  static const char end[] = "AUDIT_A27_END\n";

  (void)write(STDERR_FILENO, begin, sizeof(begin) - 1);
  for (size_t index = 0; index < 10; ++index) {
    void *ptr = ft_malloc(600);
    if (ptr == NULL)
      return case_fail("LARGE lifecycle allocation failed");
    ft_free(ptr);
  }
  (void)write(STDERR_FILENO, end, sizeof(end) - 1);
  puts("iterations=10");
  return 0;
}

static int case_a28(void)
{
  static const char first_begin[] = "AUDIT_SHOW_1_BEGIN\n";
  static const char first_end[] = "AUDIT_SHOW_1_END\n";
  static const char second_begin[] = "AUDIT_SHOW_2_BEGIN\n";
  static const char second_end[] = "AUDIT_SHOW_2_END\n";
  void *tiny = ft_malloc(10);
  void *small = ft_malloc(100);
  void *large = ft_malloc(600);

  if (tiny == NULL || small == NULL || large == NULL)
    return case_fail("show_alloc_mem setup allocation failed");
  (void)write(STDOUT_FILENO, first_begin, sizeof(first_begin) - 1);
  show_alloc_mem();
  (void)write(STDOUT_FILENO, first_end, sizeof(first_end) - 1);
  ft_free(small);
  (void)write(STDOUT_FILENO, second_begin, sizeof(second_begin) - 1);
  show_alloc_mem();
  (void)write(STDOUT_FILENO, second_end, sizeof(second_end) - 1);
  ft_free(tiny);
  ft_free(large);
  return 0;
}

typedef struct thread_case_s {
  int id;
  atomic_int *failure;
} thread_case_t;

static void *thread_worker(void *opaque)
{
  thread_case_t *config = opaque;
  const size_t sizes[] = {16, 128, 600};

  for (size_t iteration = 0; iteration < 500; ++iteration) {
    size_t size = sizes[(iteration + (size_t)config->id) % 3];
    unsigned char seed = (unsigned char)(config->id * 17 + (int)iteration);
    void *ptr = ft_malloc(size);
    if (ptr == NULL) {
      atomic_store(config->failure, 1);
      return NULL;
    }
    fill_bytes(ptr, size, seed);
    if (!check_bytes(ptr, size, seed))
      atomic_store(config->failure, 1);
    ft_free(ptr);
    if (atomic_load(config->failure) != 0)
      return NULL;
  }
  return NULL;
}

static int case_a29(void)
{
  pthread_t threads[4];
  thread_case_t configs[4];
  atomic_int failure = 0;

  for (int index = 0; index < 4; ++index) {
    configs[index].id = index;
    configs[index].failure = &failure;
    if (pthread_create(&threads[index], NULL, thread_worker, &configs[index]) != 0)
      return case_fail("pthread_create failed");
  }
  for (int index = 0; index < 4; ++index) {
    if (pthread_join(threads[index], NULL) != 0)
      return case_fail("pthread_join failed");
  }
  printf("threads=4\niterations_per_thread=500\nintegrity=%d\n",
      atomic_load(&failure) == 0);
  return atomic_load(&failure) == 0 ? 0 : case_fail("thread integrity failed");
}

static int case_a30_foreign(void)
{
  struct fake_allocation_s {
    METADATA_t metadata;
    max_align_t payload_alignment;
  } fake = {{0, 0, 0}, {0}};
  void *live = ft_malloc(16);
  void *alias;

  if (live == NULL)
    return case_fail("foreign-pointer setup failed");
  fake.metadata.size = TINY;
  fake.metadata.position = 0;
  fake.metadata.client_size = 16;
  ft_free((void *)(&fake.metadata + 1));
  alias = ft_malloc(16);
  printf("foreign_free_created_live_alias=%d\n", alias == live);
  if (alias != NULL)
    ft_free(alias);
  return 0;
}

static int case_a30_double(void)
{
  void *ptr = ft_malloc(16);
  void *next;

  if (ptr == NULL)
    return case_fail("double-free setup failed");
  ft_free(ptr);
  ft_free(ptr);
  next = ft_malloc(16);
  printf("double_free_completed=1\nslot_reusable=%d\n", next == ptr);
  ft_free(next);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 2)
    return case_fail("expected one case id");
  if (strcmp(argv[1], "A01") == 0) return case_a01();
  if (strcmp(argv[1], "A03") == 0) return case_a03();
  if (strcmp(argv[1], "A04") == 0) return allocation_extent_case(1, "TINY");
  if (strcmp(argv[1], "A05") == 0) return allocation_extent_case(80, "TINY");
  if (strcmp(argv[1], "A06") == 0) return allocation_extent_case(81, "SMALL");
  if (strcmp(argv[1], "A07") == 0) return allocation_extent_case(496, "SMALL");
  if (strcmp(argv[1], "A08") == 0) return allocation_extent_case(497, "LARGE");
  if (strcmp(argv[1], "A09") == 0) return case_a09();
  if (strcmp(argv[1], "A10") == 0) return case_a10();
  if (strcmp(argv[1], "A11") == 0) return case_a11();
  if (strcmp(argv[1], "A12") == 0) return case_a12();
  if (strcmp(argv[1], "A13") == 0) return case_a13();
  if (strcmp(argv[1], "A14") == 0) return reuse_case(32, "TINY");
  if (strcmp(argv[1], "A15") == 0) return reuse_case(200, "SMALL");
  if (strcmp(argv[1], "A16") == 0) return fixed_capacity_case(1, "TINY");
  if (strcmp(argv[1], "A17") == 0) return fixed_capacity_case(100, "SMALL");
  if (strcmp(argv[1], "A18") == 0) return case_a18();
  if (strcmp(argv[1], "A19") == 0) return case_a19();
  if (strcmp(argv[1], "A20") == 0) return case_a20();
  if (strcmp(argv[1], "A21") == 0) return realloc_case(32, 64, 1, "TINY_TO_TINY");
  if (strcmp(argv[1], "A22") == 0) return case_a22();
  if (strcmp(argv[1], "A23") == 0) return case_a23();
  if (strcmp(argv[1], "A24") == 0) return realloc_case(600, 200, 1, "LARGE_TO_SMALLER");
  if (strcmp(argv[1], "A25") == 0) return case_a25();
  if (strcmp(argv[1], "A26") == 0) return case_a26();
  if (strcmp(argv[1], "A27") == 0) return case_a27();
  if (strcmp(argv[1], "A28") == 0) return case_a28();
  if (strcmp(argv[1], "A29") == 0) return case_a29();
  if (strcmp(argv[1], "A30_FOREIGN") == 0) return case_a30_foreign();
  if (strcmp(argv[1], "A30_DOUBLE") == 0) return case_a30_double();
  return case_fail("unknown case id");
}
