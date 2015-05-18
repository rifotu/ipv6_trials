LIBS=-lm -lgps
PREFIX=/usr/local

gpsdproxy: gpsdproxy.o
	$(CC) -o $@ $^ $(LIBS)

gpsdproxy.o: gpsdproxy.c
	$(CC) -c -o $@ $<

install: gpsdproxy
	install -m 0755 -s gpsdproxy $(PREFIX)/sbin
	install -m 0755 gpsdproxy.sh /etc/init.d/gpsdproxy

clean:
	rm -f gpsdproxy gpsdproxy.o
