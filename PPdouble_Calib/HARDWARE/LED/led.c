
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
 
   

#define ON  0x01
#define OFF 0x00
  
u8 PWM_WideCon1[3]={0xf,0xf,0xf};  
u8 PWM_WideCnt1;
u8 PWM_WideCon2[3]={0xf,0xf,0xf};  
u8 PWM_WideCnt2;
  


void LED_Init(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
 

	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;				
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 
 GPIO_ResetBits(GPIOB,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4;				
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 
 GPIO_ResetBits(GPIOB,GPIO_Pin_3|GPIO_Pin_4);
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;				
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		
 GPIO_Init(GPIOA, &GPIO_InitStructure);					 
 GPIO_ResetBits(GPIOA,GPIO_Pin_15);					
}


 void pwm1(void)    
{
        PWM_WideCnt1++;
	      if( PWM_WideCnt1==16)
				{
				  PWM_WideCnt1=0; 
				}
        if(PWM_WideCnt1>=(PWM_WideCon1[0]&0x0F))
				{
				   LEDR1 = OFF;  
				}
				else
				{
				   LEDR1 = ON;
				}
         
				if(PWM_WideCnt1>=(PWM_WideCon1[1]&0x0F))
				{
				   LEDG1 = OFF; 
				}
				else
				{
				   LEDG1 = ON;
				}
        if(PWM_WideCnt1>=(PWM_WideCon1[2]&0x0F))
				{
				   LEDB1 = OFF;  
				}
				else
				{
				   LEDB1 = ON;	
				}
				
}
 void pwm2(void)    
{
        PWM_WideCnt2++; 
	      if( PWM_WideCnt2==16)
				{
				  PWM_WideCnt2=0; 
				}	
        if(PWM_WideCnt2>=(PWM_WideCon2[0]&0x0F))
				{
				   LEDR2 = OFF;  
				}
				else
				{
				   LEDR2 = ON;
				}
         
				if(PWM_WideCnt2>=(PWM_WideCon2[1]&0x0F))
				{
				   LEDG2 = OFF; 
				}
				else
				{
				   LEDG2 = ON;
				}
        if(PWM_WideCnt2>=(PWM_WideCon2[2]&0x0F))
				{
				   LEDB2 = OFF;  
				}
				else
				{
				   LEDB2 = ON;	
				}
}
