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

#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "can.h"
#include "beep.h"
#include "exti.h"
#include "timer.h"
#include "lock.h"
#include "commhead.h"
#include "string.h"
#include "stdlib.h"
#include "math.h"
#include "stdio.h"
#include "can.h"
#include "FM1702.h"
#include "spi.h"
#include "DataFlash.h"
#define FLASH_ADR 0x0801FC00
#pragma pack(2)

u32 data1=87654321;
u32 data=0;
u8 res;
u16 length;
u8 rfid_statusA;
u8 rfid_statusB;
u8 report_statusA;
u8 report_statusB;
u8 countVal1;
u8 countVal2;
unsigned char out_main_A[16];//检测函数的返回值，里面包含了FM1702中RxThreshold的帧.其中out[15]为最终需要的值。out[2]=0xFF为没有检测，out[2]=0x11为检测完毕，out[2]=0x55为正在检测。
unsigned char out_main_B[16];//检测函数的返回值，里面包含了FM1702中RxThreshold的帧.其中out[15]为最终需要的值。out[2]=0xFF为没有检测，out[2]=0x11为检测完毕，out[2]=0x55为正在检测。
unsigned char chipID[2];
//volatile unsigned char flag_pollingA; 
//volatile unsigned char flag_pollingB; 
int main(void)
{
RCC_ClocksTypeDef  RCC_Clocks; //???
	  SystemInit(); //?????????
	RCC_GetClocksFreq(&RCC_Clocks); //????????
	
    u8 mode=CAN_Mode_Normal;//CAN工作模式;CAN_Mode_Normal(0)：普通模式，CAN_Mode_LoopBack(1)：环回模式
    delay_init();	    	 //延时函数初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
    uart_init(115200);	 	//串口初始化为115200
    LED_Init();		  		//初始化与LED连接的硬件接口
    LOCK_Init();
//    TIM3_Int_Init(99,719);
	  SPI1_Init();
	  SPI1_SetSpeed(SPI_BaudRatePrescaler_256);
//	  delay_ms(100);
    Init_FM1702();
	  RFID_Powerdown_B();
	  RFID_Powerdown_A();
//    CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,mode);//CAN初始化环回模式,波特率500Kbps
	  CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_7tq,CAN_BS1_8tq,6,mode);//CAN初始化环回模式,波特率500Kbps
		FLASH_ReadByte(0x8007500,out_main_A,16);
	  delay_ms(50);
		FLASH_ReadByte(0x8007000,out_main_B,16);
	  delay_ms(50);
		FLASH_ReadByte(0x8007800,chipID,2);
	  delay_ms(50);	
    chipID_B = chipID[0]; 
		chipID_A = chipID[1];	
	//	RST_H_2nd;	
	//	RST_H;
/*老板初始化状态*/
/*		out_main_A[0]=0xFF;
		out_main_A[2]=0xFF;
		FLASH_SetLatency(FLASH_Latency_1);       
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(0x8007500);    
		FLASH_WriteByte(0x8007500,out_main_A,16);  
		FLASH_Lock();  	
		delay_ms(1000);					
		out_main_B[0]=0xFF;
		out_main_B[2]=0xFF;
		FLASH_SetLatency(FLASH_Latency_1);       
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(0x8007000);    
		FLASH_WriteByte(0x8007000,out_main_B,16);  
		FLASH_Lock();  	
		delay_ms(1000);			
		
		while(1)
    {
			LEDR1 = 0;LEDG1 = 1;LEDB1 = 1;
			LEDR2 = 0;LEDG2 = 1;LEDB2 = 1;						
		}*/
