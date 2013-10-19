/*date: 2013-10-21
 *author: shenya
	*/
#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>


#define ICMPHDR_LEN 8  //icmp head length
#define ICMPDATA_LEN 56
#define IPHDR_LEN 20
#define PACKET_LEN (IPHDR_LEN+ICMPHDR_LEN+ICMPDATA_LEN)

#define INIT_MIN 1024

#define MAXPACKET 128

typedef struct ping_packet
{
	uint16_t packet_seq;
	uint8_t flag;
	struct timeval tv_begin;
	
}record_t;

static record_t ping_record[MAXPACKET]; 

static unsigned char packet[PACKET_LEN + 10];
static unsigned char packet_recv[PACKET_LEN+IPHDR_LEN+10];

uint8_t *hostname = NULL;
struct sockaddr_in dest;
static int rawsock;
static pid_t pid = 0;
static char dest_str[64];
static packet_send = 0;
static packet_received  = 0;

static struct timeval ping_start;

static long time_sum = 0;
static long time_max = 0;
static long time_min = INIT_MIN;

static char loop = 1;


record_t *icmp_find(int seq)
{
	record_t *found = NULL;
	int i = 0;

	if(seq == -1)
	{
		for(;i<MAXPACKET; i++)
		{
			if(ping_record[i].flag == 0)
			{
				found = &ping_record[i];
				break;
			}
		}
	}
	else
	{
		for(;i<MAXPACKET; i++)
		{
			if(ping_record[i].packet_seq == seq)
			{
				found = &ping_record[i];
				break;
			}
		}
		
	}

	return found;
}

int32_t icmp_cksum(void *data, int length)
{
	int left = length;
	uint16_t *wd = (uint16_t *)data;
	int sum = 0;
	uint16_t temp;

	while(left > 1)
	{
		sum += *wd++;
		left -= 2;
	}
	if(left == 1)
	{
		*((uint8_t *)(&temp)) = *((uint8_t *)wd);
		sum += temp;
	}

	sum = (sum >> 16) + (sum&0xffff);
	sum += (sum >> 16);

	temp = ~sum;

	return temp;
}

uint32_t icmp_packet(void)
{
	
	memset(packet, 0, sizeof(packet));
	
	struct icmp *icmph = (struct icmp *)packet;
	
	packet_send++;

	icmph->icmp_type = ICMP_ECHO;
	icmph->icmp_code = 0;
	icmph->icmp_cksum = 0;
	icmph->icmp_id = pid&0xffff;
	icmph->icmp_seq = htons(packet_send);

	
	gettimeofday((struct timeval *)&packet[ICMPHDR_LEN], (struct timezone *)NULL);
	
	icmph->icmp_cksum = icmp_cksum((void *)packet, ICMPHDR_LEN+ICMPDATA_LEN);
	
	return 0;
}

void icmp_send(void *arg)
{
	
	int send_size;
	record_t *found;
	struct timeval tv_send;

	while(loop)
	{
		icmp_packet();
		send_size = 0;
		send_size = sendto(rawsock, (const void *)packet, 
											ICMPHDR_LEN+ICMPDATA_LEN, 0, 
											(const struct sockaddr *)&dest, sizeof(dest));
		if(send_size > 0)
		{
			gettimeofday(&tv_send, NULL);
			found = icmp_find(-1);
			if(found != NULL)
			{
				found->flag = 1;
				found->tv_begin = tv_send;
				found->packet_seq = packet_send;
			}
			else
			{
				printf("icmp_find: out of boundary\n");
			}

		}
		else
		{
			packet_send--;
			continue;
		}

		if(packet_send == 128)
		{
			printf("ping: packet is too many\n");
			loop = 0;
		}

		sleep(1);

	}

}

struct timeval tv_sub(struct timeval out, struct timeval in)
{
	struct timeval tv_interval;
	
	tv_interval.tv_usec = in.tv_usec - out.tv_usec;
	tv_interval.tv_sec = in.tv_sec - out.tv_sec;

	if(tv_interval.tv_usec < 0)
	{
		tv_interval.tv_sec--;
		tv_interval.tv_usec += 1000000;
	}

	return tv_interval;
}

