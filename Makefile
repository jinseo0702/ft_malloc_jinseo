ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

CC = gcc
AR = ar rcs
PIC_FLAG = -fPIC
# CFLAG = -g -Wall -Wextra -Werror -Iinclude -Iprintf -Ilibft
CFLAG = -g -pthread -Wall -Wextra -Werror -Iinclude -Iprintf -Ilibft
# CFLAG = -fsanitize=thread -g -O0 -fno-omit-frame-pointer -pthread -Iinclude -Iprintf -Ilibft
RM = rm -rf

SRC = src/ft_malloc.c \
	src/ft_override.c
OBJS = $(SRC:.c=.o)
NAME = libft_malloc_$(HOSTTYPE).so
NAME_A  = libft_malloc.a
HEADER = include/ft_malloc.h
LIBFT_A = libft/libft.a
PRINTF_A = printf/libftprintf.a

all : $(NAME)

static: $(NAME_A)

$(NAME): $(OBJS) $(LIBFT_A) $(PRINTF_A)
	@$(CC) -shared -o $@ $^

$(NAME_A): $(OBJS) $(LIBFT_A) $(PRINTF_A)
	@$(AR) $@ $^

$(LIBFT_A):
	@make -C libft/ EXTRA_CFLAG="$(PIC_FLAG)"

$(PRINTF_A):
	@make -C printf/ EXTRA_CFLAG="$(PIC_FLAG)"

%.o : %.c $(HEADER)
	@$(CC) $(CFLAG) $(PIC_FLAG) -c $< -o $@

clean :
	@make clean -C libft/
	@make clean -C printf/
	@$(RM) $(OBJS)

fclean :
	@make fclean -C libft/
	@make fclean -C printf/
	@$(RM) $(OBJS) $(NAME) $(NAME_A)

re : 
	@make fclean
	@make all

.PHONY: all clean fclean re