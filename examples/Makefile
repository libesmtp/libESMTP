
OBJECTS = mail-file.o
LIBESMTP = `pkg-config libesmtp-1.0 --libs`
CFLAGS := $(CFLAGS) -std=c99 -pedantic -O2 -g -W -Wall `pkg-config libesmtp-1.0 --cflags`

all: mail-file-a mail-file-so

mail-file-a: $(OBJECTS)
	$(CC) -g -static $(OBJECTS) $(LIBESMTP) -o mail-file-a

mail-file-so: $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LIBESMTP) -o mail-file-so

clean:
	rm -f *.o core mail-file-a mail-file-so
