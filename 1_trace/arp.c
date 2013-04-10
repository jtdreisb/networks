// arp
// trace

// struct ARPFrameHeader
// {
	
// } __attribute__((packed));
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include "trace.h"

void arp(u_char *packetData, int packetLength)
{
	
	struct arphdr *header = (struct arphdr *) packetData;
	u_char *senderMAC, *targetMAC;
	in_addr_t *senderIP, *targetIP;
	struct in_addr netAddr;

	printf("\tARP header\n");

	printf("\t\tOpcode: ");
	header->ar_op = ntohs(header->ar_op);
	switch (header->ar_op) {
		case ARPOP_REQUEST:
			printf("Request");
		break;
		case ARPOP_REPLY:
			printf("Reply");
		break;
		case ARPOP_REVREQUEST:
			printf("Rev Request"); // TODO: check these values
		break;
		case ARPOP_REVREPLY:
			printf("Rev Reply"); // TODO: check these values
		break;
		case ARPOP_INVREQUEST:
			printf("INV Request"); // TODO: check these values
		break;
		case ARPOP_INVREPLY:
			printf("INV Reply"); // TODO: check these values
		break;
		default:
			printf("Unknown (%d)", header->ar_op);
		break;
	}
	printf("\n");

	senderMAC = (u_char *)header+sizeof(struct arphdr);
	printf("\t\tSender MAC: %s\n", ether_ntoa((struct ether_addr *)senderMAC));
	
	senderIP = (in_addr_t *)senderMAC + header->ar_hln;
	netAddr.s_addr = (in_addr_t)*senderIP;
	printf("\t\tSender IP: %s\n", inet_ntoa(netAddr));


}