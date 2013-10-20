all:
	gcc sgy_ping.c -o sgy_ping -lpthread
	gcc ping_flood.c -o ping_flood -lpthread

clean:
	rm sgy_ping
	rm ping_flood
