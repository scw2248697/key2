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
uchar     	tagtype_2nd[2];	        /* 卡片标识字符 */
unsigned char buffer_A[16]; 
unsigned char buffer_temp_2nd[16];
/* FM1702变量定义 */
volatile unsigned char     	buf[10];            /* FM1702命令发送接收缓冲区 */
 uchar     	UID_2nd[5];             /* 序列号 */
//uchar       Secnr;			        /* 扇区号 */
uchar RFID_status_2nd ;  /*锁孔状态标识, 1--有钥匙 0--无钥匙*/
//void delay(unsigned int dlength)
//{ 
//	unsigned int  i;
//	unsigned char j;
//	for (i=0;i<dlength;i++)
//	{
//		for (j=0;j<100;j++);
//	}
//} 

//SPI接收
 unsigned char rev(uchar tem)
{
  return SPI1_ReadWriteByte(tem);
} 

//SPI发送
 void Send(unsigned char var) 

{ 	
	SPI1_ReadWriteByte(var);
}                  

//读寄存器
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

//写寄存器
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
/*名称: Clear_FIFO                                              */
/*功能: 该函数实现清縁FIFO的数据                                */
/*												       			                          */
/*输入:                                                         */
/*      N/A                                                     */
/*                                                              */
/*输出:                                                         */
/*	    TRUE, FIFO被清空                                        */
/*	    FALSE, FIFO未被清空  	                                  */
/****************************************************************/
 uchar Clear_FIFO(void)
{
	uchar temp;
	uint  i;
	
	temp =read_reg(Control);						//清空FIFO
	temp = (temp | 0x01);
	write_reg(Control, temp);
	for(i = 0; i < RF_TimeOut; i++)			//检查FIFO是否被清空
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
/*名称: Write_FIFO                                              */
/*功能: 该函数实现向RC531的FIFO中写入x bytes数据                */
/*												       			                          */
/*输入:                                                         */
/*      count, 待写入字节的长度                                 */
/*	    buff, 指向待写入数据的指针                              */
/*                                                              */
/*输出:                                                         */
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
/*名称: Read_FIFO                                               */
/*功能: 该函数实现从RC531的FIFO中读出x bytes数据                */
/*												        		                          */
/*输入:                                                         */
/*       buff, 指向读出数据的指针                               */
/*                                                              */
/*输出:                                                         */
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
	if (temp >= 24)						//temp=255时,会进入死循环
	{									        //因此增加FIFO_Length越限判断
		temp = 24;						 
	}
	for(i = 0;i < temp; i++)
	{
 		*(buff + i) =read_reg(FIFO);
		}
	return temp;
 }

/****************************************************************/
/*名称: Judge_Req                                               */
/*功能: 该函数实现对卡片复位应答信号的判断                      */
/*												       			                          */
/*输入:                                                         */
/*       *buff, 指向应答数据的指针                              */
/*                                                              */
/*输出:                                                         */
/*	     TRUE, 卡片应答信号正确                                 */
/*       FALSE, 卡片应答信号错误                                */
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
/*名称: Check_UID                                               */
/*功能: 该函数实现对收到的卡片的序列号的判断                    */
/*输入: N/A                                                     */
/*输出: TRUE: 序列号正确                                        */
/* FALSE: 序列号错误                                            */
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
/*名称: Save_UID                                                */
/*功能: 该函数实现保存卡片收到的序列号                          */
/*输入: row: 产生冲突的行                                       */
/* col: 产生冲突的列                                            */
/* length: 接収到的UID数据长度                                  */
/*输出: N/A                                                     */
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
/*名称: Set_BitFraming                                          */
/*功能: 该函数设置待发送数据的字节数                            */
/*输入: row: 产生冲突的行                                       */
/*      col: 产生冲突的列                                       */
/*输出: N/A                                                     */
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
/*名称: FM1702_Bus_Sel                                          */
/*功能: 该函数实现对FM1702操作的总线方式(并行总线,SPI)选择      */ 
/*												       			                          */
/*输入:                                                         */
/*      N/A                                                     */
/*                                                              */
/*输出:                                                         */
/*	    TRUE,  总线选择成功                                     */
/*	    FALSE, 总线选择失败  	                                  */
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
/*名称: Init_FM1702_2nd                                          */
/*功能: 该函数实现对FM1702初始化                                */ 
/*												       			                          */
/*输入:                                                         */
/*      N/A                                                     */
/*                                                              */
/*输出:                                                         */
/*	    TRUE,  总线选择成功                                     */
/*	    FALSE, 总线选择失败  	                                  */
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
	RST_H_2nd;				        //复位
	
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
/*新的初始化*/	
	temp = read_reg(0x05);
