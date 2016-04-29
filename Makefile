CC = gcc
CFLAGS = -g -O2
NAME = test
all: $(NAME)

$(NAME): src/fbdraw.c src/main.c
	$(CC) $^ -o $@ -lm $(CFLAGS)

clean:
	$(RM) $(NAME)
