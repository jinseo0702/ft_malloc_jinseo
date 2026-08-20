# ft_malloc_jinseo

`ft_malloc_jinseo`는 메모리 할당기의 동작을 이해하기 위해 만든 학습용 user-space allocator입니다. `LD_PRELOAD`로 `malloc`, `free`, `realloc`, `calloc`을 interposition하고 내부 저장 공간은 `mmap`/`munmap`으로 관리합니다.

## 만든 이유

범용 allocator의 성능을 재현하기보다 요청 크기를 분류하고, 빈 블록을 찾고, metadata와 실제 mapping의 생명주기를 관리하는 과정을 직접 구현하는 데 목적을 두었습니다. 그래서 각 class의 동시 할당 수를 100개로 제한하고 구조를 관찰하기 쉽게 유지했습니다.

## 핵심 기능

- libc 이름의 네 API를 `ft_malloc`, `ft_free`, `ft_realloc`, `ft_calloc2`로 전달합니다.
- TINY는 최대 80B payload를 96B 고정 slot 100개와 bitmap으로 관리합니다.
- SMALL은 최대 496B payload를 512B 고정 slot 100개와 bitmap으로 관리합니다.
- LARGE는 allocation마다 page 크기로 올림한 별도 mapping을 만들고 `free`에서 즉시 `munmap`합니다.
- 각 allocation은 `[METADATA_t][client payload]` 형태이며 payload 주소를 호출자에게 반환합니다.
- `show_alloc_mem`으로 class별 live allocation과 요청 byte 합계를 출력합니다.

## 동작 구조

```text
malloc/free/realloc/calloc
  -> src/ft_override.c interposition
  -> ft_malloc/ft_free/ft_realloc/ft_calloc2
  -> global mutex lock
  -> *_impl
       1..80B   : TINY bitmap의 첫 free slot
       81..496B : SMALL bitmap의 첫 free slot
       497B..   : allocation별 mmap
  -> global mutex unlock
```

TINY/SMALL arena는 첫 요청 때 한 번 lazy mapping하고 slot이 비어도 유지합니다. LARGE mapping은 개별 소유로 추적해 해제 시 운영체제에 반환합니다.

## 설계하면서 고민한 점

- 네 allocation API의 바깥 경계에서 mutex를 한 번 잡고, 내부 `*_impl`은 다시 lock하지 않도록 분리했습니다. `realloc`과 `calloc`이 내부 할당 동작을 재사용할 때 nested lock을 피하기 위한 구조입니다.
- bitmap의 첫 빈 bit를 재사용하는 고정 slot 정책으로 split/coalesce 없이 allocation 원리를 드러냈습니다.
- TINY/SMALL은 반복 사용을 위해 유지하고, LARGE는 필요한 page를 독립적으로 얻고 바로 반납하는 정책을 선택했습니다.
- 100개 제한은 범용성이나 성능 목표가 아니라 학습 범위를 통제하기 위한 의도적 선택입니다.

## Build

```bash
make
```

현재 host 기준 shared library `libft_malloc_$(uname -m)_$(uname -s).so`가 생성됩니다. 직접 `ft_*` API에 link할 정적 library가 필요하면 다음 target을 사용합니다.

```bash
make static
```

## Usage

Linux dynamic loader를 통해 한 workload에 allocator를 적용합니다.

```bash
HOSTTYPE="$(uname -m)_$(uname -s)"
LD_PRELOAD="$PWD/libft_malloc_${HOSTTYPE}.so" <program> [args...]
```

고정 slot 수가 작은 학습용 구현이므로 먼저 작은 workload로 동작을 확인하는 편이 적합합니다.

저장소의 정적 API 테스트는 다음 순서로 빌드하고 실행합니다.

```bash
make static
make -C test
./test/testMalloc
```

## 검증 결과

아래 수치는 allocator와 맞지 않는 과거 루트 PROFILE B가 아니라 `portfolio_audit/profile_a` 결과입니다.

| 범위 | 결과 |
|---|---|
| 비교 기준 | C allocation contract, 구현의 class/capacity 정책, `mmap`/`munmap` lifecycle |
| 실행 case | 30 |
| 분류 | PASS 28 · PARTIAL 1 · FAIL 1 · CRASH 0 |
| 대표 관찰 | 80/81/496/497B 경계, class별 100개 capacity/recovery, 4 threads × 500 operations를 확인 |

전체 결과: [`portfolio_audit/profile_a/test_results.md`](portfolio_audit/profile_a/test_results.md)

## 확인된 한계

- A28은 `show_alloc_mem`의 초기화되지 않은 TINY loop counter 때문에 live allocation과 합계를 누락한 진단 정확성 문제입니다. 10회 중 8회가 기대한 두 snapshot을 모두 충족하지 못했으며, core allocation 결과의 실패로 관찰되지는 않았습니다.
- `show_alloc_mem`은 allocator mutex를 잡지 않습니다. 동시 allocate/free 상황에서도 안전한 진단이 원래 의도였지만, 현재 구현의 concurrent snapshot 안전성은 검증되지 않았습니다.
- A30은 crafted foreign pointer가 10/10회 live bitmap slot을 지워 alias를 만든 robustness limitation이며 double free도 감지하지 않습니다. 일반 C allocation contract 밖에서는 `PARTIAL (NON-CONTRACT)`이지만, invalid/double-free 방어가 원래 프로젝트 요구사항이었으므로 그 기준에서는 아직 충족하지 못한 항목입니다.

## 상세 문서

- [PROFILE A 개요](portfolio_audit/profile_a/README.md)
- [Architecture](portfolio_audit/profile_a/architecture.md)
- [CHECKPOINT B](portfolio_audit/profile_a/checkpoint_b.md)
- [Test results](portfolio_audit/profile_a/test_results.md)
- [Failure analysis](portfolio_audit/profile_a/failures.md)
- [Design rationale](portfolio_audit/profile_a/design_rationale.md)
