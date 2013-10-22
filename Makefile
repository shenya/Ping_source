all:
	gcc sgy_ping.c -o sgy_ping -lpthread

clean:
	rm sgy_ping
