//============================================================================
// Name        : ZS-LRM100.cpp
// Author      : mh.najafi
// Version     :
// Copyright   : ZarrinSamaneh
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "main.h"

#include "gpio.h"
#include "ECXX.h"





using namespace std;

ECXX 	modem;
GPIO	gpio;


int main() {

	//prints for notify the application Start
	AppStartPrint();

	//Config Gpio pins
	gpio.Init();

	//Start  and initialize Quectel module
	ECXX_Start(&modem);


	//if( modem.ModuleType==EC200U ) EC200U_Manager(&modem);

	//Get Module Status Continuous
	ECXX_Get_Status(&modem);

	//Empty While
	while(1)
	{
		sleep(10);

	}

	return 0;
}
