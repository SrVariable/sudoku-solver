NAME := sudoku
CC := clang
CFLAGS := -g -IBFL/include
CFLAGS += -Wall -Wextra -Werror
LDFLAGS := -LBFL
LDLIBS := -lBFL -lm

BFL := BFL/libBFL.a
SRCS := main.c
OBJS := $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS) $(BFL)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BFL):
	$(MAKE) -C BFL

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
