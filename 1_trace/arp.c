// arp
// trace

#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include "trace.h"

struct ARPFrameHeader
{
	uint16_t	arp_hardware_format;
	uint16_t	arp_protocol_format;
	uint8_t	arp_hardware_length;
	uint8_t	arp_protocol_length;
	uint16_t	arp_opcode;
} __attribute__((packed));

void arp(uint8_t *packetData, int packetLength)
{

	struct ARPFrameHeader *header = (struct ARPFrameHeader *) packetData;
	uint8_t *senderMAC, *targetMAC;
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

	senderMAC = (uint8_t *)header+sizeof(struct ARPFrameHeader);
	printf("\t\tSender MAC: %s\n", ether_ntoa((struct ether_addr *)senderMAC));

	senderIP = (in_addr_t *)(senderMAC + header->arp_hardware_length);
	netAddr.s_addr = (in_addr_t)*senderIP;
	printf("\t\tSender IP: %s\n", inet_ntoa(netAddr));

	targetMAC = (uint8_t *)senderIP+header->arp_protocol_length;
	printf("\t\tTarget MAC: %s\n", ether_ntoa((struct ether_addr *)targetMAC));

	targetIP = (in_addr_t *)(targetMAC + header->arp_hardware_length);
	netAddr.s_addr = (in_addr_t)*targetIP;
	printf("\t\tTarget IP: %s\n", inet_ntoa(netAddr));


}