#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

#define LEDR2 PBout(4)
#define LEDG2 PBout(3)
#define LEDB2 PAout(15)

#define LEDR1 PBout(5)
#define LEDG1 PBout(6)
#define LEDB1 PBout(7)

extern u8 PWM_WideCon1[3];  
extern u8 PWM_WideCnt1;
extern u8 PWM_WideCon2[3];  
extern u8 PWM_WideCnt2;

void LED_Init(void);//≥ı ºªØ
void pwm1(void);
void pwm2(void);   
		 				    
#endif
