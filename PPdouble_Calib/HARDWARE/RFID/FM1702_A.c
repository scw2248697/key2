#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FM1702.h"
#include "delay.h"
#include "usart.h"
#include "string.h"
#include "spi.h"
#include "DataFlash.h"
#include "can.h"
#define uchar unsigned char

//uchar       cardtype;
uchar     	tagtype_2nd[2];	        /* ��Ƭ��ʶ�ַ� */
unsigned char buffer_A[16]; 
unsigned char buffer_temp_2nd[16];
/* FM1702�������� */
volatile unsigned char     	buf[10];            /* FM1702����ͽ��ջ����� */
 uchar     	UID_2nd[5];             /* ���к� */
//uchar       Secnr;			        /* ������ */
uchar RFID_status_2nd ;  /*����״̬��ʶ, 1--��Կ�� 0--��Կ��*/
//void delay(unsigned int dlength)
//{ 
//	unsigned int  i;
//	unsigned char j;
//	for (i=0;i<dlength;i++)
//	{
//		for (j=0;j<100;j++);
//	}
//} 

//SPI����
 unsigned char rev(uchar tem)
{
  return SPI1_ReadWriteByte(tem);
} 

//SPI����
 void Send(unsigned char var) 

{ 	
	SPI1_ReadWriteByte(var);
}                  

//���Ĵ���
 uchar read_reg(uchar SpiAddress)
{
	uchar rdata;
	//PR("2nd FM1702 SPI read_reg \r\n");
	SpiAddress=SpiAddress<<1;
	SpiAddress=SpiAddress | 0x80;
  delay_us(30);	
	NSS_L_2nd;
	NSS_H;
  delay_us(30);		
	Send(SpiAddress);
	rdata=rev(0);
	NSS_H_2nd;		
	return(rdata);
}

//д�Ĵ���
 void write_reg(uchar SpiAddress,uchar dat)
{
	//PR("2nd FM1702 SPI write_reg \r\n");
	SpiAddress = SpiAddress << 1;
	SpiAddress = SpiAddress & 0x7f;
	delay_us(30);
	NSS_L_2nd;
	NSS_H;
  delay_us(30);	
	Send(SpiAddress);		
	Send(dat);	
	NSS_H_2nd;
}

