NAME		= libft_serial.so

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -fPIC
LDFLAGS		= -shared -lpthread

SRCS		= serial_engine.c
OBJS		= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $(NAME) $(OBJS)

%.o: %.c serial_engine.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
