// ethernet functions
// trace


#include <netinet/if_ether.h>
#include "trace.h"

struct EthernetFrameHeader
{
	u_char destinationMAC[6];
	u_char sourceMAC[6];
	uint16_t etherType;
} __attribute__((packed));


#define kEthernetFrameTrailerOffset 16

struct EthernetFrameTrailer
{
	u_char frameCheckSequence[4];
	u_char interframeGap[12];
} __attribute__((packed));


void ethernet(u_char *packetData, int packetLength)
{
	char *destination, *source;
	u_char *nextFrame;
	int nextFrameLen;
	struct EthernetFrameHeader *header = (struct EthernetFrameHeader *)packetData;

	printf("\tEthernet Header\n");

	destination = ether_ntoa((struct ether_addr *)header->destinationMAC);
	printf("\t\tDest MAC: %s\n", destination);

	source = ether_ntoa((struct ether_addr *)header->sourceMAC);
	printf("\t\tSource MAC: %s\n", source);

	header->etherType = ntohs(header->etherType);

	nextFrame = packetData+sizeof(struct EthernetFrameHeader);
	nextFrameLen = packetLength-(sizeof(struct EthernetFrameHeader)+sizeof(struct EthernetFrameTrailer));

	printf("\t\tType: ");

	if (header->etherType == 0x0806) {
		printf("ARP\n\n");
		arp(nextFrame, nextFrameLen);
	}
	else if (header->etherType == 0x0800) {
		printf("IP\n\n");
		ip(nextFrame, nextFrameLen);
		// type = "IP"; // v4
	}
	else if (header->etherType == 0x86DD) {
		printf("IP\n\n");
		// type = "IP"; // v6
	}
	else {
		printf("Unknown\n\n");
		// type = "Unknown";
	}



}