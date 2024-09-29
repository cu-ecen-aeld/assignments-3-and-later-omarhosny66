# In case of using CROSS_COMPILE 
CC := $(CROSS_COMPILE)gcc

writer: writer.c
	$(CC) -Wall -Wextra -O2 writer.c -o writer

clean:
	rm -f writer