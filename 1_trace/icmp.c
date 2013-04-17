// icmp
// trace

#include "trace.h"

typedef enum {
	ICMP_TYPE_REPLY = 0x00,
	ICMP_TYPE_REQUEST = 0x08
} ICMPType;

void icmp(uint8_t *packetData, int packetLength)
{
	struct ICMPFrameHeader *header = (struct ICMPFrameHeader *)packetData;
	printf("\n\tICMP Header\n");
	printf("\t\tType: ");
	if (header->icmp_code == 0) {
		switch (header->icmp_type) {
			case ICMP_TYPE_REQUEST:
				printf("Request");
			break;
			case ICMP_TYPE_REPLY:
				printf("Reply");
			break;
			default:
				printf("Unknown");
			break;
		}
	}
	else {
		printf("Unknown");
	}
}