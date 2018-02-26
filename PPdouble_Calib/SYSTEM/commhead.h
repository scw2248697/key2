//
//  commhead.h
//  commhead
//
//  Created by Walker on 16-11-13.
//
//

#ifndef COMM_HEAD_H
#define COMM_HEAD_H

#pragma pack(1)
#pragma anon_unions

// zeromq

#define PROTOCOL_VERSION	1

#define CHANNEL_ADMIN       0x00
#define CHANNEL_MANAGE      0x01

typedef struct _COMM_HEAD {
	unsigned char         version;    // 0x01
	unsigned char         channel;    // 0x00 admin; 0x01 control;
	union {
		unsigned short      id;
		struct {
			unsigned char     compress;
			unsigned char     encrypt;
		};
	};
	// unsigned int          size;
//	unsigned char         data[0];
} COMM_HEAD, *PCOMM_HEAD;

#define COMMAND_ACK					0x80000000
#define COMMAND_WAKEUP			0x00000001
#define COMMAND_HEARTBEAT		0x00000002


// can
typedef struct _CAN_ADDR {
	unsigned char					dest;
	unsigned char					source;
//unsigned char         data[0];
} CAN_ADDR, *PCAN_ADDR;

typedef struct _DEST_EXTEND {
	unsigned int					dest_extend;
	
//	unsigned int	dest_extend4;	// broadcast bit sn is device id, 96-128
//	unsigned int 	dest_extend3;   // 65-96
//	unsigned int	dest_extend2;	// 33-64
//	unsigned int 	dest_extend1;	// 1-32	
//unsigned char         data[0];
} DEST_EXTEND, *PDEST_EXTEND;

typedef struct _CAN_BODY {
	unsigned char					size;
	unsigned char					command;
//unsigned char         data[0];
} CAN_BODY, *PCAN_BODY;

#define	CAN_SET_LED			0x01
typedef struct _SET_LED {
	unsigned char					r;
	unsigned char					g;
	unsigned char					b;
	unsigned char					breathe;
} SET_LED, *PSET_LED;
 
#define	CAN_SET_LOCK		0x02
typedef struct _SET_LOCK {
	unsigned char					lock;
} SET_LOCK, *PSET_LOCK;

#define CAN_GET_RFID		0x03
#define CAN_NTY_RFID		0x04
#define	RFID_LENGTH			16
typedef struct _RFID {
	unsigned char					rfid[RFID_LENGTH];
} RFID, *PRFID;

#define CAN_SET_KEY			0x05
#define	KEY_LENTH				6
typedef struct _KEY {
	unsigned char					rfid[KEY_LENTH];
} KEY, *PKEY;

#define	CAN_WRITE_REG		0x06
#define	CAN_CALIB   		0x07
#define	CALIB_LENGTH			1
typedef struct _CALIB {
	unsigned char					calib[CALIB_LENGTH];
} CALIB, *PCALIB;

#pragma pack()

#endif // COMM_HEAD_H
