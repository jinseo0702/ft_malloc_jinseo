#include "../include/ft_malloc.h"

//impl은 implementation(구현)의 줄임말입니다. 보통 함수 이름에 붙여 “내부 구현 함수”라는 뜻으로 씁니다.
FREEDATA_t manage_free;
static pthread_mutex_t manage_free_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
multi thread 를 보호하기 위해서 mutex 작업을 실시한다.
static 으로 선언된 변수들은 전부 datarace 가 발생 할 수 있다. ....
현재 내코드의 ft_malloc_ver.0.1.c 을 보면 manage_free 의 동작은 크게 네가지로 볼 수 있다.
- 주소부터 시작해서 mmap 에서 할당된 memory page의 주소를 찍기
- meta data 안에 있는 data 전달하기
- 관리하는 비트맵 on/off 시키기
- mmap에서 할당한 메모리주소 받기 이다.

그럼 mutex를 걸어야하 할때 반복되는 작업을 생각을 해보도록
일단 manage_free 가 가지고 있는 FREEDATA_t를 한번 분석해보자
typedef struct FREEDATA_s {
  BLOCK96_t *_tiny;
  BLOCK512_t *_small;
  char tiny[16];
  char small[16];
  unsigned long ;
  int large_cnt;
} FREEDATA_t;

여기서 읽기만하는 정보가 있는가?
 - NO
전부 READ / WRITE 를 반복한다.
공통적으로 동작하는 과정을 세밀하게 분석을 하고 최대한 반복되는 최상단 추상적인 방법을 생각해보자.
일단 구조체 멤버들이 어떤 작업을 하고 있는지 구분한다.
 - _tiny
  * 사용되는 함수 
      void show_alloc_mem();
      void *ft_malloc(size_t size);
  * 하는 작업
      show_alloc_mem 안에서는 index 에 맞는 BLOCK96_t를 가지와서 METADATA_t 로변환후 METADATA_t 정보를 출력 하는 작업을 한다.
      또한 total size를 올리기 위해서 읽기 잡업을 실시한다.
       - show_alloc_mem 에서는 READ 작업만 실행
      //처음 할당 받을때 만
      ft_malloc 안에서는 처음 100만큼 할당받을때 mmap 이 전달한 첫 주소를 기억한다. -> init_meta_data((void *)manage_free._tiny, TINY); 로 전달
      처음받은 mmap 의 data 를 인덱스를 찾고(BLOCK96_t 기준 index) METADATA_t 형변환을 한다음 meta data 를 입력하는 작업을 한다.
       - init_meta_data 에서는 WRITE 작업을 실시한다 
      // 항상
      METADATA_t *temp = (METADATA_t *)&manage_free._tiny[index];
      temp->client_size = size;
      return ((void *)(temp + 1));
      처음 이후 부터는 빈주소로 할당된 곳 에서 전달한다.
       - ft_malloc 에서는 READ / WRITE 작업을 실시한다.

- _small
  * 사용되는 함수 
      void show_alloc_mem();
      void *ft_malloc(size_t size);
  * 하는 작업
      show_alloc_mem 안에서는 index 에 맞는 BLOCK512_s를 가지와서 METADATA_t 로변환후 METADATA_t 정보를 출력 하는 작업을 한다.
      또한 total size를 올리기 위해서 읽기 잡업을 실시한다.
       - show_alloc_mem 에서는 READ 작업만 실행
      //처음 할당 받을때 만
      ft_malloc 안에서는 처음 100만큼 할당받을때 mmap 이 전달한 첫 주소를 기억한다. -> init_meta_data((void *)manage_free._small, SMALL); 로 전달
      처음받은 mmap 의 data 를 인덱스를 찾고(BLOCK512_s 기준 index) METADATA_t 형변환을 한다음 meta data 를 입력하는 작업을 한다.
       - init_meta_data 에서는 WRITE 작업을 실시한다 
      // 항상
      METADATA_t *temp = (METADATA_t *)&manage_free._small[index];
      temp->client_size = size;
      return ((void *)(temp + 1));
      처음 이후 부터는 빈주소로 할당된 곳 에서 전달한다.
       - ft_malloc 에서는 READ / WRITE 작업을 실시한다.

- tiny[16]
  * 사용되는 함수
    void show_alloc_mem();
    unsigned char find_free_zone(size_t size_type);
    void ft_free(void *ptr);
  * 하는 작업
    show_alloc_mem 안에서는 idx 에 맞는 char 형을 반환하는 작업을 한다 -> find = manage_free.tiny[idx] & (1 << fos);
    - show_alloc_mem 에서는 READ 작업만 실시한다.
    find_free_zone 에서는 idx 에 맞는 char 형을 반환하는 작업을 한다 -> find = manage_free.tiny[idx] & (1 << fos);
    또한 직접 값을 변경하는 작업도 실시한다.(할당한 부분을 체크한다.) -> manage_free.tiny[idx] |= (1 << fos);
     - find_free_zone 에서는 READ / WRITE 작업을 실시한다.
    ft_free 에서는 해제한 부분의 값을 변경한다. -> manage_free.tiny[index] &= ~(1 << meta->position);
     - ft_free 에서는 WRITE 작업을 실시한다.