/****************************************************************/
/*����: Clear_FIFO                                              */
/*����: �ú���ʵ����FFIFO������                                */
/*												       			                          */
/*����:                                                         */
/*      N/A                                                     */
/*                                                              */
/*���:                                                         */
/*	    TRUE, FIFO�����                                        */
/*	    FALSE, FIFOδ�����  	                                  */
/****************************************************************/
 uchar Clear_FIFO(void)
{
	uchar temp;
	uint  i;
	
	temp =read_reg(Control);						//���FIFO
	temp = (temp | 0x01);
	write_reg(Control, temp);
	for(i = 0; i < RF_TimeOut; i++)			//���FIFO�Ƿ����
	{
		temp = read_reg(FIFO_Length);
		if(temp == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/****************************************************************/
/*����: Write_FIFO                                              */
/*����: �ú���ʵ����RC531��FIFO��д��x bytes����                */
/*												       			                          */
/*����:                                                         */
/*      count, ��д���ֽڵĳ���                                 */
/*	    buff, ָ���д�����ݵ�ָ��                              */
/*                                                              */
/*���:                                                         */
/*	    N/A                                                 		*/
/****************************************************************/
 void Write_FIFO(uchar count,volatile unsigned char *buff)
{
	uchar i;
	
	for(i = 0; i < count; i++)
	{
		write_reg(FIFO,*(buff + i));
	}
}

/****************************************************************/
/*����: Read_FIFO                                               */
/*����: �ú���ʵ�ִ�RC531��FIFO�ж���x bytes����                */
/*												        		                          */
/*����:                                                         */
/*       buff, ָ��������ݵ�ָ��                               */
/*                                                              */
/*���:                                                         */
/*	     N/A                                                 		*/
/****************************************************************/
 uchar Read_FIFO(volatile unsigned char *buff)
{
	uchar temp;
	uchar i;
	
	temp =read_reg(FIFO_Length);
	if (temp == 0)
	{
		return 0;
	}
	if (temp >= 24)						//temp=255ʱ,�������ѭ��
	{									        //�������FIFO_LengthԽ���ж�
		temp = 24;						 
	}
	for(i = 0;i < temp; i++)
	{
 		*(buff + i) =read_reg(FIFO);
		}
	return temp;
 }

/****************************************************************/
/*����: Judge_Req                                               */
/*����: �ú���ʵ�ֶԿ�Ƭ��λӦ���źŵ��ж�                      */
/*												       			                          */
/*����:                                                         */
/*       *buff, ָ��Ӧ�����ݵ�ָ��                              */
/*                                                              */
/*���:                                                         */
/*	     TRUE, ��ƬӦ���ź���ȷ                                 */
/*       FALSE, ��ƬӦ���źŴ���                                */
/****************************************************************/
 uchar Judge_Req(volatile unsigned char *buff)
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
/*����: Check_UID                                               */
/*����: �ú���ʵ�ֶ��յ��Ŀ�Ƭ�����кŵ��ж�                    */
/*����: N/A                                                     */
/*���: TRUE: ���к���ȷ                                        */
/* FALSE: ���кŴ���                                            */
/****************************************************************/
 uchar Check_UID(void)
{
	uchar	temp;
	uchar	i;

	temp = 0x00;
	for(i = 0; i < 5; i++)
	{
		temp = temp ^ UID_2nd[i];
	}

	if(temp == 0)
	{

		return TRUE;
	}

	return FALSE;
}

/****************************************************************/
/*����: Save_UID                                                */
/*����: �ú���ʵ�ֱ��濨Ƭ�յ������к�                          */
/*����: row: ������ͻ����                                       */
/* col: ������ͻ����                                            */
/* length: �Ӆ�����UID���ݳ���                                  */
/*���: N/A                                                     */
/****************************************************************/
 void Save_UID(uchar row, uchar col, uchar length)
{
	uchar	i;
	uchar	temp;
	uchar	temp1;

	if((row == 0x00) && (col == 0x00))
	{
		for(i = 0; i < length; i++)
		{
			UID_2nd[i] = buf[i];
		}
	}
	else
	{
		temp = buf[0];
		temp1 = UID_2nd[row - 1];
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

		buf[0] = temp;
		UID_2nd[row - 1] = temp1 | temp;
		for(i = 1; i < length; i++)
		{
			UID_2nd[row - 1 + i] = buf[i];
		}
	}
}

/****************************************************************/
/*����: Set_BitFraming                                          */
/*����: �ú������ô��������ݵ��ֽ���                            */
/*����: row: ������ͻ����                                       */
/*      col: ������ͻ����                                       */
/*���: N/A                                                     */
/****************************************************************/
 void Set_BitFraming(uchar row, uchar col)
{
	switch(row)
	{
	case 0:		buf[1] = 0x20; break;
	case 1:		buf[1] = 0x30; break;
	case 2:		buf[1] = 0x40; break;
	case 3:		buf[1] = 0x50; break;
	case 4:		buf[1] = 0x60; break;
	default:	break;
	}

	switch(col)
	{
	case 0:		write_reg(0x0F,0x00);  break;
	case 1:		write_reg(0x0F,0x11); buf[1] = (buf[1] | 0x01); break;
	case 2:		write_reg(0x0F,0x22); buf[1] = (buf[1] | 0x02); break;
	case 3:		write_reg(0x0F,0x33); buf[1] = (buf[1] | 0x03); break;
	case 4:		write_reg(0x0F,0x44); buf[1] = (buf[1] | 0x04); break;
	case 5:		write_reg(0x0F,0x55); buf[1] = (buf[1] | 0x05); break;
	case 6:		write_reg(0x0F,0x66); buf[1] = (buf[1] | 0x06); break;
	case 7:		write_reg(0x0F,0x77); buf[1] = (buf[1] | 0x07); break;
	default:	break;
	}
}

/****************************************************************/
/*����: FM1702_Bus_Sel                                          */
/*����: �ú���ʵ�ֶ�FM1702���������߷�ʽ(��������,SPI)ѡ��      */ 
/*												       			                          */
/*����:                                                         */
/*      N/A                                                     */
/*                                                              */
/*���:                                                         */
/*	    TRUE,  ����ѡ��ɹ�                                     */
/*	    FALSE, ����ѡ��ʧ��  	                                  */
/****************************************************************/
 uchar FM1702_Bus_Sel(void)
{
	uchar i,temp;
	
	write_reg(Page_Sel,0x80);
	write_reg(0x08,0x80);
	write_reg(0x10,0x80);
	write_reg(0x18,0x80);
	write_reg(0x20,0x80);
	write_reg(0x28,0x80);
	write_reg(0x30,0x80);
	write_reg(0x38,0x80);
	for(i = 0; i < RF_TimeOut; i++)
	{
		temp=read_reg(Command);
		if(temp == 0x00)
		{
			write_reg(Page_Sel,0x00);
			write_reg(0x08,0x00);
			write_reg(0x10,0x00);
			write_reg(0x18,0x00);
			write_reg(0x20,0x00);
			write_reg(0x28,0x00);
			write_reg(0x30,0x00);
			write_reg(0x38,0x00);			
//			PR("2nd FM1702 SPI init successfully \r\n");
			return TRUE;
		}
	}
//	PR("2nd FM1702 SPI init fail \r\n");
	return FALSE;
}

/****************************************************************/
/*����: Init_FM1702_2nd                                          */
/*����: �ú���ʵ�ֶ�FM1702��ʼ��                                */ 
/*												       			                          */
/*����:                                                         */
/*      N/A                                                     */
/*                                                              */
/*���:                                                         */
/*	    TRUE,  ����ѡ��ɹ�                                     */
/*	    FALSE, ����ѡ��ʧ��  	                                  */
/****************************************************************/
uchar Init_FM1702_2nd(void)
{	
//    uint	i;
  	uchar temp;
		unsigned char out[16];
	FLASH_ReadByte(0x8007500,out,16);
	delay_ms(10);
	if(out[2]==0xff)
	{
	 out[15] = 0x6f;
	}
	NSS_L_2nd;
//		SPI1_Close();
	RST_H_2nd;				        //��λ
	
	delay_ms(50);
//	for (i = 0; i < 0x3fff; i++);
	RST_L_2nd;
	delay_ms(50);	


 // delay_ms(50);	
 // SPI1_Open();
//	delay_ms(50);
//		SPI1_Init();
//	  SPI1_SetSpeed(SPI_BaudRatePrescaler_256);
//	delay_ms(50);
//  delay(1000);
/*�µĳ�ʼ��*/	
	temp = read_reg(0x05);
//	while(temp!=0x60) temp=read_reg(0x05);
	delay_ms(50);
	temp = FM1702_Bus_Sel();		 	//����ѡ��
  if (temp == TRUE)
	{
		write_reg(CWConductance, 0x3F);
		write_reg(RxControl1, 0x73);
		write_reg(Rxcontrol2, 0x01);
//		write_reg(ModWidth, 0x13);	
		write_reg(RxThreshold, out[15]);
//		write_reg(RxWait, 0x06);
				
		return TRUE;
	}	
	return FALSE;	
//	for (i = 0; i < 0x3fff; i++);
//	delay(1000);
//SCK_L;	 

//	temp = read_reg(Command);
//	PR("2nd Command Reg: 0x%02x\r\n" , temp);
//	//while(temp!=0x00)
//  for (i = 0; i < 10 && temp!=0x00 ; i++)	
//	{
//		temp = read_reg(Command);
//		PR("2nd Command Reg: 0x%02x\r\n" , temp);
//	}

///*	temp = read_reg(0x05);
//	PR("2nd SecondaryStatus: %x\r\n" , temp);
//	while(temp!=0x60)    
//	{
//		temp = read_reg(0x05);
//		PR("2nd SecondaryStatus: %x\r\n" , temp);
//	}*/
//	if(temp == 0x00)
//	{
//		temp = FM1702_Bus_Sel();		 	//����ѡ��
//		if (temp == TRUE)
//		{

//			write_reg(CWConductance, 0x3f);
//			write_reg(RxControl1, 0x73);
//		  write_reg(RxThreshold, out[15]);//0x5f,0x6f,0x7f ok --2017/06/06--
//			
//		
//			return TRUE;
//		}
//		return FALSE;
//	} 
//  return FALSE;
}

/****************************************************************/
/*����: Command_Send                                            */
/*����: �ú���ʵ����RC531��������Ĺ���                       */
/*												        		                          */
/*����:                                                         */
/*       count, ����������ĳ���                              */
/*	     buff, ָ����������ݵ�ָ��                             */
/*       Comm_Set, ������                                       */
/*												       			                          */
/*���:                                                         */
/*	     TRUE, �����ȷִ��                                   */
/*	     FALSE, ����ִ�д���  	                                */
/****************************************************************/
 uchar Command_Send(uchar count,volatile unsigned char * buff,uchar Comm_Set)
{
	uint  j;
	uchar temp;
	
	write_reg(Command, 0x00);
	Clear_FIFO();
  if (count != 0)
  {
	  Write_FIFO(count, buff);
  }	 
//	temp =read_reg(FIFO_Length);
//	PR("2nd FIFO_Length: 0x%02x\r\n" , temp);
	write_reg(Command, Comm_Set);					//����ִ��
	
	for(j = 0; j< RF_TimeOut; j++)				//�������ִ�з�
	{
		temp =read_reg(Command);
		if(temp == 0x00)  
		{
			return TRUE;
		}
 // delay(1);		
	}
//	temp = read_reg(Control);
//	if((temp & 0x08)==0x08)
//	{
////		temp =read_reg(Command);
////		write_reg(Command, 0x00);
//////		temp =read_reg(Command);
//////		temp =read_reg(0x0A);
//////    Init_FM1702();
////    Init_FM1702_2nd();		
////		RFID_Powerdown_B();
////		RFID_Powerdown_A();	
//////    delay_ms(1000);	
//			__set_FAULTMASK(1);
//			NVIC_SystemReset();		
//		return TRUE;
//	}			
//	PR("2nd Command Read: 0x%02x\r\n" , temp);
	return FALSE;	
}

/****************************************************************/
/*����: MIF_Halt                                                */
/*����: �ú���ʵ����ͣMIFARE��                                  */
/*����: N/A                                                     */
/*���: FM1702_OK: Ӧ����ȷ                                     */
/* FM1702_PARITYERR: ��żУ���                                 */
/* FM1702_CRCERR: CRCУ���                                     */
/* FM1702_NOTAGERR: �޿�                                        */
/****************************************************************/
/*
uchar MIF_Halt(void)
{
	uchar	temp;
	uint	i;

	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
	write_reg(0x22,0x07);
	*buf = RF_CMD_HALT;
	*(buf + 1) = 0x00;
	temp = Command_Send(2, buf, Transmit);
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
		temp = read_reg(0x0A);
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
// ת����Կ��ʽ
///////////////////////////////////////////////////////////////////////
 char M500HostCodeKey(unsigned char *uncoded, unsigned char *coded)   
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
/*����: Load_keyE2                                              */
/*����: �ú���ʵ�ְ�E2���������FM1702��keybuf��             */
/*����: Secnr: EE��ʼ��ַ                                       */
/*���: True: ��Կװ�سɹ�                                      */
/* False: ��Կװ��ʧ��                                          */
/****************************************************************/
uchar Load_keyE2_CPY_2nd(uchar *uncoded_keys)
{
  uchar temp;
  uchar coded_keys[13];
    
  M500HostCodeKey(uncoded_keys, coded_keys);
	temp = Command_Send(12, coded_keys, LoadKey);
	temp = read_reg(0x0A) & 0x40; //check ErrorFlag bit6, KeyErr
	if (temp == 0x40)
	{
		return FALSE;
	}
	PR("2nd_RFID_load key OK!\r\n");
	return TRUE;
}

/****************************************************************/
/*����: Request                                                 */
/*����: �ú���ʵ�ֶԷ���RC531������Χ֮�ڵĿ�Ƭ��Request����    */ 
/*												       			                          */
/*����:                                                         */
/*      mode: ALL(�������RC531������Χ֮�ڵĿ�Ƭ)			   	    */
/*	    STD(�����RC531������Χ֮�ڴ���HALT״̬�Ŀ�Ƭ)          */
/*                                                              */
/*���:                                                         */
/*	    FM222_NOTAGERR: �޿�                                    */
/*      FM222_OK: Ӧ����ȷ                                      */
/*	    FM222_REQERR: Ӧ�����										              */
/****************************************************************/
uchar Request_2nd(uchar mode)
{
		write_reg(Command, 0x00);
		Clear_FIFO();
    uchar  temp;
    write_reg(TxControl,0x58);
	  delay_us(10);
 //   delay(1);
    write_reg(TxControl,0x5b);		 
    write_reg(CRCPresetLSB,0x63);
    write_reg(CWConductance,0x3f);
    buf[0] = mode;					             //Requestģʽѡ��
    write_reg(Bit_Frame,0x07);			       //����7bit
    write_reg(ChannelRedundancy,0x03);	   //�ر�CRC
    write_reg(TxControl,0x5b); 
    write_reg(Control,0x01);          		 //����CRYPTO1λ
    temp = Command_Send(1, buf, Transceive);
    if(temp == FALSE)
    {
	    return FM1702_NOTAGERR;
    }	
	
    Read_FIFO(buf);					           //��FIFO�ж�ȡӦ����Ϣ
    temp = Judge_Req(buf);			         //�ж�Ӧ���ź��Ƿ���ȷ
    if (temp == TRUE)
    {
        tagtype_2nd[0] = buf[0];
        tagtype_2nd[1] = buf[1];
			  PR("2nd_RFID_request OK!\r\n");
        return FM1702_OK;
    }

		return FM1702_REQERR;
		
}

/****************************************************************/
/*����: AntiColl                                                */
/*����: �ú���ʵ�ֶԷ���FM1702������Χ֮�ڵĿ�Ƭ�ķ���ͻ���    */
/*����: N/A                                                     */
/*���: FM1702_NOTAGERR: �޿�                                   */
/* FM1702_BYTECOUNTERR: �����ֽڴ���                            */
/* FM1702_SERNRERR: ��Ƭ���к�Ӧ�����                          */
/* FM1702_OK: ��ƬӦ����ȷ                                      */
/****************************************************************/
uchar AntiColl_2nd(void)
{
	uchar	temp;
	uchar	i;
	uchar	row, col;
	uchar	pre_row;

	row = 0;
	col = 0;
	pre_row = 0;
	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
	write_reg(0x13,0x3f);
	buf[0] = RF_CMD_ANTICOL;
	buf[1] = 0x20;
	write_reg(0x22,0x03);	                   // �ر�CRC,����żУ��
	temp = Command_Send(2, buf, Transceive);
	while(1)
	{
		if(temp == FALSE)
		{
			return(FM1702_NOTAGERR);
		}

		temp = read_reg(0x04);
		if(temp == 0)
		{
			return FM1702_BYTECOUNTERR;
		}

		Read_FIFO(buf);
		Save_UID(row, col, temp);			        // ���յ���UID����UID������
	
		temp = read_reg(0x0A);				        // �жϽӅ������Ƿ����
		temp = temp & 0x01;
		if(temp == 0x00)
		{
			temp = Check_UID();			            // У���յ���UID
			if(temp == FALSE)
			{
				return(FM1702_SERNRERR);
			}
      PR("2nd_RFID_AntiColl OK!\r\n");
			return(FM1702_OK);
		}
		else
		{
			temp = read_reg(0x0B);             // ��ȡ��ͻ���Ĵ��� 
			row = temp / 8;
			col = temp % 8;
			buf[0] = RF_CMD_ANTICOL;
			Set_BitFraming(row + pre_row, col);	// ���ô��������ݵ��ֽ��� 
			pre_row = pre_row + row;
			for(i = 0; i < pre_row + 1; i++)
			{
				buf[i + 2] = UID_2nd[i];
			}

			if(col != 0x00)
			{
				row = pre_row + 1;
			}
			else
			{
				row = pre_row;
			}
			temp = Command_Send(row + 2, buf, Transceive);
		}
	}
}

/****************************************************************/
/*����: Select_Card                                             */
/*����: �ú���ʵ�ֶԷ���FM1702������Χ֮�ڵ�ĳ�ſ�Ƭ����ѡ��    */
/*����: N/A                                                     */
/*���: FM1702_NOTAGERR: �޿�                                   */
/* FM1702_PARITYERR: ��żУ���                                 */
/* FM1702_CRCERR: CRCУ���                                     */
/* FM1702_BYTECOUNTERR: �����ֽڴ���                            */
/* FM1702_OK: Ӧ����ȷ                                          */
/* FM1702_SELERR: ѡ������                                      */
/****************************************************************/
uchar Select_Card_2nd(void)
{
	uchar	temp, i;

	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
	buf[0] = RF_CMD_SELECT;
	buf[1] = 0x70;
	for(i = 0; i < 5; i++)
	{
		buf[i + 2] = UID_2nd[i];
	}

	write_reg(0x22,0x0f);	                       // ����CRC,��żУ��У�� 
	temp = Command_Send(7, buf, Transceive);
	if(temp == FALSE)
	{
		return(FM1702_NOTAGERR);
	}
	else
	{
		temp = read_reg(0x0A);
		if((temp & 0x02) == 0x02) return(FM1702_PARITYERR);
		if((temp & 0x04) == 0x04) return(FM1702_FRAMINGERR);
		if((temp & 0x08) == 0x08) return(FM1702_CRCERR);
		temp = read_reg(0x04);
		if(temp != 1) return(FM1702_BYTECOUNTERR);
		Read_FIFO(buf);	                      // ��FIFO�ж�ȡӦ����Ϣ 
		temp = *buf;
		//�ж�Ӧ���ź��Ƿ���ȷ 
		if((temp == 0x08) || (temp == 0x88) || (temp == 0x53) ||(temp == 0x18)) //S70 temp = 0x18	
		{
			PR("2nd_RFID_SelectCard OK!\r\n");
			return(FM1702_OK);
		}
		else
			return(FM1702_SELERR);
	}
}

/****************************************************************/
/*����: Authentication                                          */
/*����: �ú���ʵ��������֤�Ĺ���                                */
/*����: UID: ��Ƭ���кŵ�ַ                                     */
/* SecNR: ������                                                */
/* mode: ģʽ                                                   */
/*���: FM1702_NOTAGERR: �޿�                                   */
/* FM1702_PARITYERR: ��żУ���                                 */
/* FM1702_CRCERR: CRCУ���                                     */
/* FM1702_OK: Ӧ����ȷ                                          */
/* FM1702_AUTHERR: Ȩ����֤�д�                                 */
/****************************************************************/
uchar Authentication_2nd(uchar *UID_2nd, uchar SecNR, uchar mode)
{
	uchar	i;
	uchar	temp, temp1;

	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
	write_reg(0x13,0x3f);
	temp1 = read_reg(0x09);
	temp1 = temp1 & 0xf7;
	write_reg(0x09,temp1);
	if(mode == RF_CMD_AUTH_LB)			            // AUTHENT1 
		buf[0] = RF_CMD_AUTH_LB;
	else
		buf[0] = RF_CMD_AUTH_LA;
	buf[1] = SecNR * 4 + 3;
	for(i = 0; i < 4; i++)
	{
		buf[2 + i] = UID_2nd[i];
	}

	write_reg(0x22,0x0f);	                     // ����CRC,��żУ��У�� 
	temp = Command_Send(6, buf, Authent1);
	if(temp == FALSE)
	{
		PR("2nd_RFID_Authentication FAIL--1!\r\n");
		return FM1702_NOTAGERR;
	}

	temp = read_reg(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp = Command_Send(0, buf, Authent2);	 // AUTHENT2 
	if(temp == FALSE)
	{ 
//		PR("2nd_RFID_Authentication FAIL--2!\r\n");
		return FM1702_NOTAGERR;
	}

	temp = read_reg(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp1 = read_reg(0x09);
	temp1 = temp1 & 0x08;	                     // Crypto1on=1��֤ͨ�� 
	if(temp1 == 0x08)
	{
		PR("2nd_RFID_Authentication OK!\r\n");
		return FM1702_OK;
	}
  PR("2nd_RFID_Authentication FAIL--3!\r\n");
	return FM1702_AUTHERR;
}

/****************************************************************/
/*����: MIF_Read                                                */
/*����: �ú���ʵ�ֶ�MIFARE�������ֵ                            */
/*����: buff: �������׵�ַ                                      */
/* Block_Adr: ���ַ                                            */
/*���: FM1702_NOTAGERR: �޿�                                   */
/* FM1702_PARITYERR: ��żУ���                                 */
/* FM1702_CRCERR: CRCУ���                                     */
/* FM1702_BYTECOUNTERR: �����ֽڴ���                            */
/* FM1702_OK: Ӧ����ȷ                                          */
/****************************************************************/
uchar MIF_READ_2nd(uchar *buff, uchar Block_Adr)
{
	uchar	temp;

	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
	write_reg(0x13,0x3f);
	write_reg(0x22,0x0f);

	buff[0] = RF_CMD_READ;
	buff[1] = Block_Adr;
	temp = Command_Send(2, buff, Transceive);
	if(temp == 0)
	{
		return FM1702_NOTAGERR;
	}

	temp = read_reg(0x0A);
	if((temp & 0x02) == 0x02) return FM1702_PARITYERR;
	if((temp & 0x04) == 0x04) return FM1702_FRAMINGERR;
	if((temp & 0x08) == 0x08) return FM1702_CRCERR;
	temp = read_reg(0x04);
	if(temp == 0x10)	                      // 8K�������ݳ���Ϊ16
	{
		Read_FIFO(buff);
		PR("2nd_RFID_MIF_READ OK!\r\n");
		return FM1702_OK;
	}
	else if(temp == 0x04)	                  // Token�������ݳ���Ϊ16
	{
		Read_FIFO(buff);
		return FM1702_OK;
	}
	else
	{
		return FM1702_BYTECOUNTERR;
	}
}

/****************************************************************/
/*����: MIF_Write                                               */
/*����: �ú���ʵ��дMIFARE�������ֵ                            */
/*����: buff: �������׵�ַ                                      */
/* Block_Adr: ���ַ                                            */
/*���: FM1702_NOTAGERR: �޿�                                   */
/* FM1702_BYTECOUNTERR: �����ֽڴ���                            */
/* FM1702_NOTAUTHERR: δ��Ȩ����֤                              */
/* FM1702_EMPTY: �����������                                   */
/* FM1702_CRCERR: CRCУ���                                     */
/* FM1702_PARITYERR: ��żУ���                                 */
/* FM1702_WRITEERR: д�������ݳ���                              */
/* FM1702_OK: Ӧ����ȷ                                          */
/****************************************************************/
uchar MIF_Write_2nd(uchar *buff, uchar Block_Adr)
{
	uchar	temp;
	uchar	F_buff[2];

	write_reg(0x23,0x63);
	write_reg(0x12,0x3f);
//	F_buff = temp1 + 0x10;
	write_reg(0x22,0x07);
	F_buff[0] = RF_CMD_WRITE;
	F_buff[1] = Block_Adr;
	temp = Command_Send(2, F_buff, Transceive);
	if(temp == FALSE)
	{
		return(FM1702_NOTAGERR);
	}

	temp = read_reg(0x04);
	if(temp == 0)
	{
		return(FM1702_BYTECOUNTERR);
	}

	Read_FIFO(F_buff);
	temp = *F_buff;
	switch(temp)
	{
		case 0x00:	return(FM1702_NOTAUTHERR);	     // ��ʱ���ε�д����
		case 0x04:	return(FM1702_EMPTY);
		case 0x0a:	break;
		case 0x01:	return(FM1702_CRCERR);
		case 0x05:	return(FM1702_PARITYERR);
		default:	return(FM1702_WRITEERR);
	}

	temp = Command_Send(16, buff, Transceive);
	if(temp == TRUE)
	{
		return(FM1702_OK);
	}
	else
	{
		temp = read_reg(0x0A);
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
/*����: ReadID_FM1702                                            */
/*����: �ú���ʵ�ֶ�FM1702 ID��ȡ  */
/*����: N/A                                                      */
/*���: TRUE ��ȡ�ɹ�����ӡ                                      */
/*      FALSE��ȡʧ�ܷ���FALSE                                   */
/*id1: 0x3088fe0303000001fef53a4395575e1b                        */
/*id2: 0x3088fe0303000001e8e63a4395575e82                        */
/*id3: 0x3088fe0303000001a4da3a4395575e78                        */
/*id4: 0x3088fe03030000014dd43a4395575e27                        */
/*id5: 0x3088fe030300000193e63a4395575e61                        */
/*id6: 0x3088fe0303000001baf93a4395575e8c                        */
/*****************************************************************/
uchar ReadID_2nd_FM1702(void)
{
	uchar	temp;

	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x10;
	
	temp = Command_Send(3, buf, ReadE2);
	delay_us(100);
	//delay(500);
	if(temp==TRUE)
	{
	Read_FIFO(buf);	                      // ��FIFO�ж�ȡӦ����Ϣ 
	delay_us(100);	
//	delay(500);
//	PRH("2nd FM1702 id: 0x");
//	for (i = 0; i < 16; ++i)PRH("%02x",buf[i]);
//	PRH("\r\n");
	return TRUE;	
	}
 return FALSE;
}

/****************************************************************/
/*����: Standby_2nd                                            */
/*����: �ú���ʵ�ֶ�FM1702 standby                               */
/*����: N/A                                                      */
/*���: TRUE ��ȡ�ɹ�����ӡ                                      */
/*      FALSE��ȡʧ�ܷ���FALSE                                   */
/*****************************************************************/
void RFID_Powerdown_A(void)
{
//	/*�ص�Դ*/
//	uchar	temp;
//	temp = read_reg(Control);
//	temp = temp | 0x10;
//	write_reg(Control,temp);
//	delay_us(100);
	/*������*/
	uchar	temp;
	temp = read_reg(TxControl);
	temp = temp & 0xFC;
	write_reg(TxControl,temp);
	delay_us(100);	
	

	
}
void RFID_Off_A(void)
{
	/*�ص�Դ*/
	uchar	temp;
	temp = read_reg(Control);
	temp = temp | 0x10;
	write_reg(Control,temp);
	delay_us(100);
	/*������*/
//	uchar	temp;
//	temp = read_reg(TxControl);
//	temp = temp & 0xFC;
//	write_reg(TxControl,temp);
//	delay_us(100);	
	

	
}
/****************************************************************/
/*����: Wakeup_2nd                                            */
/*����: �ú���ʵ�ֶ�FM1702 Wakeup                               */
/*����: N/A                                                      */
/*���: TRUE ��ȡ�ɹ�����ӡ                                      */
/*      FALSE��ȡʧ�ܷ���FALSE                                   */
/*****************************************************************/
void RFID_Wakeup_A(void)
{
	/*����Դ*/
//	uchar	temp;
//	temp = read_reg(Control);
//	temp = temp & 0xef;
//	write_reg(Control,temp);
//	delay_us(100);
	/*������*/
		uchar	temp;
	temp = read_reg(TxControl);
	temp = temp | 0x03;
	write_reg(TxControl,temp);
	delay_us(100);	
}
void RFID_On_A(void)
{
	/*����Դ*/
	uchar	temp;
	temp = read_reg(Control);
	temp = temp & 0xef;
	write_reg(Control,temp);
	delay_us(100);
//	/*������*/
//		uchar	temp;
//	temp = read_reg(TxControl);
//	temp = temp | 0x03;
//	write_reg(TxControl,temp);
//	delay_us(100);	
}
uchar Read_tag_A(void)
{
	 unsigned char status;
	 unsigned char loop = 0, s=0;
//	u8 i[8];
	unsigned char out_A[16];
	unsigned char out_B[16];
	unsigned char chipID[2];

  for(loop=0;loop<10;++loop)
		{	
  //    delay_us(100);
			status = Request_2nd(RF_CMD_REQUEST_ALL);		    //Ѱ��
		  if(status == FM1702_OK)   
			{
			status = AntiColl_2nd();                     //��ͻ���
			if(status == FM1702_OK)
			{
			status=Select_Card_2nd();                  //ѡ��
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY_2nd(DefaultKey);          //��������
			if(status == TRUE)
   	  {
			status = Authentication_2nd(UID_2nd, 1, RF_CMD_AUTH_LA);	  //��֤1����keyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ_2nd(buffer_temp_2nd,4);							       //��������ȡ1����0�����ݵ�buffer_temp_2nd[0]-buffer_temp_2nd[15]
				if (status == FM1702_OK)
				{
				  if ((buffer_temp_2nd[0]==0xaa) && (buffer_temp_2nd[1]==0xaa)) //�����ʶ��ȷ0xaaaa�����Ƶ�buffer������TRUE
				  {
					  memcpy(buffer_A,buffer_temp_2nd,16);
					  s++;
						return TRUE;
				  }
					//˫�װ�ID����
				  if ((buffer_temp_2nd[0]==0xbb) && (buffer_temp_2nd[1]==0xbb)) //�����ʶ��ȷ0xaaaa�����Ƶ�buffer������TRUE
				  {
					  //memcpy(buffer_A,buffer_temp_2nd,16);
					  //s++;
						//����Կ�׿׵�״̬����ת���ȴ�����
						FLASH_ReadByte(0x8007500,out_A,16);
	          FLASH_ReadByte(0x8007000,out_B,16);
						out_A[2]=0xF1;
						FLASH_SetLatency(FLASH_Latency_1);       
					  FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007500);    
						FLASH_WriteByte(0x8007500,out_A,16);  
						FLASH_Lock();  	
						delay_ms(10);
						out_B[2]=0xF1;
						FLASH_SetLatency(FLASH_Latency_1);       
						FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007000);    
						FLASH_WriteByte(0x8007000,out_B,16);  
						FLASH_Lock();  	
						delay_ms(10);	
						//ΪԿ�׿�д��ID
						chipID[1]=buffer_temp_2nd[2];
						chipID[0]=buffer_temp_2nd[2]-1;
						FLASH_SetLatency(FLASH_Latency_1);       
						FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007800);    
						FLASH_WriteByte(0x8007800,chipID,2);  
						FLASH_Lock();  	
						delay_ms(10);		
						
						__set_FAULTMASK(1);
						NVIC_SystemReset();			
				  }
					//���װ�ID����
				  if ((buffer_temp_2nd[0]==0xcc) && (buffer_temp_2nd[1]==0xcc)) //�����ʶ��ȷ0xaaaa�����Ƶ�buffer������TRUE
				  {
					  //memcpy(buffer_A,buffer_temp_2nd,16);
					  //s++;
						//����Կ�׿׵�״̬����ת���ȴ�����
						FLASH_ReadByte(0x8007500,out_A,16);
	          FLASH_ReadByte(0x8007000,out_B,16);
						out_A[2]=0xF1;
						FLASH_SetLatency(FLASH_Latency_1);       
					  FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007500);    
						FLASH_WriteByte(0x8007500,out_A,16);  
						FLASH_Lock();  	
						delay_ms(10);
						out_B[2]=0xF1;
						FLASH_SetLatency(FLASH_Latency_1);       
						FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007000);    
						FLASH_WriteByte(0x8007000,out_B,16);  
						FLASH_Lock();  	
						delay_ms(10);	
						//ΪԿ�׿�д��ID
						chipID[1]=buffer_temp_2nd[2];
						chipID[0]=200;
						FLASH_SetLatency(FLASH_Latency_1);       
						FLASH_Unlock();
						FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
						FLASH_ErasePage(0x8007800);    
						FLASH_WriteByte(0x8007800,chipID,2);  
						FLASH_Lock();  
						flag_pollingA=0;						
						delay_ms(10);		
						
						__set_FAULTMASK(1);
						NVIC_SystemReset();			
				  }					
				}

			  			
			}
			else
				{

					if ((loop==9) & (s==0))
					{
//						RFID_Powerdown_A();
//						delay_ms(50);
//						Init_FM1702();
//						i[0] = chipID_A;
//						i[1] = 0x11;
//						i[2] = loop;
//						i[3] = s;																
//						Can_Send_Msg(i,8);
//						SPI1_Close();
						delay_ms(5);
//						SPI1_Open();
						
//	    				__set_FAULTMASK(1);
//			        NVIC_SystemReset();
					}
				}
			}						
			}
			}
			}
/*			if (s>0)
	    {
				return TRUE;
	    }  */
		 }
//			if ((loop==20) & (s==0))
//			{
//						RFID_Powerdown_A();
//						delay_ms(50);
//						Init_FM1702();
//						i[0] = chipID_A;
//						i[1] = 0x11;
//						i[2] = loop;
//						i[3] = s;																
//						Can_Send_Msg(i,8);
////									__set_FAULTMASK(1);
////			            NVIC_SystemReset();
//				 return FALSE;
//			}
		  if (s>0)
	    {
				return TRUE;
	    }

	 return FALSE;
}	
/****************************************************************/
/*����: Read_check_B                                            */
/*����: �ú���ʵ�������Ķ�������                                */ 
/*												       			                          */
/*����:                                                         */
/*      N/A                                                     */
/*                                                              */
/*���: TRUE or FALSE                                           */
/****************************************************************/
uchar Read_check_A(void)
{
	  unsigned char status ;
			status = Request_2nd(RF_CMD_REQUEST_ALL);		    //Ѱ��
		  if(status == FM1702_OK)   
			{
			status = AntiColl_2nd();                     //��ͻ���
			if(status == FM1702_OK)
			{
			status=Select_Card_2nd();                  //ѡ��
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY_2nd(DefaultKey);          //��������
			if(status == TRUE)
   	  {
			status = Authentication_2nd(UID_2nd, 1, RF_CMD_AUTH_LA);	  //��֤1����keyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ_2nd(buffer_temp_2nd,4);							       //��������ȡ1����0�����ݵ�buffer_temp_2nd[0]-buffer_temp_2nd[15]			
			if (status == FM1702_OK)
			 {
				if ((buffer_temp_2nd[0]==0xaa) && (buffer_temp_2nd[1]==0xaa)) //�����ʶ��ȷ0xaaaa�����Ƶ�buffer������TRUE
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
/*����: Read_status_2nd                                         */
/*����: �ú���ʵ�����ζ���״̬�仯                              */ 
/*												       			                          */
/*����:                                                         */
/*      N/A                                                     */
/*                                                              */
/*���: TRUE or FALSE                                           */
/*      TRUE �б仯��FALSE �ޱ仯                               */    
/****************************************************************/
uchar Read_status_A(void)
{
	unsigned char status;
  status =  Read_tag_A();
	if (status == TRUE && RFID_status_2nd==1)  //��Կ�����ޱ仯
	{
		return FALSE;
	}
	else if(status == FALSE && RFID_status_2nd==1) //��Կ�����б仯
	{
		memset(buffer_A,0,16*sizeof(char));          //��buffer
	
		RFID_status_2nd=0;
		return TRUE;
	}
	else if(status == TRUE && RFID_status_2nd==0) //��Կ�����б仯
	{
		RFID_status_2nd=1;
		return TRUE;
	}
	else if(status == FALSE && RFID_status_2nd==0) //��Կ�����ޱ仯
	{
		return FALSE;
	}
		else
	{
		return FALSE;
	}
}

//��⺯��
uchar calibration_A(void)
{

unsigned char calibration_array_1[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	
	unsigned char RxThreshold_value=0;
//	u32 out_data[1];
	int i,j,max=0,maxb,xxx,count;
	printf("start");

	max = calibration_array_1[1];
	maxb = 0;
		FLASH_ReadByte(0x8007500,calibration_array_1,16);
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
			 FLASH_ErasePage(0x8007500);    
       FLASH_WriteByte(0x8007500,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);
			  LED2_G_OFF;
				LED2_R_ON;
				LED2_B_OFF;

			delay_ms(1000);
			relayBuf[0]=calibration_array_1[15];
		  SendPacket(chipID_A,CALIB_ACK);	
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
			 FLASH_ErasePage(0x8007500);    
       FLASH_WriteByte(0x8007500,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);
			  LED2_G_ON;
				LED2_R_OFF;
				LED2_B_OFF;

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
			 FLASH_ErasePage(0x8007500);    
       FLASH_WriteByte(0x8007500,calibration_array_1,16);  
			 FLASH_Lock();  	
			 delay_ms(1000);				
			  LED1_G_ON;
				LED1_R_ON;
				LED1_B_OFF;
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			relayBuf[0]=0x77;
		  SendPacket(chipID_A,CALIB_ACK);	
			delay_ms(1000);
//			__set_FAULTMASK(1);
//			NVIC_SystemReset();					
			}
		}
		else
		{

		write_reg(RxThreshold,RxThreshold_value);// RxThreshold_value);
			calibration_array_1[count]=0;
//			RFID_Wakeup_A();
//			delay_ms(1);			
			for(j=0;j<255;j++)
			{
			RFID_Wakeup_A();
			delay_ms(1);					
				delay_us(100);
				xxx =  Read_check_A();
				delay_us(100);
			delay_ms(1);
			RFID_Powerdown_A();					
				if(xxx == TRUE) 
				{
					calibration_array_1[count]++;
				}
			}

			 calibration_array_1[0] = RxThreshold_value+0x10;
//			delay_ms(1);
	//		RFID_Powerdown_A();					
				
			 FLASH_SetLatency(FLASH_Latency_1);       
       FLASH_Unlock();
			 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
			 FLASH_ErasePage(0x8007500);    
			 FLASH_WriteByte(0x8007500,calibration_array_1,16);
			 FLASH_Lock();  					
					
				delay_ms(1000);			
			__set_FAULTMASK(1);
			NVIC_SystemReset();

		 }
		return TRUE;
}



