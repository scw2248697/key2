#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FM1702.h"
#include "usart.h"
#include "delay.h"
#include "string.h"
#include "spi.h"
#include "DataFlash.h"
#include "can.h"
#define uchar unsigned char

//uchar       cardtype;
uchar     	tagtype[2];	        /* ¿¨Æ¬±êÊ¶×Ö·û */
/* FM1702±äÁ¿¶¨Òå */
volatile unsigned char     	buf_B[10];            /* FM1702ÃüÁî·¢ËÍ½ÓÊÕ»º³åÇø */
unsigned char buffer_B[16];
unsigned char buffer_temp_1st[16];
uchar     	UID[5];             /* ÐòÁÐºÅ */
unsigned char DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//uchar       Secnr;			        /* ÉÈÇøºÅ */
uchar RFID_status_1st;  /*Ëø¿××´Ì¬±êÊ¶, 1--ÓÐÔ¿³× 0--ÎÞÔ¿³×*/

//void delay_B(unsigned int dlength)
//{ 
//	unsigned int  i;
//	unsigned char j;
//	for (i=0;i<dlength;i++)
//	{
//		for (j=0;j<100;j++);
//	}
//} 

//SPI½ÓÊÕ
 unsigned char rev_B(uchar tem)
{
  
  return SPI1_ReadWriteByte(tem);
/*	
  uchar i;
	unsigned char var=0,var_snt=0;
	
	var=0;
	for (i=0;i<8;i++)                     // 8Î»×Ö½Ú´ø½øÎ»×óÒÆ
	{ 
		var_snt=tem&0x80;                   // Ñ¡Ôñ¸ßÎ»
		if (var_snt==0x80) MOSI_H;          // Êä³ö¸ßÎ»
		else               MOSI_L;	
		tem<<=1;	
		var<<=1;

	  if (READ_MISO==1) 
			var += 1;                        // ´ø½øÎ»×óÒÆ
		SCK_H;                             // ²úÉúSCKÂö³å	
		delay_B(1);		
		SCK_L;	 
    }	
	return(var);                         // ·µ»Ø
*/
} 

//SPI·¢ËÍ
 void Send_B(unsigned char var) 

{ 
	SPI1_ReadWriteByte(var);
/*
	unsigned char i,tem;
	 
	for (i=0;i<8;i++)                   // 8Î»×Ö½ÚÊä³ö
	{
		delay_B(1);	             
		tem=var&0x80;                     // Ñ¡Ôñ¸ßÎ»
		if (tem==0x80) MOSI_H;            // Êä³ö¸ßÎ»
    else           MOSI_L;
    delay_B(1);
		SCK_H;                            // Ê¹SCKÎª1
		var<<=1;	                        // ×óÒÆ1Î»
		delay_B(1);	
		SCK_L; 							              // Ê¹SCKÎªµÍµçÆ½
	}  
*/
}                  

//¶Á¼Ä´æÆ÷
 uchar read_reg_B(uchar SpiAddress)
{
	uchar rdata;
	//PR("1st FM1702 SPI read_reg_B \r\n");
	SpiAddress=SpiAddress<<1;
	SpiAddress=SpiAddress | 0x80; 
	delay_us(30);
	NSS_L;
	NSS_H_2nd;
  delay_us(30);
	Send_B(SpiAddress);
	rdata=rev_B(0);
	NSS_H;
  NSS_H_2nd;	
	return(rdata);
}

//Ð´¼Ä´æÆ÷
 void write_reg_B(uchar SpiAddress,uchar dat)
{
	//PR("1st FM1702 SPI write_reg_B \r\n");
	SpiAddress = SpiAddress << 1;
	SpiAddress = SpiAddress & 0x7f;
	delay_us(30);
	NSS_L;
  NSS_H_2nd;	
	delay_us(30);	
	Send_B(SpiAddress);		
	Send_B(dat);	
	NSS_H;
	NSS_H_2nd;
}