- small[16]
  * 사용되는 함수
    void show_alloc_mem();
    unsigned char find_free_zone(size_t size_type);
    void ft_free(void *ptr);
  * 하는 작업
    show_alloc_mem 안에서는 idx 에 맞는 char 형을 반환하는 작업을 한다 -> find = manage_free.small[idx] & (1 << fos);
    - show_alloc_mem 에서는 READ 작업만 실시한다.
    find_free_zone 에서는 idx 에 맞는 char 형을 반환하는 작업을 한다 -> find = manage_free.small[idx] & (1 << fos);
    또한 직접 값을 변경하는 작업도 실시한다.(할당한 부분을 체크한다.) -> manage_free.small[idx] |= (1 << fos);
     - find_free_zone 에서는 READ / WRITE 작업을 실시한다.
    ft_free 에서는 해제한 부분의 값을 변경한다. -> manage_free.small[index] &= ~(1 << meta->position);
     - ft_free 에서는 WRITE 작업을 실시한다.

- large[101]
  * 사용되는 함수
    void show_alloc_mem();
    void init_meta_data(void *ptr ,size_t size);
    void *ft_malloc(size_t size);
    void ft_free(void *ptr);
  * 하는 작업
    show_alloc_mem 안에서는 사용중인 index 에 맞는 배열안의 주소를 가지고 와서 로 METADATA_t 변환후 METADATA_t 정보를 출력 하는 작업을 한다.
     - show_alloc_mem 안에서는 READ 작업만 실시한다.
    init_meta_data 에서는 index 에맞는 값을 읽고 비어 있다면 주소 값을 넣는다.
     - init_meta_data 에서는 READ / WRITE 작업을 실시한다.
    ft_malloc 에서는 flag 가 0일때 value를 memset 으로 초기화 해주는 작업을 실시한다.
     - ft_malloc 에서는 READ / WRITE 작업을 실시한다.
    ft_free 에서는 반납한 메모리의 주소가 들어 있는 값 을 0으로 초기화 해준다.

- large_cnt
  * 사용되는 함수
    void *ft_malloc(size_t size);
    void ft_free(void *ptr);
  * 하는 작업
    ft_malloc 에서는 large의 cnt 읽어서 범위를 확인 해준다.
    매변 할당이 된다면 large_cnt를 증가 시켜준다.
     - ft_malloc 에서는 READ / WRITE 작업을 실시한다.
    ft_free 에서는 사용이 끝났다면 -1 을 해줘서 cnt 를 감소시켜준다.
  
  별표 static 변수를 사용하는 장소
    * 사용되는 함수
    long return_sc_pagesize();
    long calculater_page_size(size_t size);
    void *ft_malloc(size_t size);
    * 사용되는 변수
      - long return_sc_pagesize();
          - static long _sc_page_size_cache;
      - long calculater_page_size(size_t size);
          - static int flag_tiny;
          - static int flag_small;
      - void *ft_malloc(size_t size);
          - static char tiny_flag, small_flag, large_flag;

  ```MARKDOWN
  // AI 가 추천해준방법

  ### 요약
- **현재 상황 정리**
  - `manage_free` 전역 상태(`_tiny`, `_small`, `tiny[]`, `small[]`, `large[]`, `large_cnt`)와 여러 `static` 플래그들이 모두 R/W되어 멀티스레드에서 데이터 레이스 위험이 큼.
  - 함수별 역할: 비트맵 on/off, 메타데이터 갱신, `mmap/munmap` 호출, 통계 출력 등 대부분이 공유 상태를 건드림.

- **권장 접근(정확성 우선)**
  - **전역 락 한 번에 보호**: `ft_malloc`, `ft_free`, `ft_realloc`, `show_alloc_mem` 본문 전체를 `manage_free_mutex`로 감싸 정확성부터 확보.
  - **조기 반환은 `goto out` 패턴**으로 정리해 언락 누락 방지.
  - `find_free_zone`, `init_meta_data`는 “호출자가 이미 전역 락을 잡고 있음”을 전제로 내부에서 별도 락을 잡지 않음.

- **부가 락(정적 캐시 보호)**
  - `return_sc_pagesize`의 `_sc_page_size_cache`는 경쟁 초기화 방지를 위해 작은 락(`page_mutex`)으로 보호.
  - `calculater_page_size`의 `flag_tiny`, `flag_small`도 작은 락(`calc_mutex`)으로 보호.
  - 락 순서 규칙(교착 방지): `manage_free_mutex` → `calc_mutex` → `page_mutex` 방향만 허용. 반대로는 금지.

- **매크로에 `do { ... } while (0)`을 쓰는 이유**
  - 여러 문장짜리 매크로를 “한 문장처럼” 만들어 `if (...) MACRO(); else ...`에서의 매달린 `else` 문제 방지.
  - 블록 스코프 부여(임시 변수 누수 방지), 성능 오버헤드 없음.

- **성능 고려 팁**
  - `show_alloc_mem`은 출력이 길 수 있으니, 필요 시 락 안에서 스냅샷을 로컬로 떠서 락을 빨리 해제한 뒤 출력.
  - 성능 이슈가 실제로 생기면 2단계에서 락 세분화(`tiny_mutex`, `small_mutex`, `large_mutex`)를 검토. 반드시 고정된 락 획득 순서 유지.

- **테스트**
  - TSAN(`-fsanitize=thread`)로 데이터 레이스 확인.
  - 멀티스레드 스트레스(무작위 `malloc/free/realloc/show_alloc_mem`)로 교착/경합 점검.

- **질문에 대한 핵심 답변**
  - 지금 구조에선 “반복되는 작은 락을 많이 만드는 것”보다, **API 단위로 한 번에 전역 락**이 안전하고 단순하며 유지보수에 유리함. 성능 문제가 드러나면 그때 세분화.

  ```

