/*
 * gpio.h
 *
 *  Created on: Nov 21, 2021
 *      Author: mhn
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "tools.h"

#define		GPIO_REFRESH_TIME	3



#define		GPIO_WRITE1_LED_SIG			"echo 1 > /sys/devices/platform/leds/leds/LED:Signal/brightness"
#define		GPIO_WRITE1_LED_NET 		"echo 1 > /sys/devices/platform/leds/leds/LED:Net/brightness"
#define		GPIO_WRITE1_OUT_GSMRST		"echo 1 > /sys/devices/platform/leds/leds/output:GSMrst/brightness"
#define		GPIO_WRITE1_OUT_GSMPWR		"echo 1 > /sys/devices/platform/leds/leds/output:GSMpwr/brightness"



#define		GPIO_WRITE0_LED_SIG			"echo 0 > /sys/devices/platform/leds/leds/LED:Signal/brightness"
#define		GPIO_WRITE0_LED_NET 		"echo 0 > /sys/devices/platform/leds/leds/LED:Net/brightness"
#define		GPIO_WRITE0_OUT_GSMRST		"echo 0 > /sys/devices/platform/leds/leds/output:GSMrst/brightness"
#define		GPIO_WRITE0_OUT_GSMPWR		"echo 0 > /sys/devices/platform/leds/leds/output:GSMpwr/brightness"



#define		LED_NET_TIMEREN				"echo timer > /sys/devices/platform/leds/leds/LED:Net/trigger"
#define		LED_NET_ONDLY1				"echo 100 > /sys/devices/platform/leds/leds/LED:Net/delay_on"
#define		LED_NET_ONDLY2				"echo 500 > /sys/devices/platform/leds/leds/LED:Net/delay_on"
#define		LED_NET_ONDLY3				"echo 1000 > /sys/devices/platform/leds/leds/LED:Net/delay_on"
#define		LED_NET_ONDLY4				"echo 2000 > /sys/devices/platform/leds/leds/LED:Net/delay_on"
#define		LED_NET_OFFDLY1				"echo 100 > /sys/devices/platform/leds/leds/LED:Net/delay_off"
#define		LED_NET_OFFDLY2				"echo 500 > /sys/devices/platform/leds/leds/LED:Net/delay_off"
#define		LED_NET_OFFDLY3				"echo 1000 > /sys/devices/platform/leds/leds/LED:Net/delay_off"
#define		LED_NET_OFFDLY4				"echo 2000 > /sys/devices/platform/leds/leds/LED:Net/delay_off"

#define		LED_SIG_PTRNEN		 		"echo pattern > /sys/devices/platform/leds/leds/LED:Signal/trigger"
#define		LED_SIG_PATT0				"echo 0 10 0 100 > /sys/devices/platform/leds/leds/LED:Signal/pattern"
#define		LED_SIG_PATT1				"echo 1 10 0 20000 > /sys/devices/platform/leds/leds/LED:Signal/pattern"
#define		LED_SIG_PATT2				"echo 1 10 0 200 1 10 0 20000 > /sys/devices/platform/leds/leds/LED:Signal/pattern"
#define		LED_SIG_PATT3				"echo 1 10 0 200 1 10 0 200 1 10 0 20000 > /sys/devices/platform/leds/leds/LED:Signal/pattern"
#define		LED_SIG_PATT4				"echo 1 10 0 200 1 10 0 200 1 10 0 200 1 10 0 20000 > /sys/devices/platform/leds/leds/LED:Signal/pattern"




void *GPIO_Referesh(void *arg);


class GPIO{       // The class
	public:

	uint8_t*  	BASE_ADDR=NULL ;

	result Start()
	{
		pthread_t		thread1;
		this->Init();

		//pthread_create( &thread1 , NULL , GPIO_Referesh ,NULL);

		return OK;
	}


	result Init()
	{
		//Change signal LED work mode to pattern Mode
		exec2(LED_SIG_PTRNEN);

		//Change NET LED work mode to Timer Mode
		exec2(LED_NET_TIMEREN);

		return OK;
	}

	//Write a Value to gpio pin
	void Write(uint8_t GPIO,bool value)
	{

		if(value)
		{
			switch(GPIO)
			{

				case GPIO_OUT_GSMRST:
					exec2(GPIO_WRITE1_OUT_GSMRST);
					break;

				case GPIO_OUT_GSMPWR:
					exec2(GPIO_WRITE1_OUT_GSMPWR);
					break;


				default:
					break;
			}
		}
		else
		{
			switch(GPIO)
			{

				case GPIO_LED_NET:
					exec2(GPIO_WRITE0_LED_SIG);
					break;


				case GPIO_OUT_GSMRST:
					exec2(GPIO_WRITE0_OUT_GSMRST);
					break;

				case GPIO_OUT_GSMPWR:
					exec2(GPIO_WRITE0_OUT_GSMPWR);
					break;


				default:
					break;
			}
		}

	}



};





#endif /* GPIO_H_ */
