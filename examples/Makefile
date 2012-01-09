
OBJECTS = mail-file.o
LIBESMTP = `libesmtp-config --libs`
GSASL = `pkg-config libgsasl --libs`
CFLAGS := $(CFLAGS) -std=c99 -pedantic -O2 -g -W -Wall `libesmtp-config --cflags`

#all: mail-file mail-file-a
all: mail-file

mail-file: $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LIBESMTP) $(GSASL) -o mail-file

mail-file-a: $(OBJECTS)
	$(CC) -g -static $(OBJECTS) $(LIBESMTP) $(GSASL) -o mail-file-a

clean:
	rm -f *.o core mail-file-a mail-file-so
