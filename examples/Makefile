
OBJECTS = mail-file.o
LIBESMTP = `libesmtp-config --libs`
CFLAGS := $(CFLAGS) -std=c99 -pedantic -O2 -g -W -Wall `libesmtp-config --cflags`

all: mail-file-a mail-file-so

mail-file-a: $(OBJECTS)
	$(CC) -g -static $(OBJECTS) $(LIBESMTP) -o mail-file-a

mail-file-so: $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LIBESMTP) -o mail-file-so

clean:
	rm -f *.o core mail-file-a mail-file-so
