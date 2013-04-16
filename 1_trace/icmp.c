// icmp
// trace

#include "trace.h"

struct ICMPFrameHeader
{
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_checksum;

} __attribute__((packed));

void icmp(uint8_t *packetData, int packetLength)
{

}