AI 는 지금 말하는게 내 코드는 전역 구조체를 사용하는데 이 구조체를 안쓰는 곳이 없기때문에 최대 장소인 ft_malloc 을 잠궈버리는 빙법을 추천한다.
이렇게 하면 물론 thread 간의 data race는 발생하지 않지만 매우 느려지게 된다.


```markdown

### static inline 함수의 장단점
- 장점
  - **타입 안전**: 인자/리턴에 대해 컴파일 타임 타입 체크.
  - **오버헤드 최소화**: 최적화 시 인라인되어 호출 오버헤드 거의 없음.
  - **디버깅 용이**: 심볼/스택트레이스에 보이고 브레이크포인트 설정 가능.
  - **단 한 번 평가**: 인자가 여러 번 평가되지 않음(부작용 안전).
  - **헤더에 두기 좋음**: `static`으로 내부 연결; 여러 TU에서 중복 정의 문제 없이 사용 가능.

- 단점
  - **타입 고정**: 제네릭하게 쓰려면 오버로드(함수 여러 개)나 `void *`/매크로 등 보조 수단 필요.
  - **바이너리 크기 증가 가능성**: 여러 TU에서 각각 인라인/발행되면 코드 복제 가능(보통 작은 함수는 영향 미미).
  - **문장 합성 어려움**: 여러 문장을 “한 문장처럼” 만들기(특히 `if (...) foo(); else ...`)는 매크로만큼 유연하진 않음.

### 매크로(예: LOCK/UNLOCK)의 장단점
- 장점
  - **문장처럼 사용**: `do { ... } while (0)`로 여러 문장을 한 문장처럼 안전하게 사용 가능.
  - **0 오버헤드**: 텍스트 치환이므로 호출 비용 없음(인라인 함수와 유사).
  - **문자열화/토큰 합치기**: `#`, `##`로 `__FILE__`, `__LINE__`, 인자 이름을 메시지에 쉽게 삽입.
  - **타입 무관**: 어떤 타입 인자도 문법적으로만 맞으면 사용 가능.

- 단점
  - **타입 체크 없음**: 잘못된 인자도 통과; 버그 발견이 늦음.
  - **다중 평가 위험**: 인자에 부작용이 있으면 예기치 않은 동작.
  - **디버깅 불편**: 스택 트레이스/심볼에 안 보임, 브레이크포인트 걸기 어려움.
  - **스코프/이름 오염**: 지역 변수 충돌 위험; 주의 깊은 네이밍/괄호화 필요.

### 비교 요약
- **안전성/가독성**: static inline 우위(타입 체크·디버깅 좋음).
- **유연성(문장 합성·메타프로그래밍)**: 매크로 우위.
- **성능**: 둘 다 사실상 동일(최적화 시 호출 제거). 차이는 미미.
- **제네릭성**: 매크로가 쉽고, inline은 `_Generic`(C11)이나 여러 함수로 우회.

### 실무 가이드
- 락/언락처럼 단순 래핑: static inline 함수 권장.
- 파일/라인을 메시지에 넣거나, 한 문장처럼 합쳐야 하는 복잡 매크로: 매크로 고려.
- 혼용 시 원칙: 가능한 한 static inline 우선, 매크로는 꼭 필요할 때만.

- 한 줄 결론: 타입 안전성과 디버깅을 중시하면 static inline, 문장 합성/메타프로그래밍이 필요하면 매크로.


문제가 생겼다. ft_realloc에서 ft_malloc, ft_free 를 호출 한다. 각 함수는 mutex 를 건다. 하지만 realloc 에서도 
공용으로 보는 구역이 존재 하기때문에 mutex를 걸어야한다. 아.... 


```

AI 가 추천하는방법
```markdown
간단 원리
- 재귀 뮤텍스는 “소유한 스레드와 잠금 횟수”를 내부에 저장합니다.
  - 같은 스레드가 다시 lock하면 카운터만 +1 되고 즉시 통과.
  - unlock은 카운터 −1, 0이 되면 실제로 풀림.
- 일반(기본) 뮤텍스는 소유 스레드가 다시 lock하면 대기에 걸려 자기 자신과 교착됩니다.

static inline로 가능한가요?
- 아니요. 재귀 동작은 뮤텍스 “타입/속성”으로 결정됩니다. `static inline`은 함수 인라이닝일 뿐, 뮤텍스의 동작을 바꾸지 못합니다.
- 즉, 재귀를 쓰려면 반드시 재귀 타입으로 초기화해야 합니다.

사용 예시
```c
// glibc 간단 초기화(리눅스 전용)
static pthread_mutex_t manage_free_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
```

또는 이식성 있게 런타임 초기화:
```c
static pthread_mutex_t manage_free_mutex;

static void init_recursive_mutex(void) {
  pthread_mutexattr_t a;
  pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&manage_free_mutex, &a);
  pthread_mutexattr_destroy(&a);
}
```

