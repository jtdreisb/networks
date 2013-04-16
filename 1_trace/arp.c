// arp
// trace

#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include "trace.h"

struct ARPFrameHeader
{
	u_short	arp_hardware_format;
	u_short	arp_protocol_format;
	u_char	arp_hardware_len;
	u_char	arp_protocol_len;
	u_short	arp_opcode;
} __attribute__((packed));

void arp(u_char *packetData, int packetLength)
{

	struct ARPFrameHeader *header = (struct ARPFrameHeader *) packetData;
	u_char *senderMAC, *targetMAC;
	in_addr_t *senderIP, *targetIP;
	struct in_addr netAddr;

	printf("\tARP header\n");

	printf("\t\tOpcode: ");
	header->arp_opcode = ntohs(header->arp_opcode);
	switch (header->arp_opcode) {
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
			printf("Unknown (%d)", header->arp_opcode);
		break;
	}
	printf("\n");

	senderMAC = (u_char *)header+sizeof(struct ARPFrameHeader);
	printf("\t\tSender MAC: %s\n", ether_ntoa((struct ether_addr *)senderMAC));

	senderIP = (in_addr_t *)(senderMAC + header->arp_hardware_len);
	netAddr.s_addr = (in_addr_t)*senderIP;
	printf("\t\tSender IP: %s\n", inet_ntoa(netAddr));

	targetMAC = (u_char *)senderIP+header->arp_protocol_len;
	printf("\t\tTarget MAC: %s\n", ether_ntoa((struct ether_addr *)targetMAC));

	targetIP = (in_addr_t *)(targetMAC + header->arp_hardware_len);
	netAddr.s_addr = (in_addr_t)*targetIP;
	printf("\t\tTarget IP: %s\n", inet_ntoa(netAddr));


}