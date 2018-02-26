
/*
 * --------------------
 * Company							: Tianjin techvan
 * --------------------
 * Project Name					: AGV
 * Description					: C file
 * --------------------
 * Tool Versions				: uVision V5.17.0.0
 * Target Device				: STM32F407IGT6
 * --------------------
 * Engineer							: wangyulong
 * Revision							: 1.0
 * Created Date					: 2016.01.03
 * --------------------
 * Engineer							:
 * Revision							:
 * Modified Date				:
 * --------------------
 * Additional Comments	:
 *
 * --------------------
 */
#include "can.h"
#include "led.h"
#include "delay.h"
#include "usart.h"
#include "lock.h"
#include "string.h"
#include "commhead.h"
#include "FM1702.h"

u8 recvBuffer[1024];
u8 relayBuffer[128];
u8 sendBuffer[32];
u16 recvBufferCount=0;
u8 relayBuf[16];
u8 *WRPtr=recvBuffer;
u8 *RDPtr=recvBuffer;

u8 choiceFlagA=0;
u8 choiceFlagB=0;

u8 chipID_A=0xFF;
u8 chipID_B=0xFF;//chipID_B is "B", chipID_A is "A"


u16 std_id =0x0000;
u8 flag_pollingA=0; 
u8 flag_pollingB=0; 




u8 CAN_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{
    GPIO_InitTypeDef 		GPIO_InitStructure;
    CAN_InitTypeDef        	CAN_InitStructure;
    CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
#if CAN_RX0_INT_ENABLE
    NVIC_InitTypeDef  		NVIC_InitStructure;
#endif

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��PORTAʱ��



    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);			//��ʼ��IO

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);			//��ʼ��IO

    //CAN��Ԫ����
    CAN_InitStructure.CAN_TTCM=DISABLE;			//��ʱ�䴥��ͨ��ģʽ
    CAN_InitStructure.CAN_ABOM=DISABLE;			//����Զ����߹���
    CAN_InitStructure.CAN_AWUM=DISABLE;			//˯��ģʽͨ���������(���CAN->MCR��SLEEPλ)
    CAN_InitStructure.CAN_NART=ENABLE;			//��ֹ�����Զ�����
    CAN_InitStructure.CAN_RFLM=DISABLE;		 	//���Ĳ�����,�µĸ��Ǿɵ�
    CAN_InitStructure.CAN_TXFP=DISABLE;			//���ȼ��ɱ��ı�ʶ������
    CAN_InitStructure.CAN_Mode= mode;	        //ģʽ���ã� mode:0,��ͨģʽ;1,�ػ�ģʽ;
    //���ò�����
    CAN_InitStructure.CAN_SJW=tsjw;				//����ͬ����Ծ���(Tsjw)Ϊtsjw+1��ʱ�䵥λ  CAN_SJW_1tq	 CAN_SJW_2tq CAN_SJW_3tq CAN_SJW_4tq
    CAN_InitStructure.CAN_BS1=tbs1; 			//Tbs1=tbs1+1��ʱ�䵥λCAN_BS1_1tq ~CAN_BS1_16tq
    CAN_InitStructure.CAN_BS2=tbs2;				//Tbs2=tbs2+1��ʱ�䵥λCAN_BS2_1tq ~	CAN_BS2_8tq
    CAN_InitStructure.CAN_Prescaler=brp;        //��Ƶϵ��(Fdiv)Ϊbrp+1
    CAN_Init(CAN1, &CAN_InitStructure);        	//��ʼ��CAN1

    CAN_FilterInitStructure.CAN_FilterNumber=0;	//������0
    CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdList; 	//����λģʽ
    CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 	//32λ��
    CAN_FilterInitStructure.CAN_FilterIdHigh=std_id<<5;	//32λID
    CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32λMASK
    CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//������0������FIFO0
    CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//���������0

    CAN_FilterInit(&CAN_FilterInitStructure);			//�˲�����ʼ��

