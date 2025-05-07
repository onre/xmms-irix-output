CC			:= gcc
CFLAGS			:= -g -O3 $(CFLAGS)
SOURCES			:= audio.c init.c about.c IRIX.c
OBJS			:= $(SOURCES:.c=.o)

all: libIRIX.so

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

libIRIX.so: $(OBJS) IRIX.h
	$(CC) $(LDFLAGS) -shared -o $@ $(OBJS)

clean:
	rm -f *.o libIRIX.so
