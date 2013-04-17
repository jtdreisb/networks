// main
// trace

#include <pcap.h>
#include "trace.h"

void usage()
{
	printf("usage:\n\ttrace <pcap filename> \n");
}

int main(int argc, char **argv)
{
	char *inputFileName = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *p = NULL;
	struct pcap_pkthdr *packetHeader;
	uint8_t *packetData = NULL;
	int pcapStatus = 1;
	int packetCount = 0;

	if (argc == 2) {
		inputFileName = argv[1];
	}
	else {
		usage();
		exit(1);
	}

	p = pcap_open_offline(inputFileName, errbuf);
	if (p == NULL) {
		printf("ERROR: %s\n", errbuf);
		exit(2);
	}
	while (1) {
		pcapStatus = pcap_next_ex(p, &packetHeader, (const uint8_t **)&packetData);
		if (pcapStatus == 1) {
			printf("\nPacket number: %d  Packet Len: %d\n\n", ++packetCount, packetHeader->len);
			ethernet(packetData, packetHeader->len);
			printf("\n");
		}
		else {
			break;
		}
	}
	pcap_close(p);

	return 0;
}