# Project:	webinfo.c
# Author:	Tomas Valek
# Date:   	27.2.2012
# 

NAME = webinfo

# compiler of C language:
CC=gcc
# params
CFLAGS=-std=c99 -Wextra -Wall -pedantic -g

all: $(NAME)

.PHONY: clean
.PHONY: pack

$(NAME): $(NAME).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(NAME) $(NAME).zip

pack:
	zip $(NAME).zip $(NAME).c Makefile README.md LICENSE
