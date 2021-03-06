#ifndef __CAN_H
#define __CAN_H
#include "sys.h"




#define CAN_RX0_INT_ENABLE	1	//0,不使能;1,使能.								    
#define LEAST_TACKLE_SIZE 8
#define CALIB_ACK 2
#define ACK 1
#define NOTIFY 0
#define DOUBLE

u8 CAN_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN初始化

u8 Can_Send_Msg(u8* msg,u8 len);						//发送数据

u8 Can_Receive_Msg(u8 *buf);							//接收数据

void SendPacket(u8 SouID,u8 ACKorNOTI);

extern u8 *WRPtr;
extern u8 *RDPtr;
extern u8 relayBuffer[128];
extern u8 recvBuffer[1024];
extern u8 sendBuffer[32];
extern u16 recvBufferCount;
extern u8 choiceFlagA;
extern u8 choiceFlagB;
extern u8 flag_pollingA;
extern u8 flag_pollingB;
extern u8 chipID_A;
extern u8 chipID_B;
extern unsigned char flag_light;
extern unsigned char count;
extern u8 relayBuf[16];
#endif

















