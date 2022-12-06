CC=cc
CFLAGS=-Os -s

all:
	$(CC) $(CFLAGS) -o whatip whatip.c

clean:
	rm whatip
