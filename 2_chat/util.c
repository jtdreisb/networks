// chat
// CPE464 Program 2
// Jason Dreisbach
#include "cpe464.h"

uint8_t *makePacket(struct ChatHeader header, uint8_t *data, ssize_t dataLen)
{
	int packetLen = kChatHeaderSize + dataLen;
	uint8_t *packet = malloc(packetLen);
	if (packet == NULL) {
		perror("makePacket:malloc");
		exit(1);
	}

	memcpy(packet, &header, kChatHeaderSize);

	struct ChatHeader *packetHeader = (struct ChatHeader *)packet;
	packetHeader->checksum = 0;
	memcpy(packet + kChatHeaderSize, data, dataLen);
	// Put header fields into network order
	packetHeader->sequenceNumber = htonl(packetHeader->sequenceNumber);
	// Calculate the checksum and stuff it into the right part
	packetHeader->checksum = in_cksum((uint16_t *)packet, packetLen);
	return packet;
}