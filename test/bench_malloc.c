#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static inline uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

typedef struct {
  size_t size;
  uint64_t iters;
  int touch;          // 0: touch 안함, 1: memset으로 touch
  volatile uint64_t *sink;
} worker_cfg_t;

typedef struct {
  worker_cfg_t cfg;
} worker_arg_t;

static void *worker(void *vp) {
  worker_arg_t *a = (worker_arg_t *)vp;
  size_t sz = a->cfg.size;
  uint64_t n = a->cfg.iters;
  int touch = a->cfg.touch;
  volatile uint64_t *sink = a->cfg.sink;

  for (uint64_t i = 0; i < n; i++) {
    void *p = malloc(sz);              // LD_PRELOAD로 ft_malloc로 바뀜
    if (!p) {
      fprintf(stderr, "malloc failed at i=%" PRIu64 " (errno=%d: %s)\n",
              i, errno, strerror(errno));
      break;
    }
    if (touch) {
      memset(p, 0xA5, sz);             // 실제로 페이지를 만지게(성능은 더 느려짐)
    } else {
      ((unsigned char *)p)[0] = 0xA5;  // 최소한 최적화 방지용 touch
    }
    *sink += (uintptr_t)p & 0xFFu;     // 결과가 사라지지 않게
    free(p);
  }
  return NULL;
}

static uint64_t parse_u64(const char *s, const char *name) {
  char *end = NULL;
  errno = 0;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno || !end || *end != '\0') {
    fprintf(stderr, "bad %s: %s\n", name, s);
    exit(1);
  }
  return (uint64_t)v;
}

static size_t parse_size(const char *s, const char *name) {
  char *end = NULL;
  errno = 0;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno || !end || *end != '\0') {
    fprintf(stderr, "bad %s: %s\n", name, s);
    exit(1);
  }
  return (size_t)v;
}

static void usage(const char *argv0) {
  fprintf(stderr,
    "usage: %s [--size N] [--iters N] [--threads N] [--touch]\n"
    "\n"
    "examples:\n"
    "  %s --size 1 --iters 1000000 --threads 1\n"
    "  %s --size 200 --iters 1000000 --threads 4\n"
    "  %s --size 2000 --iters 200000 --threads 1 --touch\n",
    argv0, argv0, argv0, argv0
  );
  exit(1);
}

int main(int argc, char **argv) {
  size_t size = 1;
  uint64_t iters = 1000000;
  int threads = 1;
  int touch = 0;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--size") && i + 1 < argc) {
      size = parse_size(argv[++i], "size");
    } else if (!strcmp(argv[i], "--iters") && i + 1 < argc) {
      iters = parse_u64(argv[++i], "iters");
    } else if (!strcmp(argv[i], "--threads") && i + 1 < argc) {
      threads = (int)parse_u64(argv[++i], "threads");
      if (threads < 1) threads = 1;
    } else if (!strcmp(argv[i], "--touch")) {
      touch = 1;
    } else {
      usage(argv[0]);
    }
  }

  pthread_t *tids = calloc((size_t)threads, sizeof(*tids));
  worker_arg_t *args = calloc((size_t)threads, sizeof(*args));
  if (!tids || !args) {
    fprintf(stderr, "calloc failed\n");
    return 1;
  }

  volatile uint64_t sink = 0;
  for (int i = 0; i < threads; i++) {
    args[i].cfg.size = size;
    args[i].cfg.iters = iters;
    args[i].cfg.touch = touch;
    args[i].cfg.sink = &sink;
  }

  uint64_t t0 = now_ns();
  for (int i = 0; i < threads; i++) {
    if (pthread_create(&tids[i], NULL, worker, &args[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }
  for (int i = 0; i < threads; i++) pthread_join(tids[i], NULL);
  uint64_t t1 = now_ns();

  uint64_t total_ops = iters * (uint64_t)threads;
  uint64_t dt = (t1 - t0);

  double sec = (double)dt / 1e9;
  double ops_s = total_ops / sec;
  double ns_op = (double)dt / (double)total_ops;

  printf("size=%zu iters/thread=%" PRIu64 " threads=%d touch=%d\n",
         size, iters, threads, touch);
  printf("total_ops=%" PRIu64 " time=%.6f s  %.0f ops/s  %.1f ns/op\n",
         total_ops, sec, ops_s, ns_op);
  printf("sink=%" PRIu64 "\n", (uint64_t)sink);

  free(tids);
  free(args);
  return 0;
}

/*
### Benchmark (측정)

#### 1) 벤치 빌드
gcc -O2 -pthread bench_malloc.c -o bench_malloc#### 2) baseline(glibc) vs ft_malloc 비교(Throughput)
- TINY(1B), SMALL(200B), LARGE(2000B) 패턴으로 비교:

# baseline (glibc)
./bench_malloc --size 1 --iters 1000000 --threads 1
./bench_malloc --size 200 --iters 1000000 --threads 1
./bench_malloc --size 2000 --iters 200000 --threads 1 --touch
# ft_malloc (LD_PRELOAD)
make
export HOSTTYPE="$(uname -m)_$(uname -s)"

LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 1 --iters 1000000 --threads 1
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 200 --iters 1000000 --threads 1
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 2000 --iters 200000 --threads 1 --touch#### 3) 멀티스레드 확장성(전역 락 영향 수치화)
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 1 --iters 500000 --threads 1
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 1 --iters 500000 --threads 2
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 1 --iters 500000 --threads 4
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ./bench_malloc --size 1 --iters 500000 --threads 8#### 4) mmap/munmap 호출 수(설계 증명)
- 기대:
  - TINY/SMALL: 초기 `mmap`은 매우 적고(보통 1회), 반복 alloc/free 동안 추가 `mmap`이 거의 없음
  - LARGE: alloc마다 `mmap`, free마다 `munmap`

strace -c -e mmap,munmap \
  env LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so \
  ./bench_malloc --size 1 --iters 1000000 --threads 1

strace -c -e mmap,munmap \
  env LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so \
  ./bench_malloc --size 2000 --iters 200000 --threads 1 --touch
*/