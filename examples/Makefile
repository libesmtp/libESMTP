
OBJECTS = mail-file.o
LIBESMTP = `pkg-config libesmtp-1.0 --libs`
GSASL = `pkg-config libgsasl --libs`
CFLAGS := $(CFLAGS) -std=c99 -pedantic -O2 -g -W -Wall `pkg-config libesmtp-1.0 --cflags`

all: mail-file

mail-file: $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LIBESMTP) $(GSASL) -o mail-file

clean:
	rm -f *.o core mail-file-a mail-file-so