#if CAN_RX0_INT_ENABLE
    CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);				//FIFO0��Ϣ�Һ��ж�����.

    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // �����ȼ�Ϊ1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // �����ȼ�Ϊ0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
    return 0;
}
unsigned char data0;
unsigned char flag_light;
unsigned char count;
unsigned char flag_headjudge;
#if CAN_RX0_INT_ENABLE	//ʹ��RX0�ж�
//�жϷ�����
void USB_LP_CAN1_RX0_IRQHandler(void)
{

    CanRxMsg RxMessage;
//    PCAN_ADDR pADDR = (PCAN_ADDR)RxMessage.Data;
//	if (CAN1->RF0R & CAN_RF0R_RFOM0)
// {
    CAN_Receive(CAN1, 0, &RxMessage);
    if((RxMessage.Data[0]==chipID_B)||(RxMessage.Data[0]==chipID_A)||(RxMessage.Data[0]==0x00))
   {
        if(RDPtr==WRPtr)                        //ÿ�������µ����ݵ�ʱ�������ָ�벻���׵�ַ����ͬʱ����ָ���дָ�����׵�ַ�����ƶ�(RDPtr-recvBuffer>0)
        {
//        memmove(recvBuffer,RDPtr, WRPtr-RDPtr);    //define a read pointer and a write pointer, both of them start from recvBuffer
            WRPtr=recvBuffer;//+(WRPtr-RDPtr);            //move the pointer backwards
            RDPtr=recvBuffer;                          //the read pointer is fixed to the begining after the new data arrival
        }
        if((RxMessage.Data[1]&0x80) == 0x80 )          // the MSB of the source ID is set to 1 if this frame is the first one of the packet
        {
            if(RxMessage.Data[0] == 0)
            {
                if(RxMessage.Data[6] > 8)                  //flag_headjudge ����ź�Ŀ���Ǳ��ⶪ֡
								{ flag_headjudge = 1;}
								memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
                WRPtr = WRPtr + RxMessage.DLC;
            }
						else if ((RxMessage.Data[3]==2) && (RxMessage.Data[0]==(chipID_A)))  //������
						{
							if(RxMessage.Data[4])    //����
              {

             memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
             WRPtr = WRPtr + RxMessage.DLC;	

//							RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//							Can_Send_Msg(RxMessage.Data,8);								
              }
              else
              {
//							RFID_Powerdown_A();	    
						  flag_pollingA=0;
						 memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
             WRPtr = WRPtr + RxMessage.DLC;	

//							RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//							Can_Send_Msg(RxMessage.Data,8);								
              }
						}
						else if ((RxMessage.Data[3]==2) && (RxMessage.Data[0]==chipID_B))  //������
						{
							if(RxMessage.Data[4])    //����
              {

//							flag_pollingB=1; 
						  memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
              WRPtr = WRPtr + RxMessage.DLC;
								
//							RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//							Can_Send_Msg(RxMessage.Data,8);								
              }
              else
              {
//		          RFID_Powerdown_B();
						  flag_pollingB=0;
							memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
              WRPtr = WRPtr + RxMessage.DLC;
								
//							RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//							Can_Send_Msg(RxMessage.Data,8);								
              }
						}
/*            else if (RxMessage.Data[3]==1 && RxMessage.Data[0]==(chipID_A))   //������
						{

													    LEDR1=RxMessage.Data[4];
													    LEDG1=RxMessage.Data[5];
													    LEDB1=RxMessage.Data[6]; 


						}
						else if (RxMessage.Data[3]==1 && RxMessage.Data[0]==(chipID_B))   //������
						{

													    LEDR2=RxMessage.Data[4];
													    LEDG2=RxMessage.Data[5];
													    LEDB2=RxMessage.Data[6]; 
						}*/  
						else
            {							
             memcpy(recvBuffer + (WRPtr-recvBuffer), RxMessage.Data,RxMessage.DLC);  //��ָ���дָ�붼ָ����һ��
             WRPtr = WRPtr + RxMessage.DLC;
							
//						RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//						Can_Send_Msg(RxMessage.Data,8);							
						}
	
        }
        else
        {
            if(flag_headjudge)
            {
                flag_headjudge =0;
                memcpy(recvBuffer + (WRPtr-recvBuffer), &RxMessage.Data[2],RxMessage.DLC-2);// �ǵ�һ֡����
                WRPtr = WRPtr + RxMessage.DLC-2;
							
//								RxMessage.Data[0] = RxMessage.Data[0] | 0x80;
//								Can_Send_Msg(RxMessage.Data,8);							
            }
        }
    }
//	 PRA("READ WRITE POINTERS in interrupt! WRPtr:0x%x,  RDPtr:0x%x,\r\n", WRPtr, RDPtr);
//	}

}


