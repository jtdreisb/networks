// arp
// trace

// struct ARPFrameHeader
// {
	
// } __attribute__((packed));
#include <netinet/if_arp.h>
#include "trace.h"

void arp(u_char *packetData, int packetLength)
{
	struct arphdr *header = (struct arphdr *) packetData;
	printf("\tARP header\n");

}