참고
- `static inline` 헬퍼로 lock/unlock 래핑은 가능하지만, 재귀 가능 여부는 위 초기화로만 결정됩니다.
```

AI가 추천하는방법
그냥 모든 함수위에서 mutex를 걸고 mutex가 걸린 친구만 그함수 안에서 일을 처리하게 해놓자.
그러니까 multi thread 상황에서 trhead 한개가 작업이 끝날때까지 무식하게 다른 thread 는 작업(연산)을 못하게 만든다.
효율성은 개나줬다.
```markdown

핵심 원리
- 한 임계구역에서 원자적으로 처리: 공유 상태(`manage_free`, 메타데이터, 비트맵, large 리스트 등)를 건드리는 전체 연산(할당→복사→해제)을 하나의 뮤텍스로 감싸 “전부 혹은 전무(all-or-nothing)”로 보이게 합니다. 이렇게 하면 다른 스레드는 중간 상태를 절대 보지 못해 data race/불일치가 사라집니다.
- 내부 구현은 무잠금: 임계구역(락)은 바깥 API에서만 잡고, 실제 로직은 락이 없는 내부 함수로 분리합니다. 그래야 중첩 잠금(교착) 없이 `realloc` 내부에서 `malloc`/`free` 동작을 재사용할 수 있습니다.

개념 구조
```c
// 내부 구현(락 없음, 공용 로직)
static void *malloc_impl(size_t size);
static void  free_impl(void *ptr);
static void *realloc_impl(void *ptr, size_t size);

// 외부 API(락 담당)
void *ft_malloc(size_t size){
  LOCK(manage_free_mutex);
  void *r = malloc_impl(size);
  UNLOCK(manage_free_mutex);
  return r;
}
void ft_free(void *ptr){
  LOCK(manage_free_mutex);
  free_impl(ptr);
  UNLOCK(manage_free_mutex);
}
void *ft_realloc(void *ptr, size_t size){
  LOCK(manage_free_mutex);
  void *r = realloc_impl(ptr, size);  // 내부에서 malloc_impl/free_impl만 호출
  UNLOCK(manage_free_mutex);
  return r;
}
```
- `realloc_impl` 예: `ptr==NULL`이면 `malloc_impl`, `size==0`이면 `free_impl` 후 NULL, 그 외엔 새 블록 `malloc_impl`→`memmove`→`free_impl`.

왜 이렇게 해야 하나
- 데이터 경쟁 방지: 임계구역 밖에서는 공유 상태에 손대지 않으므로 happens-before가 보장됩니다.
- 교착 방지: 내부 구현은 락을 잡지 않으므로 API→API 재호출(중첩 잠금)이 사라집니다.
- 일관성/원자성: `realloc`의 복합 동작이 끊기지 않고 한 번에 끝납니다.

static을 붙이는 이유
- 내부 연결(캡슐화): `malloc_impl/free_impl/realloc_impl`이 현재 `.c` 파일 안에서만 보이게 하여, 외부에서 락 우회 호출을 원천 차단.
- 링크 충돌 방지: 동일 이름의 내부 헬퍼를 다른 파일에 둬도 심볼 충돌 없음.
- 최적화 여지: 같은 번역 단위라 인라이닝/상수 전파 등 최적화가 유리.
- 공개 인터페이스 최소화: 외부에 노출되는 심볼을 `ft_*`로만 제한해 유지보수성↑.

요약
- 락은 “API에서 한 번만” 잡고, 로직은 “무잠금 내부 함수”로 재사용.
- 이렇게 해야 data race/교착 없이 `malloc`/`free`/`realloc`이 한 임계구역에서 원자적으로 수행됩니다.
- `static`은 내부 구현을 숨겨 락 우회와 링크 문제를 막고 최적화에 유리합니다.

추가 질문
 -  근데 이해가 어려운게 있는데 지금 코드 최상단에서 lock 을 걸면 똑같은 상황아니야?
답변
아니요. “최상단에서 한 번만” 잠그는 건 맞지만, 그 안에서 무엇을 호출하느냐가 핵심입니다.

- 올바른 경우(권장)
  - 최상단에서 전역 락 한 번 획득 → 내부 무잠금 구현만 호출 → 마지막에 해제.
  ```c
  LOCK(manage_free_mutex);
  void *r = realloc_impl(ptr, size); // 내부에서 malloc_impl/free_impl만 호출(무잠금)
  UNLOCK(manage_free_mutex);
  ```

- 데드락 나는 경우
  - 최상단에서 락을 잡은 뒤, 같은 락을 다시 잡는 API(`ft_malloc/ft_free`)를 호출하면 비재귀 뮤텍스에서는 교착됩니다.
  ```c
  LOCK(manage_free_mutex);
  void *q = ft_malloc(n); // 내부에서 또 LOCK → 교착
  UNLOCK(manage_free_mutex);
  ```

- 레이스 나는 경우
  - 중간에 언락하고 `ft_malloc/ft_free`를 호출했다가 다시 락을 잡는 패턴은, 그 사이에 다른 스레드가 `ptr`/메타데이터를 바꿀 수 있어 UAF/이중해제/불일치가 생깁니다.
  ```c
  LOCK(manage_free_mutex);
  // ... meta 확인 ...
  UNLOCK(manage_free_mutex);
  q = ft_malloc(n); // 그 사이 다른 스레드가 ptr 해제/변경 가능 → 위험
  LOCK(manage_free_mutex);
  ```

