CC = gcc
CFLAG = -g
LIB_PATH = ../libft_malloc.a
PRINTF_PATH = ../printf/libftprintf.a
LIBFT_PATH = ../libft/libft.a
AR = ar -rcs
RM = rm -rf

SRC = testzero.c
OBJ = $(SRC:.c=.o)
NAME = testMalloc

all: $(NAME)

$(NAME): $(OBJ) $(LIB_PATH) $(PRINTF_PATH) $(LIBFT_PATH)
	$(CC) $(CFLAG) $(OBJ) $(LIB_PATH) $(PRINTF_PATH) $(LIBFT_PATH) -o $(NAME)

$(LIB_PATH):
	$(MAKE) -C ..

$(PRINTF_PATH):
	$(MAKE) -C ../printf

$(LIBFT_PATH):
	$(MAKE) -C ../libft

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

.PHONY: all clean fclean re