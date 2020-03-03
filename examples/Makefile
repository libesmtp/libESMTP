
OBJECTS = mail-file.o
LIBESMTP = `pkg-config libesmtp-1.0 --libs`
CFLAGS := $(CFLAGS) -std=c99 -pedantic -O2 -g -W -Wall `pkg-config libesmtp-1.0 --cflags`

all: mail-file

mail-file: $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LIBESMTP) -o mail-file

mail-file-a: $(OBJECTS)
	$(CC) -g -static $(OBJECTS) $(LIBESMTP) -o mail-file-a

clean:
	rm -f *.o core mail-file-a mail-file-so
