# FLAGS= -Wall -Wextra -Werror -g

FLAGS = -Wall -Wextra -Werror -g -lavformat -lavcodec -lswresample -lswscale -lavutil -lm

build:
	gcc main.c -o scbvuarge ${FLAGS}
