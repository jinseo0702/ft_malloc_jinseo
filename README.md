## malloc_jinseo

glibc의 `malloc/free/realloc/calloc`을 **LD_PRELOAD로 오버라이드**하는 학습용 메모리 할당기입니다.  
내부적으로 `mmap/munmap`을 사용하며, 작은 크기는 **TINY/SMALL 고정 슬롯 + 비트맵**, 큰 크기는 **LARGE 직접 매핑** 방식으로 처리합니다.

### Build

```bash
make
```

- **(동적 라이브러리)**: `libft_malloc_$(uname -m)_$(uname -s).so`
- 정적 라이브러리도 필요하면:

```bash
make static
```

### LD_PRELOAD

```bash
make
export HOSTTYPE="$(uname -m)_$(uname -s)"
LD_PRELOAD=./libft_malloc_${HOSTTYPE}.so ls
```

### API

- **Override 레이어**: `src/ft_override.c`
  - `malloc/free/realloc/calloc` → `ft_malloc/ft_free/ft_realloc/ft_calloc2`
- **Public API**: `include/ft_malloc.h`
  - `void *ft_malloc(size_t size)`
  - `void  ft_free(void *ptr)`
  - `void *ft_realloc(void *ptr, size_t size)`
  - `void *ft_calloc2(size_t nmemb, size_t size)`
  - `void  show_alloc_mem(void)`

### 호출 흐름

```
user code
  └─ malloc/free/realloc/calloc
       └─ (LD_PRELOAD) src/ft_override.c 에서 override
            └─ ft_malloc / ft_free / ft_realloc / ft_calloc2
                 ├─ manage_free_mutex_lock(&manage_free_mutex)
                 ├─ *_impl(...)  // 실제 로직(공유 상태 접근)
                 └─ manage_free_mutex_unlock(&manage_free_mutex)
```

### 메모리 모델

- **Zone 구분(현재 구현 기준)**  
  - TINY: `size <= (TINY - METASIZE)` → `TINY=96`, `METASIZE=16`이라서 **최대 80바이트**
  - SMALL: `size <= (SMALL - METASIZE)` → `SMALL=512`라서 **최대 496바이트**
  - LARGE: 그 외

- **전역 상태**: `manage_free` (`FREEDATA_t`)
  - `_tiny`, `_small`: TINY/SMALL 영역 시작 주소
  - `tiny[16]`, `small[16]`: 100슬롯을 관리하기 위한 비트맵(16바이트 * 8비트 = 128비트 중 100개 사용)
  - `large[101]`, `large_cnt`: LARGE 매핑 목록(최대 100개 수준)

- **메타데이터 레이아웃**
  - 각 블록은 `[METADATA_t][client memory...]` 형태
  - 반환 포인터는 항상 `meta + 1` (메타데이터 바로 뒤를 클라이언트에게 전달)

### Numbers

| 항목 | 값(현재 구현) | 근거/메모 |
|---|---:|---|
| `TINY` | 96 B | block size |
| `SMALL` | 512 B | block size |
| `METASIZE` | 16 B | `METADATA_t` 오버헤드 |
| **TINY payload max** | **80 B** | `TINY - METASIZE = 96 - 16` |
| **SMALL payload max** | **496 B** | `SMALL - METASIZE = 512 - 16` |
| **TINY 슬롯 수** | **100** | `init_meta_data`/비트맵 스캔이 100개 기준 |
| **SMALL 슬롯 수** | **100** | 동일 |
| **LARGE 최대 매핑 수** | **100** | `large_cnt < 100` |

| 항목 | 값(현재 구현) | 근거/메모 |
|---|---:|---|
| TINY bitmap | 16 B (=128 bit) | `tiny[16]` |
| SMALL bitmap | 16 B (=128 bit) | `small[16]` |
| 실제 관리 슬롯 | 100 bit | 100개까지만 사용 |
| 실제 스캔 범위 | 13 B (=104 bit) | `idx < 0x0D`, `cnt >= 100`에서 컷 |

| 항목 | 계산 | 값 |
|---|---|---:|
| TINY arena(정렬 전) | `96 * 100` | 9600 B |
| SMALL arena(정렬 전) | `512 * 100` | 51200 B |
| 최소 요청 오버헤드 | `16 / (16+1)` | 94.1% |
| TINY 최대 오버헤드 | `16 / (16+80)` | 16.7% |
| SMALL 최대 오버헤드 | `16 / (16+496)` | 3.125% |

| 케이스 | 시스템콜(대략) |
|---|---|
| TINY | 첫 요청에 `mmap` 1회, 이후 100슬롯 소진 전까지 추가 `mmap` 없음 |
| SMALL | 첫 요청에 `mmap` 1회, 이후 100슬롯 소진 전까지 추가 `mmap` 없음 |
| LARGE | 할당마다 `mmap` 1회, 해제마다 `munmap` 1회 |
| 페이지 크기 조회 | `sysconf(_SC_PAGESIZE)` 캐시로 최대 1회 |

