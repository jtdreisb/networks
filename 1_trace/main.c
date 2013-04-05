// main
// trace

#include "trace.h"
#include <stdio.h>
#include <pcap/pcap.h>

void usage()
{
	printf("usage:\n\ttrace <pcap filename> \n");
}

int main(int argc, char const *argv[])
{
	char *inputFileName = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *p = NULL;
	struct pcap_pkthdr *pkt_header;
	u_char *pkt_data = NULL;
	int pcap_status = 1;

	if (argc == 2) {
		inputFileName = argv[1];
	}
	else {
		usage();
		exit(1);
	}


	p = pcap_open_offline(inputFileName, errbuf);
	while (pcap_status == 1) {
		pcap_status =  pcap_next_ex(p, &pkt_header, &pkt_data);
		// begin decoding the file
		printf("readpacket\n");
	}
	pcap_close(p);
	
	return 0;
}