/****************************************************************/
/*Ãû³Æ: Clear_FIFO_B                                              */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÇå¿FFIFOµÄÊý¾Ý                                */
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö:                                                         */
/*	    TRUE, FIFO±»Çå¿Õ                                        */
/*	    FALSE, FIFOÎ´±»Çå¿Õ  	                                  */
/****************************************************************/
 uchar Clear_FIFO_B(void)
{
	uchar temp;
	uint  i;
	
	temp =read_reg_B(Control);						//Çå¿ÕFIFO
	temp = (temp | 0x01);
	write_reg_B(Control, temp);
	for(i = 0; i < RF_TimeOut; i++)			//¼ì²éFIFOÊÇ·ñ±»Çå¿Õ
	{
		temp = read_reg_B(FIFO_Length);
		if(temp == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************/
/*Ãû³Æ: Write_FIFO_B                                              */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÏòRC531µÄFIFOÖÐÐ´Èëx bytesÊý¾Ý                */
/*												       			                          */
/*ÊäÈë:                                                         */
/*      count, ´ýÐ´Èë×Ö½ÚµÄ³¤¶È                                 */
/*	    buff, Ö¸Ïò´ýÐ´ÈëÊý¾ÝµÄÖ¸Õë                              */
/*                                                              */
/*Êä³ö:                                                         */
/*	    N/A                                                 		*/
/****************************************************************/
 void Write_FIFO_B(uchar count,volatile unsigned char *buff)
{
	uchar i;
	
	for(i = 0; i < count; i++)
	{
		write_reg_B(FIFO,*(buff + i));
	}
}

/****************************************************************/
/*Ãû³Æ: Read_FIFO_B                                               */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ´ÓRC531µÄFIFOÖÐ¶Á³öx bytesÊý¾Ý                */
/*												        		                          */
/*ÊäÈë:                                                         */
/*       buff, Ö¸Ïò¶Á³öÊý¾ÝµÄÖ¸Õë                               */
/*                                                              */
/*Êä³ö:                                                         */
/*	     N/A                                                 		*/
/****************************************************************/
uchar Read_FIFO_B(volatile unsigned char *buff)
{
	uchar temp;
	uchar i;
	
	temp =read_reg_B(FIFO_Length);
	if (temp == 0)
	{
		return 0;
	}
	if (temp >= 24)						//temp=255Ê±,»á½øÈëËÀÑ­»·
	{									        //Òò´ËÔö¼ÓFIFO_LengthÔ½ÏÞÅÐ¶Ï
		temp = 24;						 
	}
	for(i = 0;i < temp; i++)
	{
 		*(buff + i) =read_reg_B(FIFO);
	}
	return temp;
 }

/****************************************************************/
/*Ãû³Æ: Judge_Req_B                                               */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶Ô¿¨Æ¬¸´Î»Ó¦´ðÐÅºÅµÄÅÐ¶Ï                      */
/*												       			                          */
/*ÊäÈë:                                                         */
/*       *buff, Ö¸ÏòÓ¦´ðÊý¾ÝµÄÖ¸Õë                              */
/*                                                              */
/*Êä³ö:                                                         */
/*	     TRUE, ¿¨Æ¬Ó¦´ðÐÅºÅÕýÈ·                                 */
/*       FALSE, ¿¨Æ¬Ó¦´ðÐÅºÅ´íÎó                                */
/****************************************************************/
 uchar Judge_Req_B(volatile unsigned char *buff)
{
	uchar temp1,temp2;
	
	temp1 = *buff;
	temp2 = *(buff + 1);

	if((temp1 != 0x00) && (temp2 == 0x00))
	{
		return TRUE;
	}
	return FALSE;
}

/****************************************************************/
/*Ãû³Æ: Check_UID_B                                               */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔÊÕµ½µÄ¿¨Æ¬µÄÐòÁÐºÅµÄÅÐ¶Ï                    */
/*ÊäÈë: N/A                                                     */
/*Êä³ö: TRUE: ÐòÁÐºÅÕýÈ·                                        */
/* FALSE: ÐòÁÐºÅ´íÎó                                            */
/****************************************************************/
 uchar Check_UID_B(void)
{
	uchar	temp;
	uchar	i;

	temp = 0x00;
	for(i = 0; i < 5; i++)
	{
		temp = temp ^ UID[i];
	}

	if(temp == 0)
	{
		PR("1st Check UID_OK\r\n");
		return TRUE;
	}

	return FALSE;
}

/****************************************************************/
/*Ãû³Æ: Save_UID_B                                                */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ±£´æ¿¨Æ¬ÊÕµ½µÄÐòÁÐºÅ                          */
/*ÊäÈë: row: ²úÉú³åÍ»µÄÐÐ                                       */
/* col: ²úÉú³åÍ»µÄÁÐ                                            */
/* length: ½Ó…§µ½µÄUIDÊý¾Ý³¤¶È                                  */
/*Êä³ö: N/A                                                     */
/****************************************************************/
 void Save_UID_B(uchar row, uchar col, uchar length)
{
	uchar	i;
	uchar	temp;
	uchar	temp1;

	if((row == 0x00) && (col == 0x00))
	{
		for(i = 0; i < length; i++)
		{
			UID[i] = buf_B[i];
		}
	}
	else
	{
		temp = buf_B[0];
		temp1 = UID[row - 1];
		switch(col)
		{
		case 0:		temp1 = 0x00; row = row + 1; break;
		case 1:		temp = temp & 0xFE; temp1 = temp1 & 0x01; break;
		case 2:		temp = temp & 0xFC; temp1 = temp1 & 0x03; break;
		case 3:		temp = temp & 0xF8; temp1 = temp1 & 0x07; break;
		case 4:		temp = temp & 0xF0; temp1 = temp1 & 0x0F; break;
		case 5:		temp = temp & 0xE0; temp1 = temp1 & 0x1F; break;
		case 6:		temp = temp & 0xC0; temp1 = temp1 & 0x3F; break;
		case 7:		temp = temp & 0x80; temp1 = temp1 & 0x7F; break;
		default:	break;
		}

		buf_B[0] = temp;
		UID[row - 1] = temp1 | temp;
		for(i = 1; i < length; i++)
		{
			UID[row - 1 + i] = buf_B[i];
		}
	}
}

/****************************************************************/
/*Ãû³Æ: Set_BitFraming_B                                          */
/*¹¦ÄÜ: ¸Ãº¯ÊýÉèÖÃ´ý·¢ËÍÊý¾ÝµÄ×Ö½ÚÊý                            */
/*ÊäÈë: row: ²úÉú³åÍ»µÄÐÐ                                       */
/*      col: ²úÉú³åÍ»µÄÁÐ                                       */
/*Êä³ö: N/A                                                     */
/****************************************************************/
 void Set_BitFraming_B(uchar row, uchar col)
{
	switch(row)
	{
	case 0:		buf_B[1] = 0x20; break;
	case 1:		buf_B[1] = 0x30; break;
	case 2:		buf_B[1] = 0x40; break;
	case 3:		buf_B[1] = 0x50; break;
	case 4:		buf_B[1] = 0x60; break;
	default:	break;
	}

	switch(col)
	{
	case 0:		write_reg_B(0x0F,0x00);  break;
	case 1:		write_reg_B(0x0F,0x11); buf_B[1] = (buf_B[1] | 0x01); break;
	case 2:		write_reg_B(0x0F,0x22); buf_B[1] = (buf_B[1] | 0x02); break;
	case 3:		write_reg_B(0x0F,0x33); buf_B[1] = (buf_B[1] | 0x03); break;
	case 4:		write_reg_B(0x0F,0x44); buf_B[1] = (buf_B[1] | 0x04); break;
	case 5:		write_reg_B(0x0F,0x55); buf_B[1] = (buf_B[1] | 0x05); break;
	case 6:		write_reg_B(0x0F,0x66); buf_B[1] = (buf_B[1] | 0x06); break;
	case 7:		write_reg_B(0x0F,0x77); buf_B[1] = (buf_B[1] | 0x07); break;
	default:	break;
	}
}

/****************************************************************/
/*Ãû³Æ: FM1702_Bus_Sel_B                                          */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔFM1702²Ù×÷µÄ×ÜÏß·½Ê½(²¢ÐÐ×ÜÏß,SPI)Ñ¡Ôñ      */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö:                                                         */
/*	    TRUE,  ×ÜÏßÑ¡Ôñ³É¹¦                                     */
/*	    FALSE, ×ÜÏßÑ¡ÔñÊ§°Ü  	                                  */
/****************************************************************/
uchar FM1702_Bus_Sel_B(void)
{
	uchar i,temp;
	
	write_reg_B(Page_Sel,0x80);
	write_reg_B(0x08,0x80);
	write_reg_B(0x10,0x80);
	write_reg_B(0x18,0x80);
	write_reg_B(0x20,0x80);
	write_reg_B(0x28,0x80);
	write_reg_B(0x30,0x80);
	write_reg_B(0x38,0x80);	
	for(i = 0; i < RF_TimeOut; i++)
	{
		temp=read_reg_B(Command);
		if(temp == 0x00)
		{
			write_reg_B(Page_Sel,0x00);
			write_reg_B(0x08,0x00);
			write_reg_B(0x10,0x00);
			write_reg_B(0x18,0x00);
			write_reg_B(0x20,0x00);
			write_reg_B(0x28,0x00);
			write_reg_B(0x30,0x00);
			write_reg_B(0x38,0x00);				
//			PR("1st FM1702 SPI init successfully \r\n");
			return TRUE;
		}
	}
//	PR("1st FM1702 SPI init fail \r\n");
	return FALSE;
}


/****************************************************************/
/*Ãû³Æ: Init_FM1702_1st                                          */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔFM1702³õÊ¼»¯                                */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö:                                                         */
/*	    TRUE,  ×ÜÏßÑ¡Ôñ³É¹¦                                     */
/*	    FALSE, ×ÜÏßÑ¡ÔñÊ§°Ü  	                                  */
/****************************************************************/
uchar Init_FM1702_1st(void)
{	
//  uint	i;
  uchar temp ;
	unsigned char out[16];
	FLASH_ReadByte(0x8007000,out,16);
	delay_ms(10);
  if(out[2]==0xff)
	{
		out[15] = 0x6f;
	}
	NSS_L;
//		SPI1_Close();
	RST_H;				        //¸´Î»¡
	delay_ms(50);
//	for (i = 0; i < 0x3fff; i++);
	RST_L;
//	for (i = 0; i < 0x3fff; i++);
//	SCK_L;
//delay_ms(50);		

//  delay_ms(50);	

//	  SPI1_Open();
		delay_ms(50);
//		SPI1_Init();
//	  SPI1_SetSpeed(SPI_BaudRatePrescaler_256);
//	delay_ms(50);
//  delay_B(1000);
/*ÐÂµÄ³õÊ¼»¯*/	
	temp = read_reg_B(0x05);
//	while(temp!=0x60) temp=read_reg_B(0x05);
	delay_ms(50);
	temp = FM1702_Bus_Sel_B();		 	//×ÜÏßÑ¡Ôñ
  if (temp == TRUE)
	{
		write_reg_B(CWConductance, 0x3F);
		write_reg_B(RxControl1, 0x73);
		write_reg_B(Rxcontrol2, 0x01);
	//	write_reg_B(ModWidth, 0x13);	
		write_reg_B(RxThreshold, out[15]);
//		write_reg_B(RxWait, 0x06);
				
		return TRUE;
	}	
	return FALSE;
/*Ô­À´µÄ³õÊ¼»¯*/	
//	temp = read_reg_B(Command);
//	PR("1st Command Reg: 0x%02x\r\n" , temp);
//  for (i = 0; i < 10 && temp!=0x00 ; i++) 
//	{

//		temp = read_reg_B(Command);
//		PR("1st Command Reg: 0x%02x\r\n" , temp);
//	}
//  if(temp == 0x00)
//	{
//		temp = FM1702_Bus_Sel_B();		 	//×ÜÏßÑ¡Ôñ
//		if (temp == TRUE)
//		{
//			write_reg_B(CWConductance, 0x3f);
//			write_reg_B(RxControl1, 0x73);
//		  write_reg_B(RxThreshold, out[15]);
//			

//			
//			return TRUE;
//      /*
//		  PR("1st FM1702 initialized ok !\r\n");	
//		  temp = read_reg_B(CWConductance);
//			PR("1st CWConductance Reg: 0x%02x\r\n" , temp);
//			temp = read_reg_B(RxControl1);
//			PR("1st RxControl1 Reg: 0x%02x\r\n" , temp);
//			temp = read_reg_B(RxThreshold);
//			PR("1st RxThreshold Reg: 0x%02x\r\n" , temp);	
//			*/
//		}	
//		return FALSE;
//	}
//	return FALSE;
}

/****************************************************************/
/*Ãû³Æ: Command_Send_B                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÏòRC531·¢ËÍÃüÁî¼¯µÄ¹¦ÄÜ                       */
/*												        		                          */
/*ÊäÈë:                                                         */
/*       count, ´ý·¢ËÍÃüÁî¼¯µÄ³¤¶È                              */
/*	     buff, Ö¸Ïò´ý·¢ËÍÊý¾ÝµÄÖ¸Õë                             */
/*       Comm_Set, ÃüÁîÂë                                       */
/*												       			                          */
/*Êä³ö:                                                         */
/*	     TRUE, ÃüÁî±»ÕýÈ·Ö´ÐÐ                                   */
/*	     FALSE, ÃüÁîÖ´ÐÐ´íÎó  	                                */
/****************************************************************/
uchar Command_Send_B(uchar count,volatile unsigned char * buff,uchar Comm_Set)
{
	uint  j;
	uchar temp;
	
	write_reg_B(Command, 0x00);
	Clear_FIFO_B();
  if (count != 0)
  {
	  Write_FIFO_B(count, buff);
  }	 
//	temp =read_reg_B(FIFO_Length);
//	PR("1st FIFO_Length: 0x%02x\r\n" , temp);
	write_reg_B(Command, Comm_Set);					//ÃüÁîÖ´ÐÐ
	
	for(j = 0; j< RF_TimeOut; j++)				//¼ì²éÃüÁîÖ´ÐÐ·ñ
	{
		temp =read_reg_B(Command);
		if(temp == 0x00)  
		{
			return TRUE;
		}
//		delay_B(1);
	}
//	temp = read_reg_B(Control);
//	if((temp & 0x08)==0x08)
//	{
////		temp =read_reg_B(Command);
////		write_reg_B(Command, 0x00);
//////		temp =read_reg_B(Command);
//////		temp =read_reg_B(0x0A);
//////    Init_FM1702();
////    Init_FM1702_1st();		
////		RFID_Powerdown_B();
////		RFID_Powerdown_A();	
//////    delay_ms(1000);		
//		__set_FAULTMASK(1);
//		NVIC_SystemReset();
//		return TRUE;
//	}			
//	PR("1st Command Read: 0x%02x\r\n" , temp);
	return FALSE;	
}

/****************************************************************/
/*Ãû³Æ: MIF_Halt                                                */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÔÝÍ£MIFARE¿¨                                  */
/*ÊäÈë: N/A                                                     */
/*Êä³ö: FM1702_OK: Ó¦´ðÕýÈ·                                     */
/* FM1702_PARITYERR: ÆæÅ¼Ð£Ñé´í                                 */
/* FM1702_CRCERR: CRCÐ£Ñé´í                                     */
/* FM1702_NOTAGERR: ÎÞ¿¨                                        */
/****************************************************************/
/*
uchar MIF_Halt(void)
{
	uchar	temp;
	uint	i;

	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
	write_reg_B(0x22,0x07);
	*buf_B = RF_CMD_HALT;
	*(buf_B + 1) = 0x00;
	temp = Command_Send_B(2, buf_B, Transmit);
	if(temp == TRUE)
	{
		for(i = 0; i < 0x50; i++)
		{
			_nop_();
		}

		return FM1702_OK;
	}
	else
	{
		temp = read_reg_B(0x0A);
		if((temp & 0x02) == 0x02)
		{
			return(FM1702_PARITYERR);
		}

		if((temp & 0x04) == 0x04)
		{
			return(FM1702_FRAMINGERR);
		}

		return(FM1702_NOTAGERR);
	}
}
*/
///////////////////////////////////////////////////////////////////////
// ×ª»»ÃÜÔ¿¸ñÊ½
///////////////////////////////////////////////////////////////////////
char M500HostCodeKey_B(unsigned char *uncoded, unsigned char *coded)   
{
    unsigned char cnt = 0;
    unsigned char ln  = 0;     
    unsigned char hn  = 0;      
    
    for (cnt = 0; cnt < 6; cnt++)
    {
        ln = uncoded[cnt] & 0x0F;
        hn = uncoded[cnt] >> 4;
        coded[cnt * 2 + 1] = (~ln << 4) | ln;
        coded[cnt * 2 ] = (~hn << 4) | hn;
    }
    return FM1702_OK;
}

/****************************************************************/
/*Ãû³Æ: Load_keyE2                                              */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ°ÑE2ÖÐÃÜÂë´æÈëFM1702µÄkeybufÖÐ             */
/*ÊäÈë: Secnr: EEÆðÊ¼µØÖ·                                       */
/*Êä³ö: True: ÃÜÔ¿×°ÔØ³É¹¦                                      */
/* False: ÃÜÔ¿×°ÔØÊ§°Ü                                          */
/****************************************************************/
uchar Load_keyE2_CPY(uchar *uncoded_keys)
{
  uchar temp;
  uchar coded_keys[13];
    
  M500HostCodeKey_B(uncoded_keys, coded_keys);
	temp = Command_Send_B(12, coded_keys, LoadKey);
	temp = read_reg_B(0x0A) & 0x40;
	if (temp == 0x40)
	{
		return FALSE;
	}
	PR("1st_RFID_load key OK!\r\n");
	return TRUE;
}

/****************************************************************/
/*Ãû³Æ: Request                                                 */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶Ô·ÅÈëRC531²Ù×÷·¶Î§Ö®ÄÚµÄ¿¨Æ¬µÄRequest²Ù×÷    */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      mode: ALL(¼à²âËùÒÔRC531²Ù×÷·¶Î§Ö®ÄÚµÄ¿¨Æ¬)			   	    */
/*	    STD(¼à²âÔÚRC531²Ù×÷·¶Î§Ö®ÄÚ´¦ÓÚHALT×´Ì¬µÄ¿¨Æ¬)          */
/*                                                              */
/*Êä³ö:                                                         */
/*	    FM222_NOTAGERR: ÎÞ¿¨                                    */
/*      FM222_OK: Ó¦´ðÕýÈ·                                      */
/*	    FM222_REQERR: Ó¦´ð´íÎó										              */
/****************************************************************/
uchar Request(uchar mode)
{
		write_reg_B(Command, 0x00);
		Clear_FIFO_B();	  
    uchar  temp;
    write_reg_B(TxControl,0x58);
    delay_us(10);
    write_reg_B(TxControl,0x5b);		 
    write_reg_B(CRCPresetLSB,0x63);
    write_reg_B(CWConductance,0x3f);
    buf_B[0] = mode;					             //RequestÄ£Ê½Ñ¡Ôñ
    write_reg_B(Bit_Frame,0x07);			       //·¢ËÍ7bit
    write_reg_B(ChannelRedundancy,0x03);	   //¹Ø±ÕCRC
    write_reg_B(TxControl,0x5b); 
    write_reg_B(Control,0x01);          		 //ÆÁ±ÎCRYPTO1Î»
    temp = Command_Send_B(1, buf_B, Transceive);
    if(temp == FALSE)
    {

	    return FM1702_NOTAGERR;
    }	
	
    Read_FIFO_B(buf_B);					           //´ÓFIFOÖÐ¶ÁÈ¡Ó¦´ðÐÅÏ¢
    temp = Judge_Req_B(buf_B);			         //ÅÐ¶ÏÓ¦´ðÐÅºÅÊÇ·ñÕýÈ·
    if (temp == TRUE)
    {
        tagtype[0] = buf_B[0];
        tagtype[1] = buf_B[1];
			  PR("1st_RFID_request OK!\r\n");
        return FM1702_OK;
    }

		return FM1702_REQERR;
		
}

/****************************************************************/
/*Ãû³Æ: AntiColl                                                */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶Ô·ÅÈëFM1702²Ù×÷·¶Î§Ö®ÄÚµÄ¿¨Æ¬µÄ·À³åÍ»¼ì²â    */
/*ÊäÈë: N/A                                                     */
/*Êä³ö: FM1702_NOTAGERR: ÎÞ¿¨                                   */
/* FM1702_BYTECOUNTERR: ½ÓÊÕ×Ö½Ú´íÎó                            */
/* FM1702_SERNRERR: ¿¨Æ¬ÐòÁÐºÅÓ¦´ð´íÎó                          */
/* FM1702_OK: ¿¨Æ¬Ó¦´ðÕýÈ·                                      */
/****************************************************************/
uchar AntiColl(void)
{
	uchar	temp;
	uchar	i;
	uchar	row, col;
	uchar	pre_row;

	row = 0;
	col = 0;
	pre_row = 0;
	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
	write_reg_B(0x13,0x3f);
	buf_B[0] = RF_CMD_ANTICOL;
	buf_B[1] = 0x20;
	write_reg_B(0x22,0x03);	                   // ¹Ø±ÕCRC,´ò¿ªÆæÅ¼Ð£Ñé
	temp = Command_Send_B(2, buf_B, Transceive);
	while(1)
	{
		if(temp == FALSE)
		{
			return(FM1702_NOTAGERR);
		}

		temp = read_reg_B(0x04);
		if(temp == 0)
		{
			return FM1702_BYTECOUNTERR;
		}

		Read_FIFO_B(buf_B);
		Save_UID_B(row, col, temp);			        // ½«ÊÕµ½µÄUID·ÅÈëUIDÊý×éÖÐ
	
		temp = read_reg_B(0x0A);				        // ÅÐ¶Ï½Ó…§Êý¾ÝÊÇ·ñ³ö´í
		temp = temp & 0x01;
		if(temp == 0x00)
		{
			temp = Check_UID_B();			            // Ð£ÑéÊÕµ½µÄUID
			if(temp == FALSE)
			{
				return(FM1702_SERNRERR);
			}
      PR("1st_RFID_AntiColl OK!\r\n");
			return(FM1702_OK);
		}
		else
		{
			temp = read_reg_B(0x0B);             // ¶ÁÈ¡³åÍ»¼ì²â¼Ä´æÆ÷ 
			row = temp / 8;
			col = temp % 8;
			buf_B[0] = RF_CMD_ANTICOL;
			Set_BitFraming_B(row + pre_row, col);	// ÉèÖÃ´ý·¢ËÍÊý¾ÝµÄ×Ö½ÚÊý 
			pre_row = pre_row + row;
			for(i = 0; i < pre_row + 1; i++)
			{
				buf_B[i + 2] = UID[i];
			}

			if(col != 0x00)
			{
				row = pre_row + 1;
			}
			else
			{
				row = pre_row;
			}
			temp = Command_Send_B(row + 2, buf_B, Transceive);
		}
	}
}

/****************************************************************/
/*Ãû³Æ: Select_Card                                             */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶Ô·ÅÈëFM1702²Ù×÷·¶Î§Ö®ÄÚµÄÄ³ÕÅ¿¨Æ¬½øÐÐÑ¡Ôñ    */
/*ÊäÈë: N/A                                                     */
/*Êä³ö: FM1702_NOTAGERR: ÎÞ¿¨                                   */
/* FM1702_PARITYERR: ÆæÅ¼Ð£Ñé´í                                 */
/* FM1702_CRCERR: CRCÐ£Ñé´í                                     */
/* FM1702_BYTECOUNTERR: ½ÓÊÕ×Ö½Ú´íÎó                            */
/* FM1702_OK: Ó¦´ðÕýÈ·                                          */
/* FM1702_SELERR: Ñ¡¿¨³ö´í                                      */
/****************************************************************/
uchar Select_Card(void)
{
	uchar	temp, i;

	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
	buf_B[0] = RF_CMD_SELECT;
	buf_B[1] = 0x70;
	for(i = 0; i < 5; i++)
	{
		buf_B[i + 2] = UID[i];
	}

	write_reg_B(0x22,0x0f);	                       // ¿ªÆôCRC,ÆæÅ¼Ð£ÑéÐ£Ñé 
	temp = Command_Send_B(7, buf_B, Transceive);
	if(temp == FALSE)
	{
		return(FM1702_NOTAGERR);
	}
	else
	{
		temp = read_reg_B(0x0A);
		if((temp & 0x02) == 0x02) return(FM1702_PARITYERR);
		if((temp & 0x04) == 0x04) return(FM1702_FRAMINGERR);
		if((temp & 0x08) == 0x08) return(FM1702_CRCERR);
		temp = read_reg_B(0x04);
		if(temp != 1) return(FM1702_BYTECOUNTERR);
		Read_FIFO_B(buf_B);	                      // ´ÓFIFOÖÐ¶ÁÈ¡Ó¦´ðÐÅÏ¢ 
		temp = *buf_B;
		//ÅÐ¶ÏÓ¦´ðÐÅºÅÊÇ·ñÕýÈ· 
		if((temp == 0x08) || (temp == 0x88) || (temp == 0x53) ||(temp == 0x18)) //S70 temp = 0x18	
		{
			PR("1st_RFID_SelectCard OK!\r\n");
			return(FM1702_OK);
		}
		else
			return(FM1702_SELERR);
	}
}

/****************************************************************/
/*Ãû³Æ: Authentication                                          */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÃÜÂëÈÏÖ¤µÄ¹ý³Ì                                */
/*ÊäÈë: UID: ¿¨Æ¬ÐòÁÐºÅµØÖ·                                     */
/* SecNR: ÉÈÇøºÅ                                                */
/* mode: Ä£Ê½                                                   */
/*Êä³ö: FM1702_NOTAGERR: ÎÞ¿¨                                   */
/* FM1702_PARITYERR: ÆæÅ¼Ð£Ñé´í                                 */
/* FM1702_CRCERR: CRCÐ£Ñé´í                                     */
/* FM1702_OK: Ó¦´ðÕýÈ·                                          */
/* FM1702_AUTHERR: È¨ÍþÈÏÖ¤ÓÐ´í                                 */
/****************************************************************/
uchar Authentication(uchar *UID, uchar SecNR, uchar mode)
{
	uchar	i;
	uchar	temp, temp1;

	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
	write_reg_B(0x13,0x3f);
	temp1 = read_reg_B(0x09);
	temp1 = temp1 & 0xf7;
	write_reg_B(0x09,temp1);
	if(mode == RF_CMD_AUTH_LB)			            // AUTHENT1 
		buf_B[0] = RF_CMD_AUTH_LB;
	else
		buf_B[0] = RF_CMD_AUTH_LA;
	buf_B[1] = SecNR * 4 + 3;
	for(i = 0; i < 4; i++)
	{
		buf_B[2 + i] = UID[i];
	}

	write_reg_B(0x22,0x0f);	                     // ¿ªÆôCRC,ÆæÅ¼Ð£ÑéÐ£Ñé 
	temp = Command_Send_B(6, buf_B, Authent1);
	if(temp == FALSE)
	{
//		PR("1st_RFID_Authentication FAIL--1!\r\n");
		return FM1702_NOTAGERR;
	}

	temp = read_reg_B(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp = Command_Send_B(0, buf_B, Authent2);	 // AUTHENT2 
	if(temp == FALSE)
	{
//		PR("1st_RFID_Authentication FAIL--2!\r\n");
		return FM1702_NOTAGERR;
	}

	temp = read_reg_B(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp1 = read_reg_B(0x09);
	temp1 = temp1 & 0x08;	                     // Crypto1on=1ÑéÖ¤Í¨¹ý 
	if(temp1 == 0x08)
	{
//		PR("1st_RFID_Authentication OK!\r\n");
		return FM1702_OK;
	}
//  PR("1st_RFID_Authentication FAIL--3!\r\n");
	return FM1702_AUTHERR;
}

/****************************************************************/
/*Ãû³Æ: MIF_Read                                                */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÁMIFARE¿¨¿éµÄÊýÖµ                            */
/*ÊäÈë: buff: »º³åÇøÊ×µØÖ·                                      */
/* Block_Adr: ¿éµØÖ·                                            */
/*Êä³ö: FM1702_NOTAGERR: ÎÞ¿¨                                   */
/* FM1702_PARITYERR: ÆæÅ¼Ð£Ñé´í                                 */
/* FM1702_CRCERR: CRCÐ£Ñé´í                                     */
/* FM1702_BYTECOUNTERR: ½ÓÊÕ×Ö½Ú´íÎó                            */
/* FM1702_OK: Ó¦´ðÕýÈ·                                          */
/****************************************************************/
uchar MIF_READ(uchar *buff, uchar Block_Adr)
{
	uchar	temp;

	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
	write_reg_B(0x13,0x3f);
	write_reg_B(0x22,0x0f);

	buff[0] = RF_CMD_READ;
	buff[1] = Block_Adr;
	temp = Command_Send_B(2, buff, Transceive);
	if(temp == 0)
	{
		return FM1702_NOTAGERR;
	}

	temp = read_reg_B(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp = read_reg_B(0x04);
	if(temp == 0x10)	                      // 8K¿¨¶ÁÊý¾Ý³¤¶ÈÎª16
	{
		Read_FIFO_B(buff);
		PR("1st_RFID_MIF_READ OK!\r\n");
		return FM1702_OK;
	}
	else if(temp == 0x04)	                  // Token¿¨¶ÁÊý¾Ý³¤¶ÈÎª16
	{
		Read_FIFO_B(buff);
		return FM1702_OK;
	}
	else
	{
		return FM1702_BYTECOUNTERR;
	}
}

/****************************************************************/
/*Ãû³Æ: MIF_Write                                               */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÐ´MIFARE¿¨¿éµÄÊýÖµ                            */
/*ÊäÈë: buff: »º³åÇøÊ×µØÖ·                                      */
/* Block_Adr: ¿éµØÖ·                                            */
/*Êä³ö: FM1702_NOTAGERR: ÎÞ¿¨                                   */
/* FM1702_BYTECOUNTERR: ½ÓÊÕ×Ö½Ú´íÎó                            */
/* FM1702_NOTAUTHERR: Î´¾­È¨ÍþÈÏÖ¤                              */
/* FM1702_EMPTY: Êý¾ÝÒç³ö´íÎó                                   */
/* FM1702_CRCERR: CRCÐ£Ñé´í                                     */
/* FM1702_PARITYERR: ÆæÅ¼Ð£Ñé´í                                 */
/* FM1702_WRITEERR: Ð´¿¨¿éÊý¾Ý³ö´í                              */
/* FM1702_OK: Ó¦´ðÕýÈ·                                          */
/****************************************************************/
uchar MIF_Write(uchar *buff, uchar Block_Adr)
{
	uchar	temp;
	uchar	F_buff[2];

	write_reg_B(0x23,0x63);
	write_reg_B(0x12,0x3f);
//	F_buff = temp1 + 0x10;
	write_reg_B(0x22,0x07);
	F_buff[0] = RF_CMD_WRITE;
	F_buff[1] = Block_Adr;
	temp = Command_Send_B(2, F_buff, Transceive);
	if(temp == FALSE)
	{
		return(FM1702_NOTAGERR);
	}

	temp = read_reg_B(0x04);
	if(temp == 0)
	{
		return(FM1702_BYTECOUNTERR);
	}

	Read_FIFO_B(F_buff);
	temp = *F_buff;
	switch(temp)
	{
		case 0x00:	return(FM1702_NOTAUTHERR);	     // ÔÝÊ±ÆÁ±ÎµôÐ´´íÎó
		case 0x04:	return(FM1702_EMPTY);
		case 0x0a:	break;
		case 0x01:	return(FM1702_CRCERR);
		case 0x05:	return(FM1702_PARITYERR);
		default:	return(FM1702_WRITEERR);
	}

	temp = Command_Send_B(16, buff, Transceive);
	if(temp == TRUE)
	{
		return(FM1702_OK);
	}
	else
	{
		temp = read_reg_B(0x0A);
		if((temp & 0x02) == 0x02)
			return(FM1702_PARITYERR);
		else if((temp & 0x04) == 0x04)
			return(FM1702_FRAMINGERR);
		else if((temp & 0x08) == 0x08)
			return(FM1702_CRCERR);
		else
			return(FM1702_WRITEERR);
	}
}

/****************************************************************/
/*Ãû³Æ: ReadID_FM1702                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔFM1702 ID¶ÁÈ¡  */
/*ÊäÈë: N/A                                                      */
/*Êä³ö: TRUE ¶ÁÈ¡³É¹¦²¢´òÓ¡                                      */
/*      FALSE¶ÁÈ¡Ê§°Ü·µ»ØFALSE                                   */
/*id1: 0x3088fe0303000001fef53a4395575e1b                        */
/*id2: 0x3088fe0303000001e8e63a4395575e82                        */
/*id3: 0x3088fe0303000001a4da3a4395575e78                        */
/*id4: 0x3088fe03030000014dd43a4395575e27                        */
/*id5: 0x3088fe030300000193e63a4395575e61                        */
/*id6: 0x3088fe0303000001baf93a4395575e8c                        */
/*****************************************************************/
uchar ReadID_1st_FM1702(void)
{
	uchar	temp;

	buf_B[0] = 0x00;
	buf_B[1] = 0x00;
	buf_B[2] = 0x10;
	
	temp = Command_Send_B(3, buf_B, ReadE2);
	delay_us(100);
//	delay_B(500);
	if(temp==TRUE)
	{
	Read_FIFO_B(buf_B);
 delay_us(100);		// ´ÓFIFOÖÐ¶ÁÈ¡Ó¦´ðÐÅÏ¢ 
//	delay_B(500);
//	PRH("1st FM1702 id: 0x");
//	for (i = 0; i < 16; ++i)PRH("%02x",buf_B[i]);

//	PRH("\r\n");
	return TRUE;	
	}
 return FALSE;

}


/****************************************************************/
/*Ãû³Æ: Standby_1st                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔFM1702 standby                               */
/*ÊäÈë: N/A                                                      */
/*Êä³ö: N/A                                                      */
/*****************************************************************/
void RFID_Powerdown_B(void)
{
//	/*¹ØµçÔ´*/
//	uchar	temp;
//	temp = read_reg_B(Control);
//	temp = temp | 0x10;
//	write_reg_B(Control,temp);
//	delay_us(100);
	/*¹ØÌìÏß*/
	uchar	temp;
	temp = read_reg_B(TxControl);
	temp = temp & 0xFC;
	write_reg_B(TxControl,temp);
	delay_us(100);				
}
void RFID_Off_B(void)
{
	/*¹ØµçÔ´*/
	uchar	temp;
	temp = read_reg_B(Control);
	temp = temp | 0x10;
	write_reg_B(Control,temp);
	delay_us(100);
//	/*¹ØÌìÏß*/
//	uchar	temp;
//	temp = read_reg_B(TxControl);
//	temp = temp & 0xFC;
//	write_reg_B(TxControl,temp);
//	delay_us(100);				
}
/****************************************************************/
/*Ãû³Æ: Wakeup_1st                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔFM1702 Wakeup                               */
/*ÊäÈë: N/A                                                      */
/*Êä³ö: N/A                                                      */
/*****************************************************************/
void RFID_Wakeup_B(void)
{
	/*¿ªµçÔ´*/
//	uchar	temp;
//	temp = read_reg_B(Control);
//	temp = temp & 0xef;
//	write_reg_B(Control,temp);
//	delay_us(100);
	/*¿ªÌìÏß*/
		uchar	temp;
	temp = read_reg_B(TxControl);
	temp = temp | 0x03;
	write_reg_B(TxControl,temp);
	delay_us(100);	
}
void RFID_On_B(void)
{
	/*¿ªµçÔ´*/
	uchar	temp;
	temp = read_reg_B(Control);
	temp = temp & 0xef;
	write_reg_B(Control,temp);
	delay_us(100);
//	/*¿ªÌìÏß*/
//		uchar	temp;
//	temp = read_reg_B(TxControl);
//	temp = temp | 0x03;
//	write_reg_B(TxControl,temp);
//	delay_us(100);	
}
/****************************************************************/
/*Ãû³Æ: Read_tag_1st                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÍêÕûµÄ¶Á¿¨²Ù×÷                                */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö: TRUE or FALSE                                           */
/****************************************************************/
uchar Read_tag_B(void)
{
	  unsigned char status ;
	  unsigned char loop = 0, s=0;
//    u8 i[8];	
  for(loop=0;loop<10;++loop)
		{	
	//		delay_us(100);
			status = Request(RF_CMD_REQUEST_ALL);		    //Ñ°¿¨
		  if(status == FM1702_OK)   
			{
			status = AntiColl();                     //³åÍ»¼ì²â
			if(status == FM1702_OK)
			{
			status=Select_Card();                  //Ñ¡¿¨
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY(DefaultKey);          //¼ÓÔØÃÜÂë
			if(status == TRUE)
   	  {
			status = Authentication(UID, 1, RF_CMD_AUTH_LA);	  //ÑéÖ¤1ÉÈÇøkeyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ(buffer_temp_1st,4);							       //¶Á¿¨£¬¶ÁÈ¡1ÉÈÇø0¿éÊý¾Ýµ½buffer_temp_2nd[0]-buffer_temp_2nd[15]			
			if (status == FM1702_OK)
			 {
				if ((buffer_temp_1st[0]==0xaa) && (buffer_temp_1st[1]==0xaa)) //Èç¹û±êÊ¶ÕýÈ·0xaaaa£¬Ôò¸´ÖÆµ½buffer£¬·µ»ØTRUE
				{
					memcpy(buffer_B,buffer_temp_1st,16);
					s++;
					return TRUE;
				}
			 } 

			}
			else
			 {				 

		
			if ((loop==10) & (s==0))
			{
//						RFID_Powerdown_B();
//						delay_ms(50);
//						Init_FM1702();
//						i[0] = chipID_B;
//						i[1] = 0x11;
//						i[2] = loop;
//						i[3] = s;																
//						Can_Send_Msg(i,8);
//						SPI1_Close();
						delay_ms(5);
//						SPI1_Open();
//									__set_FAULTMASK(1);
//			            NVIC_SystemReset();
			}
				}	
			}						
			}
			}
			}
/*			if (s>0)
	    {
			return TRUE; 	
	    }*/
		}
//						if ((loop==20) & (s==0))
//					{
//						RFID_Powerdown_B();
//						delay_ms(50);
//						Init_FM1702();
//						i[0] = chipID_B;
//						i[1] = 0x11;
//						i[2] = loop;
//						i[3] = s;																
//						Can_Send_Msg(i,8);
////									__set_FAULTMASK(1);
////			            NVIC_SystemReset();
//						 return FALSE;
//					}
		  if (s>0)
	    {
			return TRUE; 	
	    }

	 return FALSE;		
}
/****************************************************************/
/*Ãû³Æ: Read_check_B                                            */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÍêÕûµÄ¶Á¿¨²Ù×÷                                */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö: TRUE or FALSE                                           */
/****************************************************************/
uchar Read_check_B(void)
{
	  unsigned char status ;
			status = Request(RF_CMD_REQUEST_ALL);		    //Ñ°¿¨
		  if(status == FM1702_OK)   
			{
			status = AntiColl();                     //³åÍ»¼ì²â
			if(status == FM1702_OK)
			{
			status=Select_Card();                  //Ñ¡¿¨
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY(DefaultKey);          //¼ÓÔØÃÜÂë
			if(status == TRUE)
   	  {
			status = Authentication(UID, 1, RF_CMD_AUTH_LA);	  //ÑéÖ¤1ÉÈÇøkeyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ(buffer_temp_1st,4);							       //¶Á¿¨£¬¶ÁÈ¡1ÉÈÇø0¿éÊý¾Ýµ½buffer_temp_2nd[0]-buffer_temp_2nd[15]			
			if (status == FM1702_OK)
			 {
				if ((buffer_temp_1st[0]==0xaa) && (buffer_temp_1st[1]==0xaa)) //Èç¹û±êÊ¶ÕýÈ·0xaaaa£¬Ôò¸´ÖÆµ½buffer£¬·µ»ØTRUE
				{
					//memcpy(buffer_B,buffer_temp_1st,16);
					//s++;
					return TRUE; 	
				}
			 }
			}
			}						
			}
			}
			}

	 return FALSE;		
}
/****************************************************************/
/*Ãû³Æ: STM32_FM1702_GPIO_Init                                  */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶ÔSTM32 GPIO-¡·SPI³õÊ¼»¯                      */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö: N/A                                                     */
/****************************************************************/
void STM32_FM1702_GPIO_Init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_RST_PIN | MF522_RST_PIN_2nd | MF522_MOSI_PIN | MF522_SCK_PIN | MF522_NSS_PIN | MF522_NSS_PIN_2nd;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = MF522_MISO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(MF522_MISO_PORT, &GPIO_InitStructure);
}


/****************************************************************/
/*Ãû³Æ: Init_FM1702                                             */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖ¶Ô2¸öFM1702³õÊ¼»¯                             */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö: N/A                                                     */
/****************************************************************/
uchar Init_FM1702()
{
	uchar	temp1,temp2;
//	STM32_FM1702_GPIO_Init();
	temp1=Init_FM1702_1st();
	temp2=Init_FM1702_2nd();
	if(temp1==TRUE && temp2==TRUE)
	{
//		PRH("Two FM1702 initialized successfully\r\n");
		return TRUE;
	}
	return FALSE;
}

/****************************************************************/
/*Ãû³Æ: Read_status_1st                                         */
/*¹¦ÄÜ: ¸Ãº¯ÊýÊµÏÖÁ½´Î¶Á¿¨×´Ì¬±ä»¯                              */ 
/*												       			                          */
/*ÊäÈë:                                                         */
/*      N/A                                                     */
/*                                                              */
/*Êä³ö: TRUE or FALSE                                           */
/*      TRUE ÓÐ±ä»¯£¬FALSE ÎÞ±ä»¯                               */    
/****************************************************************/
uchar Read_status_B(void)
{
	unsigned char status;
  status =  Read_tag_B();
	if (status == TRUE && RFID_status_1st==1)  //ÓÐÔ¿³×ÇÒÎÞ±ä»¯
	{
		return FALSE;
	}
	else if(status == FALSE && RFID_status_1st==1) //ÎÞÔ¿³×ÇÒÓÐ±ä»¯
	{
		memset(buffer_B,0,16*sizeof(char));          //Çåbuffer 

		RFID_status_1st=0;
		return TRUE;
	}
	else if(status == TRUE && RFID_status_1st==0) //ÓÐÔ¿³×ÇÒÓÐ±ä»¯
	{
		RFID_status_1st=1;
		return TRUE;
	}
	else if(status == FALSE && RFID_status_1st==0) //ÎÞÔ¿³×ÇÒÎÞ±ä»¯
	{
		return FALSE;
	}
	else
	{
		return FALSE;
	}
}
//CAN ¶Á¼Ä´æÆ÷
/*
uchar can_read_reg(uchar SpiAddress)
{
 uchar read_reg_B(uchar SpiAddress);
}

//CAN Ð´¼Ä´æÆ÷
void can_write_reg(uchar SpiAddress,uchar dat)
{
	void write_reg_B(uchar SpiAddress,uchar dat);
}
*/
uchar calibration_B(void)
{

unsigned char calibration_array_1[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	
	unsigned char RxThreshold_value=0;
//	u32 out_data[2];
	int i,j,max=0,maxb,xxx,count;

	printf("start");

	max = calibration_array_1[1];
	maxb = 0;
		FLASH_ReadByte(0x8007000,calibration_array_1,16);
delay_ms(1000);
		RxThreshold_value =calibration_array_1[0];
		delay_us(100);
		count = (RxThreshold_value>>4);
		if(RxThreshold_value==0xBF)
		{
			calibration_array_1[count]=0;
			calibration_array_1[2] = 0x11;
			 FLASH_SetLatency(FLASH_Latency_1);       
       FLASH_Unlock();
			 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
			 FLASH_ErasePage(0x8007000);    
       FLASH_WriteByte(0x8007000,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);				
				LED1_G_OFF;
				LED1_R_ON;
				LED1_B_OFF;
			delay_ms(1000);
			relayBuf[0]=calibration_array_1[15];
		  SendPacket(chipID_B,CALIB_ACK);	
			delay_ms(1000);
			__set_FAULTMASK(1);
			NVIC_SystemReset();				
			return TRUE;
		}
		else if(RxThreshold_value==0xAF)
		{
			calibration_array_1[count]=0;
					for(i=5;i<11;i++)
			{
					if(calibration_array_1[i]>max)
					{
							max=calibration_array_1[i];
							maxb=i;
					}
			}
			calibration_array_1[0] = RxThreshold_value+0x10;
			calibration_array_1[15] = maxb*16+0x0f;
			 FLASH_SetLatency(FLASH_Latency_1);       
       FLASH_Unlock();
			 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
			 FLASH_ErasePage(0x8007000);    
       FLASH_WriteByte(0x8007000,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);				
						  LED1_G_ON;
				LED1_R_OFF;
				LED1_B_OFF;

			delay_ms(1000);		
			if(max>200)
			{
			__set_FAULTMASK(1);
			NVIC_SystemReset();			
			}
			if(max<=200)
			{
			calibration_array_1[0] = 0x77;
			calibration_array_1[count]=0;
			calibration_array_1[2] = 0x22;
			 FLASH_SetLatency(FLASH_Latency_1);       
       FLASH_Unlock();
			 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
			 FLASH_ErasePage(0x8007000);    
       FLASH_WriteByte(0x8007000,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);				

			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
				LED1_G_ON;
				LED1_R_ON;
				LED1_B_OFF;
			relayBuf[0]=0x77;
		  SendPacket(chipID_B,CALIB_ACK);
			delay_ms(1000);
//			__set_FAULTMASK(1);
//			NVIC_SystemReset();			
			}	
			
		}
		else
		{

		write_reg_B(RxThreshold,RxThreshold_value);// RxThreshold_value);
			calibration_array_1[count]=0;
//			RFID_Wakeup_B();
//			delay_ms(1);
			for(j=0;j<255;j++)
			{
			RFID_Wakeup_B();
			delay_ms(1);
				delay_us(100);
				xxx =  Read_check_B();
				delay_us(100);
			delay_ms(1);
			RFID_Powerdown_B();					
				if(xxx == TRUE) 
				{
					calibration_array_1[count]++;
				}
			}

			 calibration_array_1[0] = RxThreshold_value+0x10;
				
//			delay_ms(1);
//			RFID_Powerdown_B();		
			
			 FLASH_SetLatency(FLASH_Latency_1);       
       FLASH_Unlock();
			 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
			 FLASH_ErasePage(0x8007000);    
			 FLASH_WriteByte(0x8007000,calibration_array_1,16);
			 FLASH_Lock();  							
				delay_ms(1000);
					
			__set_FAULTMASK(1);
			NVIC_SystemReset();

		 }
		return TRUE;
}

