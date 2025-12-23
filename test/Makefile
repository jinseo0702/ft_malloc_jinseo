CC = gcc
CFLAG = -g
LIB_PATH = ../libft_malloc.a
PRINTF_PATH = ../printf/libftprintf.a
LIBFT_PATH = ../libft/libft.a
RM = rm -rf

SRC = testMalloc.c
OBJ = $(SRC:.c=.o)
NAME = testMalloc

all: $(NAME)

$(NAME): $(OBJ) $(LIB_PATH) $(LIBFT_PATH) $(PRINTF_PATH) 
	$(CC) $(CFLAG) -o $@ $^

%.o : %.c
	@$(CC) $(CFLAG) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: all clean fclean re