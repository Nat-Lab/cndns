CFLAGS=-O3 -Wall
OBJS=cndns.o

eoip: $(OBJS)
	$(CC) $(CFLAGS) -o cndns $(OBJS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: install
install:
	cp cndns /usr/local/sbin/cndns

.PHONY: uninstall
uninstall:
	rm -f /usr/local/sbin/cndns

.PHONY: clean
clean:
	rm -f cndns
	rm -f *.o