- 예외적 대안
  - 전역 락을 재귀 뮤텍스로 초기화하면, 최상단에서 락을 잡은 뒤 `ft_malloc/ft_free`(안에서도 같은 락) 호출이 가능합니다. 다만 구조적으로는 “API만 잠금 + 내부 무잠금 구현”이 더 깔끔합니다.

요점
- “최상단에서 한 번만” 잠그는 건 맞고, 그 안에서는 “락을 다시 잡지 않는 내부 구현(impl)”만 호출해야 같은 상황이 아닙니다(= 안전).

```

```markdown
## 지금까지 LD_PRELOAD=../libft_malloc_x86_64_Linux.so ls 하면 실패한이유
ft_malloc_ver.0.1.c 랑 비교하면서 뭐가 다른지 보면 이해가 될 것.

- 경계/존 선택 불일치: `malloc_impl`은 크기로 TINY/SMALL 분기, `find_free_zone`은 크기 비교(< TINY/< SMALL)로 비트맵 선택 → 95/96B 등 경계에서 NULL. 해결: `find_free_zone`을 존 타입(TINY/SMALL) 인자로 받고, 분기 비교는 `<=`로 통일.
- 잘못된 페이지 계산 호출: `malloc_impl` 초반에 `calculater_page_size(size)` 호출로, TINY/SMALL 플래그 로직에 걸려 `-1` 반환 → 즉시 NULL. 해결: 이 호출 제거(large 경로에서만 페이지 계산).
- 잠금 설계 문제: `realloc`이 `ft_malloc/ft_free`를 호출하면 중첩 잠금으로 교착. 해결: API에서만 전역 락, 내부 `*_impl`은 무잠금으로 분리(또는 재귀 뮤텍스 사용).
- 언락 래퍼 버그: `manage_free_mutex_unlock`가 실수로 `pthread_mutex_lock` 호출 → 교착. 해결: `pthread_mutex_unlock` 호출로 수정.
- 기타 정리: `print_error`는 `const char *`로 수정(문자열 리터럴), `show_alloc_mem` 스냅샷 또는 전체 잠금, 재진입 위험 함수 사용 자제, 반환 포인터 정렬(가능하면 16B) 확인.


