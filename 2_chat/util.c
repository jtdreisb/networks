// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"

uint8_t *makePacket(HeaderFlag flag, uint8_t *data, ssize_t dataLen)
{
	uint8_t *packet = malloc(kChatHeaderSize + dataLen);
	if (packet == NULL) {
		perror("makePacket:malloc");
		exit(1);
	}

	struct ChatHeader *packetHeader = (struct ChatHeader *)packet;
	packetHeader->flag = flag;

	//TODO: packetHeader->sequenceNumber
	//TODO: packetHeader->checksum

	memcpy(packet + sizeof(struct ChatHeader *), data, dataLen);
	return packet;
}