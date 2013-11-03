all:
	gcc sgy_ping.c -o sgy_ping -lpthread
	gcc ping_flood.c -o ping_flood -lpthread
	gcc udp_flood.c -o udp_flood -lpthread

clean:
	rm sgy_ping
	rm ping_flood
	rm udp_flood
