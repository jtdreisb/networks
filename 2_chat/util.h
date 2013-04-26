// chat
// CPE464 Program 2
// Jason Dreisbach

typedef enum {
	FLAG_INIT_REQ = 1,
	FLAG_INIT_ACK = 2,
	FLAG_INIT_ERR = 3,
	FLAG_MSG_REQ  = 6,
	FLAG_MSG_ERR  = 7,
	FLAG_MSG_ACK = 255,
	FLAG_EXIT_REQ = 8,
	FLAG_EXIT_ACK = 9,
	FLAG_LIST_REQ = 10,
	FLAG_LIST_RESP = 11,
	FLAG_HNDL_REQ = 12,
	FLAG_HNDL_RESP = 13
} PacketFlag;