```

*/


static void *malloc_impl(size_t size);
static void *realloc_impl(void *ptr, size_t size);
static void	*callo_imp(size_t nmemb, size_t size);
static void  free_impl(void *ptr);

void *ft_malloc(size_t size){
  manage_free_mutex_lock(&manage_free_mutex);
  void *reptr = malloc_impl(size);
  manage_free_mutex_unlock(&manage_free_mutex);
  return (reptr);
}

void ft_free(void *ptr){
  manage_free_mutex_lock(&manage_free_mutex);
  free_impl(ptr);
  manage_free_mutex_unlock(&manage_free_mutex);
}

void *ft_realloc(void *ptr, size_t size){
  manage_free_mutex_lock(&manage_free_mutex);
  void *reptr = realloc_impl(ptr, size);
  manage_free_mutex_unlock(&manage_free_mutex);
  return (reptr);
}

void	*ft_calloc2(size_t nmemb, size_t size){
  manage_free_mutex_lock(&manage_free_mutex);
  void *reptr = callo_imp(nmemb, size);
  manage_free_mutex_unlock(&manage_free_mutex);
  return (reptr);
}

void show_alloc_mem(){
  size_t total_size = 0;
  ft_printf("TINY : %p\n", manage_free._tiny);
  unsigned char find, cnt, cnt2 = 0x00;
    for (int idx = 0x00; idx < 0x0D; idx++) {
      for (int fos = 0x00; fos < 0x08 ; fos++) {
        if (cnt >= 100) {
          break;
        }
        find = manage_free.tiny[idx] & (1 << fos);
        if (find != 0x00){
          METADATA_t *meta = (METADATA_t *)&manage_free._tiny[(idx * 8) + fos];
          ft_printf("%P - %P : %d byte tiny zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, (meta->size & ~INDEX_MASK));
        // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
          total_size += meta->client_size;
        }
        cnt++;
      }
    }
  ft_printf("SMALL : %p\n", manage_free._small);
  for (int idx = 0x00; idx < 0x0D; idx++) {
    for (int fos = 0x00; fos < 0x08 ; fos++) {
      if (cnt2 >= 100) {
        break;
      }
      find = manage_free.small[idx] & (1 << fos);
      if (find != 0x00){
        METADATA_t *meta = (METADATA_t *)&manage_free._small[(idx * 8) + fos];
        ft_printf("%P - %P : %d byte small zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, (meta->size & ~INDEX_MASK));
        // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
        total_size += meta->client_size;
      }
      cnt2++;
    }
  }
  ft_printf("LARGE : %P\n", manage_free.large);
  for (int idx = 0; idx < 100; idx++) {
    if (manage_free.large[idx] != 0) {
      METADATA_t *meta = (METADATA_t *)manage_free.large[idx];
      ft_printf("%P - %P : %d byte Large zone is %d \n", meta, (unsigned char *)meta + meta->client_size, meta->client_size, meta->size);
      // ft_printf("%P - %P : %d byte\n", meta, (unsigned char *)meta + meta->client_size, meta->client_size);
      total_size += meta->client_size;
    }
  }
  ft_printf("Total size: %d byte\n", total_size);
}

void print_error(const char *str){
  ft_fprintf(2, "Error %s : %s (errno = %d)\n", str, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

/*
sysconf 는 system의 config 를 가져 오기때문에 많은 비용을 요구한다. 그래서 캐시로 한번 페이지의 단위를 한번 만 가졍온다.
*/
long return_sc_pagesize() {
  static long _sc_page_size_cache;
  int flag = 0x00;
  if (_sc_page_size_cache == 0x00) {
    flag = sysconf(_SC_PAGESIZE);
    if (flag == -1){
      print_error("return_sc_pagesize");
    }
    _sc_page_size_cache = flag;
  }
  return (_sc_page_size_cache);
}

/*
make catch unkwon error flag
flag_tiny and falg_small
use usefull
#define DEFAULTSIZE 0x10 //16
#define TINY 0x60 //96
#define SMALL 0x200 //512
  static int flag_tiny;
  static int flag_small;
  flag를 사용해서 캐시시스템을 이용한다. zone 마다 한번만 page_count 를 받고 한번만 할당할 수 있는 시스템이다.
  page mask 를 사용한 이유는 page count 를 할때 계산시에 99만큼 나오는경우가 있을 수도 있는데 우리는 항상
  100만큼을 보장해야한다. 이경우 미리 (결과값 + page_size -1 )을 해주고 비트맵 마스크로 page_size의 배수를 맞춰준다.
  ex ) [page size 가 5000 일때] 51 * 100 = 5100 이때 5100 / 5000 을하면 결과값이 1 이 나오고 page 1 개가 나온다. 우리가 원하는 사이즈가 보장이 안된다.
  이떄 (51 * 100) + 4999 = 10099 에서 99 를 빼준다. 10000 
  10000 / 5000 = 2 우리는 2개의 page 를 받을 수 있다.
*/
long calculater_page_size(size_t size) {
  static int flag_tiny;
  static int flag_small;
  long PAGE_SIZE;
  long PAGE_COUNT;
  long PAGE_MASK;

  if (!(~flag_tiny & size) || !(~flag_small & size) || size < 1) {
    return (-1);
  }
  PAGE_SIZE = return_sc_pagesize();
  PAGE_MASK = ~(PAGE_SIZE - 1);
  if (size == TINY && !flag_tiny) {
    PAGE_COUNT = ((TINY * 0x64) + (PAGE_SIZE - 1)) & PAGE_MASK;
    flag_tiny |= TINY;
  }
  else if (size == SMALL && !flag_small) {
    PAGE_COUNT = ((SMALL * 0x64) + (PAGE_SIZE - 1)) & PAGE_MASK;
    flag_small |= SMALL;
  }
  else {
    PAGE_COUNT = (size + (PAGE_SIZE - 1)) & PAGE_MASK;
  }
  return (PAGE_COUNT);
}

/*
확장성을 고려해서 아직 error 플레그는 만들지 말자.
페이지 단편화를 해결 할 수 있으니까.
void init_meta_data(void *ptr ,size_t size, int multiful_of_size);
단편화 해결 못할 것 같다. 난 딱 100까지만 보장한다.
idx 의 하위 3개의 비트만 보면 position 이 어디인지 알수 있다.
```c
      index = (idx >> 3);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
      tmp->client_size = 0;