/*老板初始化状态*/		
		
    while(1)
    {
			PCAN_BODY pBODY =NULL;
//			
//   			u8 i=0;
//			  u8 j=0;
		if(out_main_A[2]==0x55)
		{
		LEDR1 = 1;LEDG1 = 0;LEDB1 = 1;
	  RFID_Powerdown_B();
	  RFID_Wakeup_A();			
		calibration_A();	
		}
		else if(out_main_B[2]==0x55)
		{
		RFID_Wakeup_B();
	  RFID_Powerdown_A();
		LEDR2 = 1;LEDG2 = 0;LEDB2 = 1;		
		calibration_B();		
		}
		else
		{
			if((out_main_A[2]==0xFF) || (out_main_A[2]==0x11)||(out_main_B[2]==0x11)||(out_main_B[2]==0xFF)||(out_main_A[2]==0xF1)||(out_main_B[2]==0xF1)||(out_main_A[2]==0x22)||(out_main_B[2]==0x22))
			{	
					if((WRPtr - RDPtr)>((u8)(2)))                //原来写的是大于8,发现有些指令根本不需要8字节
					{
//							PRA("READ WRITE POINTERS ! WRPtr-RDPtr:0x%x,\r\n", (WRPtr-RDPtr));
							PCAN_ADDR pADDR = (PCAN_ADDR)(recvBuffer+(RDPtr-recvBuffer));
							if(pADDR->dest)
							{
									pBODY = (PCAN_BODY)(pADDR+1);
									if((pADDR->dest)== chipID_B)                    //using the two parallel if
									{
											choiceFlagB = 1;
											choiceFlagA = 0;
									}
									else if((pADDR->dest)== chipID_A)
									{
											choiceFlagA = 1;
											choiceFlagB = 0;
									}
//									PRA("NORMAL packet ! choiceFlagA:0x%x, choiceFlagB:0x%x,\r\n",choiceFlagA, choiceFlagB);
//									PRA("First packet: 0x");
//									for (i = 0; i < 8; ++i)PRA("%02x",RDPtr[i]);
//									PRA("\r\n");
							}
							else
							{
									PDEST_EXTEND pEXTEND = (PDEST_EXTEND)(pADDR+1);
									pBODY = (PCAN_BODY)(pEXTEND+1);
									if((pEXTEND->dest_extend)&(1<<chipID_B) )
									{
											choiceFlagB = 1;
									}
									if((pEXTEND->dest_extend)&(1<<(chipID_A)) )
									{
											choiceFlagA = 1;
									}
//									PRA("EXTEND packet ! choiceFlagA:0x%x, choiceFlagB:0x%x,\r\n",choiceFlagA, choiceFlagB);
//									PRA("First packet: 0x");
//									for (i = 0; i < 8; ++i)PRA("%02x",RDPtr[i]);
//									PRA("\r\n");
							}
							u8 size;
							size = pBODY ->size;
							if( (WRPtr - RDPtr)>= (size))
							{
									if((choiceFlagA==1)||(choiceFlagB==1))
									{
//										PRA("Just Before Command Switch ! choiceFlagA:0x%x, choiceFlagB:0x%x,\r\n",choiceFlagA, choiceFlagB);
//										PRA("READ WRITE POINTERS ! WRPtr:0x%x,  RDPtr:0x%x,\r\n", WRPtr, RDPtr);
											switch(pBODY->command)
											{
											/*设置灯的状态*/
											case CAN_SET_LED:
											{
													PSET_LED pLED = (PSET_LED)(pBODY+1);
													if(choiceFlagA)
													{
																LEDR1=pLED->r;
																LEDG1=pLED->g;
																LEDB1=pLED->b; 
														
													}
													if(choiceFlagB)
													{
																LEDR2=pLED->r;
																LEDG2=pLED->g;
																LEDB2=pLED->b; 
													}
											}
											break; 
											/*设置锁的状态*/
											case CAN_SET_LOCK:
											{
													PSET_LOCK pLOCK = (PSET_LOCK)(pBODY+1);
													if(choiceFlagA)
													{
															if(pLOCK->lock)    //开锁
															{
													//			  Init_FM1702_2nd();
                         //         SPI1_Open();
																	flag_pollingA=1; 
//																  flag_pollingB=0;
//																  i=0;
//																  RFID_On_A();
																  delay_ms(5);
																  RFID_Wakeup_A();
						                      delay_ms(1);

																  RFID_status_2nd = Read_tag_A();                  //初始化RFID状态标识																
																  delay_ms(1);
																  RFID_Powerdown_A();																				
																	LOCK1P =1;
																	LOCK1N =0;
																	delay_ms(50);
																	LOCK1P =0;
																	LOCK1N =0;
//																	RFID_Wakeup_A();
//																	delay_ms(110);																		
															
//																  delay_ms(80);

//																  i[0] = chipID_A;
//																  i[1] = 0x11;
//																  i[2] = flag_pollingA;
//																  i[3] = flag_pollingB;																
//																  Can_Send_Msg(i,8);
															}
															else
															{
//																  delay_ms(10);	
																	RFID_Powerdown_A();
																  delay_ms(5);
//																  RFID_Off_A();
												//				  SPI1_Close();
															//	  RST_H_2nd;
																  delay_ms(10);	
																  flag_pollingA=0;
																  flag_pollingB=0;																
																	LOCK1P =0;
																	LOCK1N =1;
																	delay_ms(50);
																	LOCK1P =0;
																	LOCK1N =0;
//																delay_ms(30);
//																  i[0] = chipID_A;
//																  i[1] = 0x33;
//																  i[2] = flag_pollingA;
//																  i[3] = flag_pollingB;																
//																  Can_Send_Msg(i,8);
															}
													}
													if(choiceFlagB)
													{
															if(pLOCK->lock)    //开锁
															{
													//			Init_FM1702_1st();
													//			SPI1_Open();
																	flag_pollingB=1;
//																  flag_pollingA=0;
//																  i=0;
//																  RFID_On_B();
																	delay_ms(5);
																	RFID_Wakeup_B();
																	delay_ms(1);																
																  RFID_status_1st=Read_tag_B();//初始化RFID状态标识			
																	delay_ms(1);
																	RFID_Powerdown_B();																
																	LOCK2N =0;
																	LOCK2P =1;
																	delay_ms(50);
																	LOCK2N =0;
																	LOCK2P =0;
//																	RFID_Wakeup_B();
//																	delay_ms(110);																
													
																  //delay_ms(80);

//																  i[0] = chipID_B;
//																  i[1] = 0x11;
//																  i[2] = flag_pollingA;
//																  i[3] = flag_pollingB;
//																  Can_Send_Msg(i,8);
															}
															else
															{
//																  delay_ms(10);	
																  RFID_Powerdown_B();
																  delay_ms(5);
//																  RFID_Off_B();
												//				  SPI1_Close();
															//	  RST_H;
																  delay_ms(10);	
																	flag_pollingB=0;
																  flag_pollingA=0;															
																	LOCK2N =1;
																	LOCK2P =0;
																	delay_ms(50);
																	LOCK2N =0;
																	LOCK2P =0;
//																	delay_ms(30);																	
//																  i[0] = chipID_B;
//																  i[1] = 0x33;
//																  i[2] = flag_pollingA;
//																  i[3] = flag_pollingB;																
//																  Can_Send_Msg(i,8);																
															}
													}
											}
											break;
											case CAN_GET_RFID:
											{
													if(choiceFlagA)
													{   
												//		 SPI1_Open();
														  delay_ms(50);
															RFID_Wakeup_A();
															delay_ms(1);
															rfid_statusA=Read_tag_A();
															RFID_Powerdown_A();
														  delay_ms(1);
															if(rfid_statusA)
															{
																	memcpy(relayBuf,buffer_A,16);
																	SendPacket(chipID_A,ACK);
															}
															else
															{ 															
																	memset(relayBuf,0x00,16);
																	SendPacket(chipID_A,ACK);
															}
															delay_ms(50);
												//			SPI1_Close();
													}
													if(choiceFlagB)
													{
											//			  SPI1_Open();
												//		  delay_ms(280);
														 delay_ms(50);
															RFID_Wakeup_B();
															delay_ms(1);
															rfid_statusB=Read_tag_B();
															RFID_Powerdown_B();
														  delay_ms(1);
															if(rfid_statusB)
															{
																	memcpy(relayBuf,buffer_B,16);
																	SendPacket(chipID_B,ACK);
															}
															else
															{															
																	memset(relayBuf,0x00,16);
																	SendPacket(chipID_B,ACK);	
															}
															delay_ms(50);
											//				SPI1_Close();
													}
											}
											break;
											case CAN_SET_KEY:
											{
	//                        PKEY pKEY = (PKEY)(pBODY+1);
											}
											break;
											/*初始化，选择出最佳寄存器的值*/											
											case CAN_CALIB:
											{
												if(choiceFlagA)
												{
													out_main_A[0]=0x3F;
													out_main_A[2]=0x55;
													 FLASH_SetLatency(FLASH_Latency_1);       
													 FLASH_Unlock();
													 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
													 FLASH_ErasePage(0x8007500);    
													 FLASH_WriteByte(0x8007500,out_main_A,16);  
													 FLASH_Lock();  	
													 delay_ms(1);												
													__set_FAULTMASK(1);
													NVIC_SystemReset();														
												}
												if(choiceFlagB)
												{
													out_main_B[0]=0x3F;
													out_main_B[2]=0x55;
													 FLASH_SetLatency(FLASH_Latency_1);       
													 FLASH_Unlock();
													 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
													 FLASH_ErasePage(0x8007000);    
													 FLASH_WriteByte(0x8007000,out_main_B,16);  
													 FLASH_Lock();  	
													 delay_ms(1);	
														__set_FAULTMASK(1);
														NVIC_SystemReset();														
												}		
											}
											break;
									
											}
											choiceFlagA=0;
											choiceFlagB=0;
									}

									RDPtr = RDPtr+size;  
//									PRA("Command Switch Finish! choiceFlagA:0x%x, choiceFlagB:0x%x,\r\n",choiceFlagA, choiceFlagB);
//									PRA("READ WRITE POINTERS ! WRPtr:0x%x,  RDPtr:0x%x,\r\n", WRPtr, RDPtr);
							}


					}
//flag_pollingA=1;
					if(flag_pollingB)
					{
						RFID_Wakeup_B();
						delay_ms(1);
						rfid_statusB=Read_status_B();
//						RFID_Powerdown_B();
						delay_ms(1);
						RFID_Powerdown_B();
						if(rfid_statusB && flag_pollingB)
						{
								memcpy(relayBuf,buffer_B,16);
								SendPacket(chipID_B,NOTIFY);
						}
						delay_ms(30);
					}

					if(flag_pollingA)
					{
						RFID_Wakeup_A();
						delay_ms(1);
						rfid_statusA=Read_status_A();
						RFID_Powerdown_A();
//						RFID_Powerdown_A();
						delay_ms(1);
						if(rfid_statusA && flag_pollingA)
						{
								memcpy(relayBuf,buffer_A,16);
								SendPacket(chipID_A,NOTIFY);
						}
						delay_ms(30);
					}

/*如果没有校准过的孔,指示灯亮起。并且打开A孔的天线等待写ID*/
					if((out_main_A[2]==0xFF)&(out_main_B[2]==0xFF))
					{
						 LEDR1 = 0;LEDG1 = 1;LEDB1 = 0;
						 LEDR2 = 0;LEDG2 = 0;LEDB2 = 0;							
						 delay_ms(1000);
						 flag_pollingA=1; 
						 
					}
					if((out_main_A[2]==0xF1)&(out_main_B[2]==0xF1))
					{
						 LEDR1 = 0;LEDG1 = 0;LEDB1 = 1;
						 LEDR2 = 0;LEDG2 = 0;LEDB2 = 1;							
						 delay_ms(1000);
						 
					}
					if(out_main_A[2]==0x22)
					{
						 LEDR1 = 1;LEDG1 = 1;LEDB1 = 0;				
						 delay_ms(1000);
						 
					}
					if(out_main_B[2]==0x22)
					{
						 LEDR2 = 1;LEDG2 = 1;LEDB2 = 0;							
						 delay_ms(1000);
						 
					}
				}
		}
    }
}


