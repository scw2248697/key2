#ifndef __LOCK_H
#define __LOCK_H	 
#include "sys.h"
	   

void LOCK_Init(void);	//≥ı ºªØ

#define LOCK1P PBout(8)
#define LOCK1N PBout(9)
#define LOCK2P PBout(10)
#define LOCK2N PBout(11)

#endif