```
idx를 >> 3 만큼밀어 낸 이유는 char 자료형의 bit 는 8개 즉 8의 배수이다.
총 100개의 배열안에 100개만큼의 자료를 초기화 해야하는데
0 ~ 7 : 0 번째 index
8 ~ 15 : 1번째 index
...
96 ~ 103 : 12 번째 index 가 된다. 이때 0110 0000 >> 3 : 0000 1100 = 12 가 나온다.
이렇게 범위를 구한다.
tmp->size = size | index; size 는 무조건 16 이상의 숫자를 보장한다. 즉 하위 4개의 비트는 사용을 한다.
이때 index 를 그 하위 4개의 bit에 저장을 한다.
size : .... 0000 0000 일때 하위 4개의 비트는 몇변째 배열안에 속해있는지 flag를 남긴다 index 는 0 ~ 12 까지의 범위라 2^4 범위 를충족한다.
tmp->position = idx & POS_MASK; position을 알려준다.  즉 N % 8 을 해서 나머지를 알려주는 기법이다.
*/
void init_meta_data(void *ptr ,size_t size){
  if (size == TINY) {
    short index;
    BLOCK96_t *ptr_TINY = (BLOCK96_t *)ptr;
    for (int idx = 0; idx < 100; idx++) {
      METADATA_t *tmp = (METADATA_t *)(ptr_TINY + idx);
      index = (idx >> 3);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
      tmp->client_size = 0;
    }
  }
  else if (size == SMALL) {
    BLOCK512_t *ptr_SAMLL = (BLOCK512_t *)ptr;
    short index;
    for (int idx = 0; idx < 100; idx++) {
      METADATA_t *tmp = (METADATA_t *)(ptr_SAMLL + idx);
      index = (idx >> 3);
      tmp->size = size | index;
      tmp->position = idx & POS_MASK;
      tmp->client_size = 0;
    }
  }
  else if(size > 512) {
    int idx = 0;
    for (; idx < 100; idx++) {
      if(manage_free.large[idx] == 0){
        manage_free.large[idx] = (unsigned long)ptr;
        break;
      }
    }
    METADATA_t *tmp = (METADATA_t *)ptr;
    tmp->size = size;
    tmp->position = idx;
    tmp->client_size = 0;
  }
  else {
    print_error("do Error");
  }
}

/*
나중에 이해하기 쉽고 다른 사람들이 이해하기 쉽도록 변수를 많이 선언했습니다.
result 가 0xFF 이라는건 지금 할당 할 수 있는 공간 이 없다는 의미 입니다.
find : index 를 저장합니다. 8 * 16 의 크기의 char 형 배열에서 몇번째 index 인지 기억합니다.
back : char 자료형의 fos 를 기억합니다. 몇번째 비트 position 인지를 기억합니다.
result : 0000 0000 의 but 를 가지고 있는 flag 로 
front 는 앞의 4개의 비트 (front 의 최대크기는 16 즉 0 ~ 15 4개의 비트 사용)
back 은 뒤 4개의 비트를 사용합니다. (back 의 최대크기는 8 즉 0 ~ 7 3개의 비트 사용)
*/
unsigned char find_free_zone(size_t size_type){
  unsigned char front, back, find, cnt = 0x00;
  unsigned char result = 0xFF;

  if (size_type == TINY) {
    for (int idx = 0x00; idx < 0x0D; idx++) {
      for (int fos = 0x00; fos < 0x08 ; fos++) {
        if (cnt >= 100) {
          return(result);
        }
        find = manage_free.tiny[idx] & (1 << fos);
        if (find == 0x00){
          front = idx;
          back = fos;
          result = (front << 0x04) | back;
          manage_free.tiny[idx] |= (1 << fos);
          return (result);
        }
        cnt++;
      }
    }
  }
  else if(size_type == SMALL){
      for (int idx = 0x00; idx < 0x0D; idx++) {
        for (int fos = 0x00; fos < 0x08 ; fos++) {
          if (cnt >= 100) {
            return(result);
          }
          find = manage_free.small[idx] & (1 << fos);
          if (find == 0x00){
            front = idx;
            back = fos;
            result = (front << 0x04) | back;
            manage_free.small[idx] |= (1 << fos);
            return (result);
          }
          cnt++;
        }
    } 
  }
  return (result);
}

/*
메모리의 욺직임은 내가 만든 BLOCK 의 크기만큼 욺직이기때문에 행복하다.
*/
static void *malloc_impl(size_t size){
  static char tiny_flag, small_flag, large_flag;
  long page_size;


  if (size <  1){
    size = 1;
  }

  //debug
  // static int cnt = 0;
  // ft_printf("cnt : %d\n", cnt);
  // cnt++;
  //debug
  
  // show_alloc_mem();
  // page_size = calculater_page_size(size); //debug
  // if(page_size == -1){
  //   return (NULL);
  // }

  if (size <= (TINY - METASIZE)) {
    if (!tiny_flag){
      page_size = calculater_page_size(TINY);
      if(page_size == -1){
        return (NULL);
      }
      manage_free._tiny = (BLOCK96_t *)mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (manage_free._tiny == MAP_FAILED) {
        return (NULL);
      }
      init_meta_data((void *)manage_free._tiny, TINY);
      tiny_flag = 0x01;
    }
    unsigned char zone, front, back, index = 0x00;
    zone = find_free_zone(TINY);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._tiny[index];
    temp->client_size = size;
    return ((void *)(temp + 1));
  }
  else if (size <= (SMALL - METASIZE)) {
    if (!small_flag) {
      page_size = calculater_page_size(SMALL);
      if(page_size == -1){
        return (NULL);
      }
      manage_free._small = (BLOCK512_t *)mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (manage_free._small == MAP_FAILED) {
        return (NULL);
      }
      init_meta_data((void *)manage_free._small, SMALL);
      small_flag = 0x01;
    }
    unsigned char zone, front, back, index= 0x00;
    zone = find_free_zone(SMALL);
    if (zone == 0xFF) {
      return (NULL);
    }
    front = (zone >> 0x04);
    back = (zone & ((0x01 << 0x04) -1));
    index = (front * 8) + back;
    METADATA_t *temp = (METADATA_t *)&manage_free._small[index];
    temp->client_size = size;
    return ((void *)(temp + 1));
  }
  else  {
    if (manage_free.large_cnt < 100){
      if (large_flag == 0){
        ft_memset(manage_free.large, 0, sizeof(manage_free.large));
        large_flag |= (0x01);
      }
      page_size = calculater_page_size(size + METASIZE);
      if(page_size == -1){
        return (NULL);
      }
      void *ptr = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE , MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
      if (ptr == MAP_FAILED) {
        return (NULL);
      }
      init_meta_data(ptr, page_size);
      METADATA_t *temp = (METADATA_t *)ptr;
      temp->client_size = size;
      manage_free.large_cnt++;
      return ((void *)(temp + 1));
    }
  }
  return (NULL);
}

/*
*/
static void  free_impl(void *ptr){
  int size_type, index = 0;

  if (ptr == NULL){
    return;
  }
  METADATA_t *meta;
  meta = (METADATA_t *)ptr - 1;
  if(meta == NULL){
    return;
  } 
  index = meta->size & INDEX_MASK;
  size_type = meta->size & ~INDEX_MASK;
  if (size_type == TINY) {
    manage_free.tiny[index] &= ~(1 << meta->position);
    // meta->size = 0;
    return ;
  }
  else if (size_type == SMALL) {
    manage_free.small[index] &= ~(1 << meta->position);
    // meta->size = 0;
    return ;
  }
  else if (size_type > 0x200){
    int flag = 0;
    int pos = meta->position;
    size_t len = meta->size;
    manage_free.large[pos] = 0;
    flag = munmap((void *)meta, len);
    if (flag < 0) {
      print_error("munmap Fail");
    }
    manage_free.large_cnt -= 1;
    return ;
  }
  else {
    return ;
  }
}

/*
realloc() 함수는 ptr이 가리키는 메모리 블록의 크기를 size 바이트로 변경합니다. 
내용은 영역의 시작점부터 이전 크기와 새 크기 중 더 작은 값(minimum)까지의 범위에서 변경되지 않은 상태로 유지됩니다. 
만약 새 크기가 이전 크기보다 크다면, 추가된 메모리는 초기화되지 않습니다. 

만약 ptr이 NULL이라면, 그 호출은 모든 size 값에 대해 malloc(size)와 동일합니다;
만약 size가 0이고 ptr이 NULL이 아니라면, 
그 호출은 free(ptr)과 동일합니다. 
ptr이 NULL이 아니라면, 그것은 이전에 malloc(), calloc(), 또는 realloc() 호출에 의해 반환된 값이어야 합니다.
만약 가리키던 영역이 이동되었다면, free(ptr)이 수행됩니다.

같은 zone 이라면 meta data만 변경됩니다.
만약 small zone 에서 tiny 존의 변경이라면 그냥 small zone을 사용하고 meta data 만 변경됩니다.
만약 작은 zone 에서 더 큰 존으로 간다면 data는 보존이되고 작은 zone 의 memory 는 free(*beforezone) 이 되어야한다.

 |  ptr  | size |       do      |
 | NULL  | ?    |  malloc(size) |
 |!=NULL | ==0  |  free(ptr)    |
 
 반환 값
 realloc() 함수는 새로 할당된 메모리에 대한 포인터를 반환하며, 
 이 포인터는 어떤 내장 타입(built-in type)에도 적합하게 정렬(aligned)되어 있습니다. 
 요청이 실패하면 NULL을 반환합니다. 
 반환된 포인터는, 만약 할당이 이동되지 않았다면 (예: 그 자리에서(in-place) 할당을 확장할 공간이 있었다면) ptr과 같을 수도 있고, 
 할당이 새 주소로 이동되었다면 ptr과 다를 수도 있습니다. 
 만약 size가 0이었다면, NULL 또는 free()에 전달하기 적합한 포인터가 반환됩니다. 
 만약 realloc()이 실패하면, 원본 블록은 건드려지지 않은 상태로 남습니다; 그것은 해제(freed)되거나 이동되지 않습니다.

 |  size | meta   | return |
 | tiny  | tiny   | origin |
 | tiny  | small  | origin |
 | tiny  | large  | origin | //최적화 할거면 반납후 사용해도 되는데 굳이?
 | small | tiny   | small  |
 | small | small  | origin |
 | small | large  | origin |
 | large | tiny   | large  |
 | large | small  | large  |
 | large | large  | origin |
*/

//여기까지 하고 있었음 
static void *realloc_impl(void *ptr, size_t size){
  METADATA_t *meta;
  void *temp;
  size_t zone;
  
  if (ptr != NULL && size == 0) {
    free_impl(ptr);
    return (NULL);
  }
  if (ptr == NULL) {
    return (malloc_impl(size));
  }
  meta = (METADATA_t *)ptr - 1;
  zone = (meta->size & ~INDEX_MASK);
  if (size < (TINY - METASIZE)) {
    meta->client_size = size;
    return ((void *)(meta + 1));
  }
  else if (size < (SMALL - METASIZE)) {
    if (zone != TINY) {
      meta->client_size = size;
      return ((void *)(meta + 1));
    }
    temp = malloc_impl(size);
    if (temp == NULL) {
      return (NULL);
    }
    size_t client_size_temp = meta->client_size;
    size_t copy = client_size_temp < size ? client_size_temp : size;
    temp = ft_memmove(temp, ptr, copy);
    free_impl(ptr);
    return (temp);
  }
  else {
    if (zone < size + 1) {
      temp = malloc_impl(size);
      if (temp == NULL) {
        return (NULL);
      }
      size_t client_size_temp = meta->client_size;
      size_t copy = client_size_temp < size ? client_size_temp : size;
      temp = ft_memmove(temp, ptr, copy);
      free_impl(ptr);
      return (temp);
    }
    meta->client_size = size;
    return ((void *)(meta + 1));
  }
  return (NULL);
}

static void	*callo_imp(size_t nmemb, size_t size){
  void	*reptr;
	size_t	total;

  if (nmemb == 0 || size == 0)
    return (malloc_impl(0));
	total = nmemb * size;
	if (total != 0 && total / nmemb != size)
		return (NULL);
	reptr = (void *)malloc_impl(size * nmemb);
	if (reptr == NULL) {
    return (NULL);
  };
	ft_memset(reptr, 0, size * nmemb);
	return (reptr);
}
