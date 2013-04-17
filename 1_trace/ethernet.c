// ethernet functions
// trace


#include <netinet/if_ether.h>
#include "trace.h"


#define kEthernetFrameTrailerOffset 16

struct EthernetFrameTrailer
{
	uint8_t frameCheckSequence[4];
	uint8_t interframeGap[12];
} __attribute__((packed));


void ethernet(uint8_t *packetData, int packetLength)
{
	char *destination, *source;
	uint8_t *nextFrame;
	int nextFrameLength;
	struct EthernetFrameHeader *header = (struct EthernetFrameHeader *)packetData;

	printf("\tEthernet Header\n");

	destination = ether_ntoa((struct ether_addr *)header->destinationMAC);
	printf("\t\tDest MAC: %s\n", destination);

	source = ether_ntoa((struct ether_addr *)header->sourceMAC);
	printf("\t\tSource MAC: %s\n", source);

	header->etherType = ntohs(header->etherType);

	nextFrame = packetData+sizeof(struct EthernetFrameHeader);
	nextFrameLength = packetLength-(sizeof(struct EthernetFrameHeader)+sizeof(struct EthernetFrameTrailer));

	printf("\t\tType: ");

	if (header->etherType == 0x0806) {
		printf("ARP\n\n");
		arp(nextFrame, nextFrameLength);
	}
	else if (header->etherType == 0x0800) {
		printf("IP\n\n");
		ip(nextFrame, nextFrameLength);
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