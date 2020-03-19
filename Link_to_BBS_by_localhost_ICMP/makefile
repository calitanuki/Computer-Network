CC=gcc

RM=rm -rf

all:OutsideProxy InsideProxy 

%:%.c
		$(CC) $< -lpthread -o $@

clean:
		$(RM) sender receiver