void icmp_unpacket(char *buf, int len)
{
	int iphdr_len = 0;
	int ttl;
	struct icmp *icmph = NULL;
	struct iphdr *iph = (struct iphdr *)buf;
	record_t *found;
	struct timeval tv_begin;
	struct timeval tv_end;
	struct timeval tv_interval;
	long int time_interval;
	
	iphdr_len = iph->ihl << 2;
	ttl = iph->ttl;
	icmph = (struct icmp *)(buf + iphdr_len);
#if 0
	printf("icmp_id is %d pid %d,ttl %d addr %s\n", icmph->icmp_id, pid, 
					ttl, inet_ntoa(*(struct in_addr *)&(iph->saddr)));
#endif
	if(icmph->icmp_id == pid)
	{
		found = icmp_find(ntohs(icmph->icmp_seq));
		if(found != NULL)
		{
			found->flag = 0;
			tv_begin = found->tv_begin;
		}
		gettimeofday(&tv_end, NULL);

		tv_interval = tv_sub(tv_begin, tv_end);
#if 0
		printf("tv_begin: sec %ld, usec %ld, tv_end: sec %ld, usec %ld\n",
						tv_begin.tv_sec, tv_begin.tv_usec, tv_end.tv_sec, tv_end.tv_usec);
#endif
		time_interval = tv_interval.tv_sec*1000 + tv_interval.tv_usec/1000;

		printf("64 bytes from %s: icmp_req=%d ttl=%d time=%ld ms\n", 
						inet_ntoa(*(struct in_addr *)&(iph->saddr)),
						ntohs(icmph->icmp_seq),
						ttl,
						time_interval);

		packet_received++;

		time_sum += time_interval;
		if(time_interval > time_max)
		{
			time_max = time_interval;
		}else if(time_interval < time_min)
		{
			time_min = time_interval;
		}


	}
	else
	{
		return ;
	}

}
void icmp_recv(void *arg)
{
		int recv_size;
		struct timeval timeout;
		fd_set readfd;

		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		while(loop)
		{

			FD_ZERO(&readfd);
			FD_SET(rawsock, &readfd);
			if(select(rawsock+1, &readfd, NULL, NULL,
								&timeout) < 1)
			{
				continue;
			}
			recv_size = recv(rawsock, packet_recv, sizeof(packet_recv), 0);
			if(recv_size > 0)
			{
				
				icmp_unpacket(packet_recv, recv_size);
			}

		}
}

void icmp_statistics(void)
{
	struct timeval ping_end;
	struct timeval time_inteval;

	gettimeofday(&ping_end, NULL);
	time_inteval = tv_sub(ping_start, ping_end);

	printf("\n\n--- %s statistics ---\n", hostname);
	printf("%d packets transmitted, %d received, %d%c packet loss, time %ldms\n",
				packet_send, packet_received, 
				((packet_send-packet_received)*100)/packet_send,
				'%',
				(time_inteval.tv_sec*1000 + time_inteval.tv_usec/1000));

	printf("rtt min/avg/max = %ld/%ld/%ld ms\n", 
					time_min,
					time_sum/packet_received,
					time_max);
}
void sig_int(int num)
{
	loop = 0;
	icmp_statistics();
	signal(SIGINT, SIG_DFL);
}
int main(int argc, char *argv[])
{
	struct hostent *host = NULL;
	struct protoent *protocol = NULL;		
	char protoname[] = "icmp";
	int32_t inaddr = -1;

	int size = 84*1024;

	pthread_t send_id;
	pthread_t recv_id;

	if(argc < 2)
	{
		printf("ping aaa.bbb.ccc.ddd\n");
		exit(1);
	}
	protocol = getprotobyname(protoname);
	if(protocol == NULL)
	{
		printf("fail: not support icmp\n");
		exit(1);
	}
	//printf("protocol is %d\n", protocol->p_proto);
	pid = getpid();

	memset(&dest, 0, sizeof(dest));
	hostname = argv[1];

	dest.sin_family = AF_INET;
	inaddr = inet_addr(hostname);
	if(inaddr > 0)
	{
		dest.sin_addr.s_addr = inaddr;
		memcpy(dest_str, hostname, strlen(hostname));
	}
	else
	{

		host = gethostbyname(hostname);
		if(host == NULL)
		{
			printf("fail to gthostbyname");
			exit(1);
		}
		
	//	printf("addr is %s\n", inet_ntoa(*((struct in_addr *)host->h_addr)));
		memcpy((char *)&dest.sin_addr, host->h_addr, host->h_length);
		memcpy(dest_str, inet_ntoa(*((struct in_addr *)host->h_addr)), strlen(inet_ntoa(*((struct in_addr *)host->h_addr))));
	}
	
	rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
	if(rawsock < 0)
	{
		perror("fail to socket");
		exit(1);
	}

	setsockopt(rawsock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	memset(ping_record, 0, sizeof(ping_record));
	printf("PING %s (%s) %d(84) bytes of data.\n", hostname, dest_str, ICMPDATA_LEN);
	
	gettimeofday(&ping_start, NULL);

	signal(SIGINT, sig_int);
	pthread_create(&send_id, NULL, icmp_send, NULL);
	pthread_create(&recv_id, NULL, icmp_recv, NULL);



	pthread_join(send_id, NULL);
	pthread_join(recv_id, NULL);

	return 0;
}