#endif

//can����һ������(�̶���ʽ:IDΪ0X12,��׼֡,����֡)
//len:���ݳ���(���Ϊ8)
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;
//		 ����,ʧ��;
u8 Can_Send_Msg(u8* msg,u8 len)
{
    u8 mbox;
    u16 i=0;
    CanTxMsg TxMessage;
    TxMessage.StdId=chipID_B;			// ��׼��ʶ��
    TxMessage.ExtId=0x00;			// ������չ��ʾ��
    TxMessage.IDE=CAN_Id_Standard; 	// ��׼֡
    TxMessage.RTR=CAN_RTR_Data;		// ����֡
    TxMessage.DLC=len;				// Ҫ���͵����ݳ���
    for(i=0; i<len; i++)
        TxMessage.Data[i]=msg[i];
    mbox= CAN_Transmit(CAN1, &TxMessage);
//  if (mbox == CAN_TxStatus_NoMailBox)
//	{
//	  while(1);
//	}
    i=0;
    while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
    if(i>=0XFFF)
        return 1;
    return 0;
}
//can�ڽ������ݲ�ѯ
//buf:���ݻ�����;
//����ֵ:0,�����ݱ��յ�;
//		 ����,���յ����ݳ���;
u8 Can_Receive_Msg(u8 *buf)
{
    u32 i;
    CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)
        return 0;		//û�н��յ�����,ֱ���˳�
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//��ȡ����
    for(i=0; i<8; i++)
        buf[i]=RxMessage.Data[i];
    return RxMessage.DLC;
}


void SendPacket(u8 SouID,u8 ACKorNOTI)
{

//   PCAN_ADDR pADDR = (PCAN_ADDR)sendBuffer;
//   PCAN_BODY pBODY = (PCAN_BODY)(pADDR+1);
    PCAN_BODY pBODY = (PCAN_BODY)sendBuffer;
    PRFID pRFID = (PRFID)(pBODY+1);	
		PCALIB pCALIB = (PCALIB)(pBODY+1);
    unsigned char size;
    unsigned char flag;
    unsigned char CanSend[8];
    CanSend[0] = 127;
    CanSend[1] = SouID;
    if(ACKorNOTI == ACK)
    {
        pBODY ->command = CAN_GET_RFID|0x80;
    }
		else if(ACKorNOTI == CALIB_ACK)
		{
				pBODY ->command = CAN_CALIB;//|0x80;
		}
    else
    {
        pBODY ->command = CAN_NTY_RFID;
    }
		
		if(ACKorNOTI==CALIB_ACK)
		{
    size = sizeof(*pBODY)+sizeof(*pCALIB)+2;
    pBODY -> size = size;
    memcpy(pCALIB ->calib,relayBuf,2);
//    flag =1;			
		}
		else
		{			
    size = sizeof(*pBODY)+sizeof(*pRFID)+2;
    pBODY -> size = size;
    memcpy(pRFID ->rfid,relayBuf,16);
    flag =1;
		}
    while(size>=6)
    {
        if(flag==1)
        {
            flag = 0;
//          CanSend[0] = CanSend[0]|0x80;

            CanSend[1] = CanSend[1]|0x80;
            memcpy(&(CanSend[2]),sendBuffer,6);
            size = size -8;
            Can_Send_Msg(CanSend,8);
            memmove(sendBuffer,sendBuffer+6,size);
            CanSend[1] = CanSend[1]&0x7f;

        }
        else
        {
            CanSend[0] = CanSend[0];
            CanSend[1] = CanSend[1];
            memcpy(&(CanSend[2]),sendBuffer,6);
            size = size - 6;
            Can_Send_Msg(CanSend,8);
            memmove(sendBuffer,sendBuffer+6,size);
        }
    }
    if(size>0)
    {
        CanSend[0] = CanSend[0];
        CanSend[1] = CanSend[1]|0x80;
        memcpy(&(CanSend[2]),sendBuffer,size);
        Can_Send_Msg(CanSend,size);
    }
}










