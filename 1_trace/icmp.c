// icmp
// trace

#include "trace.h"

typedef enum {
	ICMP_TYPE_REPLY = 0x00,
	ICMP_TYPE_UNREACHABLE = 0x03,
	ICMP_TYPE_REQUEST = 0x08
} ICMPType;

struct ICMPFrameHeader
{
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_checksum;

} __attribute__((packed));

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
			case ICMP_TYPE_UNREACHABLE:
				printf("Unreachable");
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