//	while(temp!=0x60) temp=read_reg(0x05);
	delay_ms(50);
	temp = FM1702_Bus_Sel();		 	//总线选择
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
//		temp = FM1702_Bus_Sel();		 	//总线选择
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
/*名称: Command_Send                                            */
/*功能: 该函数实现向RC531发送命令集的功能                       */
/*												        		                          */
/*输入:                                                         */
/*       count, 待发送命令集的长度                              */
/*	     buff, 指向待发送数据的指针                             */
/*       Comm_Set, 命令码                                       */
/*												       			                          */
/*输出:                                                         */
/*	     TRUE, 命令被正确执行                                   */
/*	     FALSE, 命令执行错误  	                                */
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
	write_reg(Command, Comm_Set);					//命令执行
	
	for(j = 0; j< RF_TimeOut; j++)				//检查命令执行否
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
/*名称: MIF_Halt                                                */
/*功能: 该函数实现暂停MIFARE卡                                  */
/*输入: N/A                                                     */
/*输出: FM1702_OK: 应答正确                                     */
/* FM1702_PARITYERR: 奇偶校验错                                 */
/* FM1702_CRCERR: CRC校验错                                     */
/* FM1702_NOTAGERR: 无卡                                        */
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
// 转换密钥格式
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
/*名称: Load_keyE2                                              */
/*功能: 该函数实现把E2中密码存入FM1702的keybuf中             */
/*输入: Secnr: EE起始地址                                       */
/*输出: True: 密钥装载成功                                      */
/* False: 密钥装载失败                                          */
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
/*名称: Request                                                 */
/*功能: 该函数实现对放入RC531操作范围之内的卡片的Request操作    */ 
/*												       			                          */
/*输入:                                                         */
/*      mode: ALL(监测所以RC531操作范围之内的卡片)			   	    */
/*	    STD(监测在RC531操作范围之内处于HALT状态的卡片)          */
/*                                                              */
/*输出:                                                         */
/*	    FM222_NOTAGERR: 无卡                                    */
/*      FM222_OK: 应答正确                                      */
/*	    FM222_REQERR: 应答错误										              */
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
    buf[0] = mode;					             //Request模式选择
    write_reg(Bit_Frame,0x07);			       //发送7bit
    write_reg(ChannelRedundancy,0x03);	   //关闭CRC
    write_reg(TxControl,0x5b); 
    write_reg(Control,0x01);          		 //屏蔽CRYPTO1位
    temp = Command_Send(1, buf, Transceive);
    if(temp == FALSE)
    {
	    return FM1702_NOTAGERR;
    }	
	
    Read_FIFO(buf);					           //从FIFO中读取应答信息
    temp = Judge_Req(buf);			         //判断应答信号是否正确
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
/*名称: AntiColl                                                */
/*功能: 该函数实现对放入FM1702操作范围之内的卡片的防冲突检测    */
/*输入: N/A                                                     */
/*输出: FM1702_NOTAGERR: 无卡                                   */
/* FM1702_BYTECOUNTERR: 接收字节错误                            */
/* FM1702_SERNRERR: 卡片序列号应答错误                          */
/* FM1702_OK: 卡片应答正确                                      */
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
	write_reg(0x22,0x03);	                   // 关闭CRC,打开奇偶校验
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
		Save_UID(row, col, temp);			        // 将收到的UID放入UID数组中
	
		temp = read_reg(0x0A);				        // 判断接収数据是否出错
		temp = temp & 0x01;
		if(temp == 0x00)
		{
			temp = Check_UID();			            // 校验收到的UID
			if(temp == FALSE)
			{
				return(FM1702_SERNRERR);
			}
      PR("2nd_RFID_AntiColl OK!\r\n");
			return(FM1702_OK);
		}
		else
		{
			temp = read_reg(0x0B);             // 读取冲突检测寄存器 
			row = temp / 8;
			col = temp % 8;
			buf[0] = RF_CMD_ANTICOL;
			Set_BitFraming(row + pre_row, col);	// 设置待发送数据的字节数 
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
/*名称: Select_Card                                             */
/*功能: 该函数实现对放入FM1702操作范围之内的某张卡片进行选择    */
/*输入: N/A                                                     */
/*输出: FM1702_NOTAGERR: 无卡                                   */
/* FM1702_PARITYERR: 奇偶校验错                                 */
/* FM1702_CRCERR: CRC校验错                                     */
/* FM1702_BYTECOUNTERR: 接收字节错误                            */
/* FM1702_OK: 应答正确                                          */
/* FM1702_SELERR: 选卡出错                                      */
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

	write_reg(0x22,0x0f);	                       // 开启CRC,奇偶校验校验 
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
		Read_FIFO(buf);	                      // 从FIFO中读取应答信息 
		temp = *buf;
		//判断应答信号是否正确 
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
/*名称: Authentication                                          */
/*功能: 该函数实现密码认证的过程                                */
/*输入: UID: 卡片序列号地址                                     */
/* SecNR: 扇区号                                                */
/* mode: 模式                                                   */
/*输出: FM1702_NOTAGERR: 无卡                                   */
/* FM1702_PARITYERR: 奇偶校验错                                 */
/* FM1702_CRCERR: CRC校验错                                     */
/* FM1702_OK: 应答正确                                          */
/* FM1702_AUTHERR: 权威认证有错                                 */
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

	write_reg(0x22,0x0f);	                     // 开启CRC,奇偶校验校验 
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
	temp1 = temp1 & 0x08;	                     // Crypto1on=1验证通过 
	if(temp1 == 0x08)
	{
		PR("2nd_RFID_Authentication OK!\r\n");
		return FM1702_OK;
	}
  PR("2nd_RFID_Authentication FAIL--3!\r\n");
	return FM1702_AUTHERR;
}