### malloc_impl tree

```
malloc_impl(size_t size)
│
├─ [1] 크기 검증
│   └─ size < 1 → size = 1로 설정
│
├─ [2] 크기 분기
│   │
│   ├─ [경로 A] size <= (TINY - METASIZE)
│   │   │
│   │   ├─ [A-1] 첫 할당 여부 확인 (!tiny_flag)
│   │   │   ├─ calculater_page_size(TINY)
│   │   │   │   └─ return_sc_pagesize()
│   │   │   ├─ mmap() → manage_free._tiny 할당
│   │   │   ├─ init_meta_data(_tiny, TINY)
│   │   │   └─ tiny_flag = 0x01 설정
│   │   │
│   │   ├─ [A-2] 빈 공간 찾기
│   │   │   └─ find_free_zone(TINY)
│   │   │       └─ zone == 0xFF → return NULL (공간 없음)
│   │   │
│   │   ├─ [A-3] 인덱스 계산
│   │   │   ├─ front = zone >> 4
│   │   │   ├─ back = zone & 0x0F
│   │   │   └─ index = front * 8 + back
│   │   │
│   │   └─ [A-4] 메타데이터 설정 후 반환
│   │       ├─ temp = &manage_free._tiny[index]
│   │       ├─ temp->client_size = size
│   │       └─ return (temp + 1)  [메타데이터 다음 주소 반환]
│   │
│   ├─ [경로 B] size <= (SMALL - METASIZE)
│   │   │
│   │   ├─ [B-1] 첫 할당 여부 확인 (!small_flag)
│   │   │   ├─ calculater_page_size(SMALL)
│   │   │   │   └─ return_sc_pagesize()
│   │   │   ├─ mmap() → manage_free._small 할당
│   │   │   ├─ init_meta_data(_small, SMALL)
│   │   │   └─ small_flag = 0x01 설정
│   │   │
│   │   ├─ [B-2] 빈 공간 찾기
│   │   │   └─ find_free_zone(SMALL)
│   │   │       └─ zone == 0xFF → return NULL (공간 없음)
│   │   │
│   │   ├─ [B-3] 인덱스 계산
│   │   │   ├─ front = zone >> 4
│   │   │   ├─ back = zone & 0x0F
│   │   │   └─ index = front * 8 + back
│   │   │
│   │   └─ [B-4] 메타데이터 설정 후 반환
│   │       ├─ temp = &manage_free._small[index]
│   │       ├─ temp->client_size = size
│   │       └─ return (temp + 1)  [메타데이터 다음 주소 반환]
│   │
│   └─ [경로 C] LARGE 존
│       │
│       ├─ [C-1] large_cnt < 100 확인
│       │   └─ 아니면 → return NULL
│       │
│       ├─ [C-2] 첫 할당 여부 확인 (!large_flag)
│       │   ├─ ft_memset(manage_free.large, 0, ...)
│       │   └─ large_flag = 0x01 설정
│       │
│       ├─ [C-3] 페이지 크기 계산
│       │   └─ calculater_page_size(size + METASIZE)
│       │       └─ return_sc_pagesize()
│       │
│       ├─ [C-4] 메모리 할당
│       │   ├─ mmap() → ptr 할당
│       │   └─ ptr == MAP_FAILED → return NULL
│       │
│       ├─ [C-5] 메타데이터 초기화
│       │   └─ init_meta_data(ptr, page_size)
│       │
│       └─ [C-6] 메타데이터 설정 후 반환
│           ├─ temp = (METADATA_t *)ptr
│           ├─ temp->client_size = size
│           ├─ large_cnt++
│           └─ return (temp + 1)  [메타데이터 다음 주소 반환]
│
└─ [3] 모든 경로 실패 시
    └─ return NULL
```

### Thread-safety

- `manage_free` 및 관련 상태(비트맵/large 리스트 등)는 **전역 뮤텍스(`manage_free_mutex`)로 보호**합니다.
- 외부 API(`ft_malloc/ft_free/ft_realloc/ft_calloc2`)에서만 락을 잡고, 내부 `*_impl`은 락을 다시 잡지 않는 구조입니다.

### Test

`test/Makefile`은 **정적 라이브러리(`../libft_malloc.a`)가 이미 존재한다는 전제**로 링크합니다.

```bash
make static
(cd test && make)
./test/testMalloc
```

### 제한/주의사항

- **TINY/SMALL 슬롯 수는 100개**까지만 관리합니다(비트맵 스캔도 100개 기준).
- **LARGE는 100개**까지만 관리합니다(`large_cnt < 100`).
- `LD_PRELOAD`로 시스템 바이너리에 적용 시, 출력/동작이 많아질 수 있으니 작은 프로그램부터 확인하는 것을 권장합니다.