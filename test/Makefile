CC = gcc
CFLAG = -g
BENCH_FALG = -O2 -pthread
LIB_PATH = ../libft_malloc.a
PRINTF_PATH = ../printf/libftprintf.a
LIBFT_PATH = ../libft/libft.a
RM = rm -rf

SRC = testMalloc.c
BENCH_SRC = bench_malloc.c
OBJ = $(SRC:.c=.o)
BENCH_OBJ = $(BENCH_SRC:.c=.o)
NAME = testMalloc

all: $(NAME)

bench : $(BENCH_OBJ)
	$(CC) $(CFLAG) $(BENCH_FALG) -o $@ $^

$(NAME): $(OBJ) $(LIB_PATH) $(LIBFT_PATH) $(PRINTF_PATH) 
	$(CC) $(CFLAG) -o $@ $^

%.o : %.c
	@$(CC) $(CFLAG) -c $< -o $@

bench_malloc.o: bench_malloc.c
	@$(CC) $(CFLAG) $(BENCH_FALG) -c $< -o $@

clean:
	$(RM) $(OBJ)
	$(RM) $(BENCH_OBJ)
fclean: clean
	$(RM) $(NAME)
	$(RM) bench
re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: all clean fclean re bench