/****************************************************************/
/*名称: MIF_Read                                                */
/*功能: 该函数实现读MIFARE卡块的数值                            */
/*输入: buff: 缓冲区首地址                                      */
/* Block_Adr: 块地址                                            */
/*输出: FM1702_NOTAGERR: 无卡                                   */
/* FM1702_PARITYERR: 奇偶校验错                                 */
/* FM1702_CRCERR: CRC校验错                                     */
/* FM1702_BYTECOUNTERR: 接收字节错误                            */
/* FM1702_OK: 应答正确                                          */
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
	if(temp == 0x10)	                      // 8K卡读数据长度为16
	{
		Read_FIFO(buff);
		PR("2nd_RFID_MIF_READ OK!\r\n");
		return FM1702_OK;
	}
	else if(temp == 0x04)	                  // Token卡读数据长度为16
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
/*名称: MIF_Write                                               */
/*功能: 该函数实现写MIFARE卡块的数值                            */
/*输入: buff: 缓冲区首地址                                      */
/* Block_Adr: 块地址                                            */
/*输出: FM1702_NOTAGERR: 无卡                                   */
/* FM1702_BYTECOUNTERR: 接收字节错误                            */
/* FM1702_NOTAUTHERR: 未经权威认证                              */
/* FM1702_EMPTY: 数据溢出错误                                   */
/* FM1702_CRCERR: CRC校验错                                     */
/* FM1702_PARITYERR: 奇偶校验错                                 */
/* FM1702_WRITEERR: 写卡块数据出错                              */
/* FM1702_OK: 应答正确                                          */
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
		case 0x00:	return(FM1702_NOTAUTHERR);	     // 暂时屏蔽掉写错误
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
/*名称: ReadID_FM1702                                            */
/*功能: 该函数实现对FM1702 ID读取  */
/*输入: N/A                                                      */
/*输出: TRUE 读取成功并打印                                      */
/*      FALSE读取失败返回FALSE                                   */
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
	Read_FIFO(buf);	                      // 从FIFO中读取应答信息 
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
/*名称: Standby_2nd                                            */
/*功能: 该函数实现对FM1702 standby                               */
/*输入: N/A                                                      */
/*输出: TRUE 读取成功并打印                                      */
/*      FALSE读取失败返回FALSE                                   */
/*****************************************************************/
void RFID_Powerdown_A(void)
{
//	/*关电源*/
//	uchar	temp;
//	temp = read_reg(Control);
//	temp = temp | 0x10;
//	write_reg(Control,temp);
//	delay_us(100);
	/*关天线*/
	uchar	temp;
	temp = read_reg(TxControl);
	temp = temp & 0xFC;
	write_reg(TxControl,temp);
	delay_us(100);	
	

	
}
void RFID_Off_A(void)
{
	/*关电源*/
	uchar	temp;
	temp = read_reg(Control);
	temp = temp | 0x10;
	write_reg(Control,temp);
	delay_us(100);
	/*关天线*/
//	uchar	temp;
//	temp = read_reg(TxControl);
//	temp = temp & 0xFC;
//	write_reg(TxControl,temp);
//	delay_us(100);	
	

	
}
/****************************************************************/
/*名称: Wakeup_2nd                                            */
/*功能: 该函数实现对FM1702 Wakeup                               */
/*输入: N/A                                                      */
/*输出: TRUE 读取成功并打印                                      */
/*      FALSE读取失败返回FALSE                                   */
/*****************************************************************/
void RFID_Wakeup_A(void)
{
	/*开电源*/
//	uchar	temp;
//	temp = read_reg(Control);
//	temp = temp & 0xef;
//	write_reg(Control,temp);
//	delay_us(100);
	/*开天线*/
		uchar	temp;
	temp = read_reg(TxControl);
	temp = temp | 0x03;
	write_reg(TxControl,temp);
	delay_us(100);	
}
void RFID_On_A(void)
{
	/*开电源*/
	uchar	temp;
	temp = read_reg(Control);
	temp = temp & 0xef;
	write_reg(Control,temp);
	delay_us(100);
//	/*开天线*/
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
			status = Request_2nd(RF_CMD_REQUEST_ALL);		    //寻卡
		  if(status == FM1702_OK)   
			{
			status = AntiColl_2nd();                     //冲突检测
			if(status == FM1702_OK)
			{
			status=Select_Card_2nd();                  //选卡
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY_2nd(DefaultKey);          //加载密码
			if(status == TRUE)
   	  {
			status = Authentication_2nd(UID_2nd, 1, RF_CMD_AUTH_LA);	  //验证1扇区keyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ_2nd(buffer_temp_2nd,4);							       //读卡，读取1扇区0块数据到buffer_temp_2nd[0]-buffer_temp_2nd[15]
				if (status == FM1702_OK)
				{
				  if ((buffer_temp_2nd[0]==0xaa) && (buffer_temp_2nd[1]==0xaa)) //如果标识正确0xaaaa，则复制到buffer，返回TRUE
				  {
					  memcpy(buffer_A,buffer_temp_2nd,16);
					  s++;
						return TRUE;
				  }
					//双孔板ID设置
				  if ((buffer_temp_2nd[0]==0xbb) && (buffer_temp_2nd[1]==0xbb)) //如果标识正确0xaaaa，则复制到buffer，返回TRUE
				  {
					  //memcpy(buffer_A,buffer_temp_2nd,16);
					  //s++;
						//现有钥匙孔的状态，跳转至等待检验
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
						//为钥匙孔写入ID
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
					//单孔板ID设置
				  if ((buffer_temp_2nd[0]==0xcc) && (buffer_temp_2nd[1]==0xcc)) //如果标识正确0xaaaa，则复制到buffer，返回TRUE
				  {
					  //memcpy(buffer_A,buffer_temp_2nd,16);
					  //s++;
						//现有钥匙孔的状态，跳转至等待检验
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
						//为钥匙孔写入ID
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
/*名称: Read_check_B                                            */
/*功能: 该函数实现完整的读卡操作                                */ 
/*												       			                          */
/*输入:                                                         */
/*      N/A                                                     */
/*                                                              */
/*输出: TRUE or FALSE                                           */
/****************************************************************/
uchar Read_check_A(void)
{
	  unsigned char status ;
			status = Request_2nd(RF_CMD_REQUEST_ALL);		    //寻卡
		  if(status == FM1702_OK)   
			{
			status = AntiColl_2nd();                     //冲突检测
			if(status == FM1702_OK)
			{
			status=Select_Card_2nd();                  //选卡
			if(status == FM1702_OK)
			{
			status = Load_keyE2_CPY_2nd(DefaultKey);          //加载密码
			if(status == TRUE)
   	  {
			status = Authentication_2nd(UID_2nd, 1, RF_CMD_AUTH_LA);	  //验证1扇区keyA
			if(status == FM1702_OK)
		  {
		  status=MIF_READ_2nd(buffer_temp_2nd,4);							       //读卡，读取1扇区0块数据到buffer_temp_2nd[0]-buffer_temp_2nd[15]			
			if (status == FM1702_OK)
			 {
				if ((buffer_temp_2nd[0]==0xaa) && (buffer_temp_2nd[1]==0xaa)) //如果标识正确0xaaaa，则复制到buffer，返回TRUE
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
/*名称: Read_status_2nd                                         */
/*功能: 该函数实现两次读卡状态变化                              */ 
/*												       			                          */
/*输入:                                                         */
/*      N/A                                                     */
/*                                                              */
/*输出: TRUE or FALSE                                           */
/*      TRUE 有变化，FALSE 无变化                               */    
/****************************************************************/
uchar Read_status_A(void)
{
	unsigned char status;
  status =  Read_tag_A();
	if (status == TRUE && RFID_status_2nd==1)  //有钥匙且无变化
	{
		return FALSE;
	}
	else if(status == FALSE && RFID_status_2nd==1) //无钥匙且有变化
	{
		memset(buffer_A,0,16*sizeof(char));          //清buffer
	
		RFID_status_2nd=0;
		return TRUE;
	}
	else if(status == TRUE && RFID_status_2nd==0) //有钥匙且有变化
	{
		RFID_status_2nd=1;
		return TRUE;
	}
	else if(status == FALSE && RFID_status_2nd==0) //无钥匙且无变化
	{
		return FALSE;
	}
		else
	{
		return FALSE;
	}
}

//检测函数
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



