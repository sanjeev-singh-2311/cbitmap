CXX = gcc
CFLAGS = -Wall \
		 -Wextra \
		 -g

.PHONY: run

main: main.c
	$(CXX) $(CFLAGS) -o main main.c

run:
	make main
	./main
