/*
 * EC25.c
 *
 *  Created on: Aug 6, 2021
 *      Author: mhn
 */

#include "main.h"
#include "gpio.h"
#include "ECXX.h"
#include "stdlib.h"
#include "string.h"


extern 	ECXX 	modem;


//extern EC25 modem;
using namespace std;

GPRS gprs;
DataCenter  datacenter;
LAN lan;
USER user;
IO io;
FTP  ftp;
ADMIN admin;
HELP help;




void *EC25_Manager_thread(void *unused)
{
	char str1[50],str2[100],str3[50];

	memset(str1,0,sizeof(str1));
	exec(UCI_GET_APN1,str1,100);

	str1[strlen(str1)-1]=0;

	memset(str2,0,sizeof(str2));

	report("s","EC25 --> Manager_Thread Started !");

	sprintf(str2,"/usr/bin/app1 -s %s -f log.txt",str1,str3);

	while(1)
	{
		std::system(str2);
		sleep(5);
	}
}




void ECXX_Send_AT(ECXX *module,char *Request,char *Response,uint16_t timeout)
{
	char temp_buffer[500];
	uint16_t count=0,ret=0;
	uint16_t time=0;
	string str;
	int find=0;

	memset(Response,0,ECXX_BUFFER_SIZE);
	memset(temp_buffer,0,sizeof(temp_buffer));

	//Empty all the data in serial file  descriptor
	tcflush(module->PORT.Port_fd, TCIOFLUSH);

	//Write the command into serial File descriptor
	write(module->PORT.Port_fd,Request,strlen(Request));
	msleep(50);

	//Wait until timeout
	while(time<timeout)
	{
		//Read data from serial File descriptor
		ret=read(module->PORT.Port_fd, temp_buffer,500);
		if(ret) memcpy(Response+count,temp_buffer,ret);
		count+=ret;
		str=Response;
		//Find the end of Module response "OK"
		find= str.find("\r\nOK",2);
		if(find >1 ) break;
		msleep(1);
		time++;
	}

}


void ECXX_Set_Parameters(ECXX *module)
{
	char str1[20];

	module->status.Connected=NOTDEFINE;
	module->status.Ready=NOTDEFINE;
	module->status.SimReady=NOTDEFINE;
	module->status.Registration=NOTDEFINE;
	module->status.Roaming=NOTDEFINE;
	module->status.NoNetCount=0;
	module->status.NoSigCount=0;
	module->status.NoSimCount=0;
	module->SimCard=0;

	memset(str1,0,sizeof(str1));

	//Read Default simcard using uci
	exec(UCI_GET_DEFSIM,str1,10);
	if(str1[0]=='1') module->SimCard=1;
	else module->SimCard=0;

	//Read Auto Sim change timings using uci
	memset(str1,0,20);
	exec(UCI_GET_SIMCHANGE, str1, 10);
	uint16_t tmp=stoi(str1)/3;
	if(tmp>5)
	{
		module->NoNetTime=tmp;
		module->NoSigTime=tmp;
	}
	else
	{
		module->NoNetTime=NO_NET_TIMEOUT;
		module->NoSigTime=NO_SIG_TIMEOUT;
	}
	module->NoSimTime=NO_SIM_TIMEOUT;


	//Read all of modem parameters and save to buffer
	memset(module->settingfile,0,sizeof(module->settingfile));
	exec("uci show modem.parameter", module->settingfile, 10);

}


void ECXX_Init(ECXX *module,char reset)
{
	char str1[ECXX_BUFFER_SIZE],str2[100];


	//========================( Active Dual simcard )=========================
	if(module->ModuleType==EC200U)
	{
		ECXX_Send_AT(module, AT_ACTIVE_DUALSIM, str1, 1000);
	}


	//restart module if needed
	if(reset==1) ECXX_Reset(module);


	report("s","ECXX --> Initializing... ");


	if(module->ModuleType==EC200U)
	{
		//========================( Disable Data call )=========================
		ECXX_Send_AT(module, AT_SETUP_DATACALL0, str1, 1000);

		//========================( Deactivate PDP Context )====================
		ECXX_Send_AT(module, AT_DEACTIVE_PDP, str1, 3000);

		//========================( Select active USIM )========================
		if(module->SimCard==0)
		{
			ECXX_Send_AT(module, AT_SELECT_SIM0, str1, 7000);
			exec2(UCI_SET_CURRNETSIM1);
		}
		else
		{
			ECXX_Send_AT(module, AT_SELECT_SIM1, str1, 7000);
			exec2(UCI_SET_CURRNETSIM2);
		}
		exec2(UCI_COMMIT_MODEM);

		//========================( Enable USIM Present )=======================
		ECXX_Send_AT(module, AT_PIN_PRESENT, str1, 1000);


		//========================( Set APN )===================================
		memset(str1,0,sizeof(str1));
		if(module->SimCard==0) exec(UCI_GET_APN1,str1,100);
		else exec(UCI_GET_APN2,str1,100);
		str1[strlen(str1)-1]=0;
		memset(str2,0,sizeof(str2));
		sprintf(str2,"%s\"%s\"\r\n\0",AT_SET_APN,str1);

		ECXX_Send_AT(module, str2, str1, 1000);

		//==========================( Set LTE Band Lock )=======================
		{
			char str2[100];
			char str3[100];
			uint16_t val0=0;
			uint16_t val1=0;
			uint16_t val2=0;
			uint16_t val3=0;	memset(module->settingfile,0,sizeof(module->settingfile));
			exec("uci show modem.parameter", module->settingfile, 10);

			memset(str1,0,sizeof(str1));
			memset(str2,0,sizeof(str2));

			exec(UCI_GET_UMTSLOCK_ALL,str1,10);
			if(str1[0]=='1') val0=0;
			else
			{
				const uint8_t GSMbandNUM[4]={2,3,5,8};
				const uint_fast32_t GSMbandVAL[4]={0x8,0x2,0x4,0x1};
				const uint8_t UMTSbandNUM[6]={1,2,5,6,8,19};
				const uint_fast32_t UMTSbandVAL[6]={0x10,0x20,0x40,0x100,0x80,0x100};

				for(uint8_t i=0;i<4;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_GSMLOCK,GSMbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val0|=GSMbandVAL[i];
				}

			}

			exec(UCI_GET_LTELOCK_ALL,str1,100);
			if(str1[0]=='1')
			{
				val1=0;
				val2=0;
				val3=0;
			}
			else
			{
				const uint8_t LTEbandNUM[15]={1,2,3,4,5,7,8,19,20,26,28,32,38,40,41};

				for(uint8_t i=0;i<7;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val1|=(1 << (LTEbandNUM[i] - 1) );
				}

				for(uint8_t i=7;i<11;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val2|=(1 << (LTEbandNUM[i] - 17) );
				}

				for(uint8_t i=11;i<15;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val3|=(1 << (LTEbandNUM[i] - 33));
				}
			}

			memset(str1,0,sizeof(str1));
			if(val0==0) sprintf(str1,"ffff");
			else sprintf(str1,"%04x",val0);

			memset(str2,0,sizeof(str2));
			if(val1==0 && val2==0 && val3==0) sprintf(str2,"1a0080800d5");
			else	sprintf(str2,"%x%04x%04x",val3,val2,val1);

			memset(str3,0,sizeof(str3));

			sprintf(str3,"%s,%s,%s\r\n",AT_BAND_LOCK,str1,str2);


			ECXX_Send_AT(module, str3, str1, 1000);

		}


		//==========================( Set Technology Lock )=======================
		exec(UCI_GET_TECHLOCK,str1,100);
		if(str1[0]=='0')
			ECXX_Send_AT(module, AT_TECH_LOCK_ALL, str1, 1000);
		else if(str1[0]=='1')
			ECXX_Send_AT(module, AT_TECH_LOCK_GSM, str1, 1000);
		else if(str1[0]=='4')
			ECXX_Send_AT(module, AT_TECH_LOCK_LTE, str1, 1000);



		//========================( Activate PDP Context )=======================
		//memset(str1,0,sizeof(str1));
		//write(module->PORT.Port_fd,AT_ACTIVE_PDP,sizeof(AT_ACTIVE_PDP));
		//sleep(2);
		//read(module->PORT.Port_fd, str1,100);


		//========================( Setup Data Call )============================
		ECXX_Send_AT(module, AT_SETUP_DATACALL3, str1, 1000);

		//========================( Set SMS to Text Mode )=======================
		ECXX_Send_AT(module, AT_SET_SMS_TXT, str1, 1000);

		//========================( Delete All  SMS )============================
		ECXX_Send_AT(module, AT_DEL_SMS_ALL, str1, 1000);

		//Restart modem interface
		exec2(USBNET0_up);


	}
	else
	{

		//========================( Enable USIM Present )=======================
		ECXX_Send_AT(module, AT_PIN_PRESENT, str1, 1000);


		//==========================( Set LTE Band Lock )=======================
		{
			char str2[100];
			char str3[100];
			uint16_t val0=0;
			uint16_t val1=0;
			uint16_t val2=0;
			uint16_t val3=0;

			memset(str1,0,sizeof(str1));
			memset(str2,0,sizeof(str2));

			exec(UCI_GET_UMTSLOCK_ALL,str1,10);
			if(str1[0]=='1') val0=0;
			else
			{
				const uint8_t GSMbandNUM[4]={2,3,5,8};
				const uint_fast32_t GSMbandVAL[4]={0x8,0x2,0x4,0x1};
				const uint8_t UMTSbandNUM[6]={1,2,5,6,8,19};
				const uint_fast32_t UMTSbandVAL[6]={0x10,0x20,0x40,0x100,0x80,0x100};

				for(uint8_t i=0;i<4;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_GSMLOCK,GSMbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val0|=GSMbandVAL[i];
				}

				for(uint8_t i=0;i<6;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_UMTSLOCK,UMTSbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val0|=UMTSbandVAL[i];
				}

			}

			exec(UCI_GET_LTELOCK_ALL,str1,100);
			if(str1[0]=='1')
			{
				val1=0;
				val2=0;
				val3=0;
			}
			else
			{
				const uint8_t LTEbandNUM[15]={1,2,3,4,5,7,8,19,20,26,28,32,38,40,41};

				for(uint8_t i=0;i<7;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val1|=(1 << (LTEbandNUM[i] - 1) );
				}

				for(uint8_t i=7;i<11;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val2|=(1 << (LTEbandNUM[i] - 17) );
				}

				for(uint8_t i=11;i<15;i++)
				{
					sprintf(str2,"%s%d",UCI_GET_LTELOCK,LTEbandNUM[i]);
					exec(str2,str1,10);
					if(str1[0]=='1') val3|=(1 << (LTEbandNUM[i] - 33));
				}
			}

			memset(str1,0,sizeof(str1));
			if(val0==0) sprintf(str1,"FFFF");
			else sprintf(str1,"%04x",val0);

			memset(str2,0,sizeof(str2));
			if(val1==0 && val2==0 && val3==0) sprintf(str2,"1A08A0C00DF");
			else	sprintf(str2,"%x%04x%04x",val3,val2,val1);

			memset(str3,0,sizeof(str3));

			sprintf(str3,"%s,%s,%s,0\r\n\0",AT_BAND_LOCK,str1,str2);


			ECXX_Send_AT(module, str3, str1, 1000);

		}
		//==========================( Set Roaming Lock )=======================
		memset(str1,0,sizeof(str1));
		exec(UCI_GET_ROAMLOCK,str1,100);
		if(str1[0]=='1')
			ECXX_Send_AT(module, AT_ROAM_DISABLE, str1, 1000);
		else
			ECXX_Send_AT(module, AT_ROAM_ENABLE, str1, 1000);


		//==========================( Set Technology Lock )=======================
		memset(str2,0,sizeof(str1));
		exec(UCI_GET_TECHLOCK,str2,100);


		if(str1[0]=='0')
			ECXX_Send_AT(module, AT_TECH_LOCK_ALL, str1, 1000);
		else if(str2[0]=='1')
			ECXX_Send_AT(module, AT_TECH_LOCK_GSM, str1, 1000);
		else if(str2[0]=='2')
			ECXX_Send_AT(module, AT_TECH_LOCK_WCDMA, str1, 1000);
		else if(str2[0]=='3')
			ECXX_Send_AT(module, AT_TECH_LOCK_UMTS, str1, 1000);
		else if(str2[0]=='4')
			ECXX_Send_AT(module, AT_TECH_LOCK_LTE, str1, 1000);



		//========================( Set SMS to Text Mode )=======================
		memset(str1,0,sizeof(str1));
		ECXX_Send_AT(module, AT_SET_SMS_TXT, str1, 1000);

		//========================( Set SMS to Text Mode )=======================
		memset(str1,0,sizeof(str1));
		ECXX_Send_AT(module, AT_DEL_SMS_ALL, str1, 1000);

	}

}


void ECXX_Get_atcmd_data2(ECXX *module,uint8_t param)
{
	char str1[ECXX_BUFFER_SIZE],str2[100];
	int f1=0,f2=0;
	string string_tmp,ss=str1;

	switch(param)
	{
		case 1:
			//==========================( Read CPIN )=====================================
			ECXX_Send_AT(module, AT_CPIN, str1, 1000);									//
			ss=str1;																	//
																						//
			f1=ss.find("IN: ");															//
			if(f1>0)																	//
			{																			//
				if(module->status.SimReady!=YES)										//
				{																		//
					f1+=4;																//
					f2=ss.find("\r\n",f1);												//
					string_tmp=ss.substr(f1,f2-f1);										//
					exec2(CPY_SIMICON1);												//
					module->status.SimReady=YES;										//
					memset(str2,0,sizeof(str2));										//
					sprintf(str2,"uci set modem.usim.state=%s",string_tmp.c_str());		//
					exec2(str2);														//
					//drive NET LED
					if(module->status.Connected==YES)									//
					{																	//
						exec2(LED_NET_ONDLY1);											//
						exec2(LED_NET_OFFDLY4);											//
					}																	//
					else																//
					{																	//
						exec2(LED_NET_ONDLY2);											//
						exec2(LED_NET_OFFDLY2);											//
					}																	//
					if(module->ModuleType==EC200U) exec2(USBNET0_up);		  			//
				}																		//
			}																			//
			else																		//
			{																			//
				if(module->status.SimReady!=NO)											//
				{																		//
					string_tmp="--";													//
					exec2(CPY_SIMICON0);												//
					module->status.SimReady=NO;											//
					memset(str2,0,sizeof(str2));										//
					sprintf(str2,"uci set modem.usim.state=%s",string_tmp.c_str());		//
					exec2(str2);														//
					exec2(LED_NET_ONDLY2);												//
					exec2(LED_NET_OFFDLY1);												//
					if(module->ModuleType==EC200U) exec2(USBNET0_down);		  			//
				}																		//
			}																			//
																						//
			//=============================================================================
			break;

		case 2:
			//==========================( Read ICCID )====================================
			ECXX_Send_AT(module, AT_ICCID, str1, 1000);									//
			ss=str1;																	//
																						//
			f1=ss.find("ID: ");											    			//
			if(f1>0)																	//
			{																	        //
				f1+=4;																	//
				f2=ss.find("\r\n",f1);									 				//
				string_tmp=ss.substr(f1,f2-f1);		 									//
			}																			//
			else string_tmp="--";														//
																						//
			memset(str2,0,sizeof(str2));											    //
			sprintf(str2,"uci set modem.usim.iccid=%s",string_tmp.c_str());				//
			exec2(str2);																//
			//============================================================================
			break;

		case 3:
			//==========================( Read IMSI )=====================================
			ECXX_Send_AT(module, AT_GET_IMSI, str1, 1000);								//
			ss=str1;																	//
																						//
			f1=ss.find("\r\n");															//
			f2=ss.find("OK");															//
			if(f2>2)																	//
			{																			//
				f1+=2;																	//
				f2=ss.find("\r\n",f1);													//
				string_tmp=ss.substr(f1,f2-f1);											//
			}																			//
			else string_tmp="--";														//
			memset(str2,0,sizeof(str2));												//
			sprintf(str2,"uci set modem.numbers.imsi=%s",string_tmp.c_str());			//
			exec2(str2);																//
			//============================================================================
			break;

		case 4:
			//==========================( Read IMEI )=====================================
			ECXX_Send_AT(module, AT_GET_IMEI, str1, 1000);								//
			ss=str1;																	//
																						//
			f1=ss.find("\r\n");															//
			f2=ss.find("OK");															//
			if(f2>2)																	//
			{																			//
				f1+=2;																	//
				f2=ss.find("\r\n",f1);													//
				string_tmp=ss.substr(f1,f2-f1);											//
			}																			//
			else string_tmp="--";														//
			memset(str2,0,sizeof(str2));												//
			sprintf(str2,"uci set modem.numbers.imei=%s",string_tmp.c_str());			//
			exec2(str2);																//
			//============================================================================
			break;

		case 5:
			//=======================( Read network information )=========================
			ECXX_Send_AT(module, AT_NETW_STS, str1, 1000);								//
																						//
			ss=str1;																	//
																						//
			f1=ss.find("FO: ");								//first parameter			//
			if(f1>0)																	//
			{																			//
				f1+=4;																	//
				f2=ss.find(",",f1);														//
				string_tmp=ss.substr(f1,f2-f1);											//
			}																			//
			else string_tmp="--";														//
			memset(str2,0,sizeof(str2));												//
			sprintf(str2,"uci set modem.network.technology=%s",string_tmp.c_str());		//
			exec2(str2);																//
																						//
			f1=f2+1;																	//
			f2=ss.find(",",f1);								//first parameter			//
			if(f2>0)																	//
			{																			//
				string_tmp=ss.substr(f1,f2-f1);											//
			}																			//
			else string_tmp="--";														//
			memset(str2,0,sizeof(str2));												//
			sprintf(str2,"uci set modem.network.operator=%s",string_tmp.c_str());		//
			exec2(str2);																//
																						//
			f1=f2+1;																	//
			f2=ss.find(",",f1);								//first parameter			//
			if(f2>0)																	//
			{																			//
				string_tmp=ss.substr(f1,f2-f1);											//
			}																			//
			else string_tmp="--";														//
			memset(str2,0,sizeof(str2));												//
			sprintf(str2,"uci set modem.network.band=%s",string_tmp.c_str());			//
			exec2(str2);																//
			//============================================================================
			break;

		case 6:
			//==========================( Read NET CONNECTION )===========================
			ECXX_Send_AT(module, AT_GET_CGATT, str1, 1000);								//
			ss=str1;																	//
			f2=ss.find("1");															//
			memset(str2,0,sizeof(str2));												//
			if(f2>2)																	//
			{																			//
				if(module->status.Connected!=YES)										//
				{																		//
					sprintf(str2,"uci set modem.data.connected=YES");					//
					exec2(CPY_NETICON1);												//
					exec2(str2);														//
					module->status.Connected=YES;										//
					if(module->status.SimReady==YES)									//
					{																	//
						exec2(LED_NET_ONDLY1);											//
						exec2(LED_NET_OFFDLY4);											//
					}																	//
				}																		//
																						//
				ECXX_Send_AT(module, AT_GET_PDP_STATUS, str1, 1000);					//
				ss=str1;																//
																						//
				f1=ss.find("\"");														//
				if(f1>2)																//
				{																		//
					f1+=1;																//
					f2=ss.find("\"",f1);												//
					string_tmp=ss.substr(f1,f2-f1);										//
				}																		//
				else string_tmp="--";													//
				memset(str2,0,sizeof(str2));											//
				sprintf(str2,"uci set modem.data.ip=%s",string_tmp.c_str());			//
				exec2(str2);															//
																						//
			}																			//
			else																		//
			{																			//
				if(module->status.Connected!=NO)										//
				{																		//
					sprintf(str2,"uci set modem.data.connected=NO");					//
					exec2(str2);														//
					exec2(CPY_NETICON0);												//
					module->status.Connected=NO;										//
					if(module->status.SimReady==YES)									//
					{																	//
						exec2(LED_NET_ONDLY2);											//
						exec2(LED_NET_OFFDLY2);											//
					}																	//
				}																		//
			}																			//
																						//
			//============================================================================
			break;

		case 7:
			//==========================( Read Registration Status )======================
			ECXX_Send_AT(module, AT_GET_CGREG, str1, 1000);								//
																						//
			ss=str1;																	//
			f2=ss.find(",");															//
			memset(str2,0,sizeof(str2));												//
			if(f2>1)																	//
			{																			//
				switch(str1[f2+1])														//
				{																		//
					case '0':															//
						sprintf(str2,"uci set modem.network.registration=NotRegisterd");//
						module->status.Registration=NO;									//
						module->status.Roaming=NO;										//
					break;																//
																						//
					case '1':															//
						sprintf(str2,"uci set modem.network.registration=Registerd");	//
						module->status.Registration=YES;								//
						module->status.Roaming=NO;										//
					break;																//
																						//
					case '2':															//
						sprintf(str2,"uci set modem.network.registration=Searching");	//
						module->status.Registration=NO;									//
						module->status.Roaming=NO;										//
					break;																//
																						//
					case '3':															//
						sprintf(str2,"uci set modem.network.registration=Denied");		//
						module->status.Registration=NO;									//
						module->status.Roaming=NO;										//
					break;																//
																						//
					case '5':															//
						sprintf(str2,"uci set modem.network.registration=Registered");	//
						module->status.Registration=YES;								//
						module->status.Roaming=YES;										//
					break;																//
																						//
					default:															//
						sprintf(str2,"uci set modem.network.registration=---");			//
						module->status.Registration=NO;									//
						module->status.Roaming=NO;										//
					break;																//
																						//
				}																		//
				exec2(str2);															//
				memset(str2,0,sizeof(str2));											//
				if(module->status.Roaming==YES)											//
					sprintf(str2,"uci set modem.network.roaming=YES");					//
				else sprintf(str2,"uci set modem.network.roaming=NO");					//
																						//
			}																			//
			else																		//
			{																			//
				sprintf(str2,"uci set modem.network.registration=---");					//
				module->status.Registration=NO;											//
			}																			//
			exec2(str2);																//
			//==========================================================================//
			break;


	}

	//Commit uci config changes
	exec2(UCI_COMMIT_MODEM);
}


void ECXX_Get_atcmd_data1(ECXX *module)
{
	char str1[ECXX_BUFFER_SIZE],str2[100];
	int f1=0,f2=0;
	char str[20][20];
	string string_tmp;

	//====================( Read Serving Cell information )=======================
	ECXX_Send_AT(module, AT_SERV_CELL, str1, 1000);								//
	string ss=str1;																//
																				//
	for(int i=0;i<20;i++) memset(str[i],0,sizeof(str[i]));						//
																				//
	f1=ss.find(",");															//
																				//
	for(int i=0;i<16;i++)														//
	{																			//
		f2=ss.find(",",f1+1);													//
		if(f2>0)																//
		{																		//
			sprintf(str[i],"%s",ss.substr(f1+1,f2-f1-1).c_str());				//
			f1=f2;																//
		}																		//
		else																	//
		{																		//
			sprintf(str[i],"--");												//
			break;																//
		}																		//
	}																			//
																				//
	memset(str2,0,sizeof(str2));												//
	if(strlen(str[1])>1) sprintf(str2,"uci set modem.signal.type=%s",str[1]);	//
	else sprintf(str2,"uci set modem.signal.type=---",str[1]);					//
	exec2(str2);																//


	int rssi=0;
	if(str[1][1]=='G' || str[1][1]=='W')							//GSM Mode
	{
		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.cellid=%s",str[5]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mcc=%s",str[2]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mnc=%s",str[3]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrp=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrq=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.snr=--");
		exec2(str2);


		ECXX_Send_AT(module, AT_CSQ, str1, 1000);								//
		ss=str1;


		f1=ss.find("SQ: ");
		f2=ss.find(",",f1);
		string_tmp=ss.substr(f1+4,f2-f1);

		try{
			rssi=stoi(string_tmp);
		}
		catch(...){
		}

		rssi=rssi*2-113;
		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rssi=%d",rssi);
		exec2(str2);

	}
	else if(str[1][1]=='L')
	{
		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.cellid=%s",str[5]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mcc=%s",str[3]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mnc=%s",str[4]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrp=%s",str[12]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrq=%s",str[13]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rssi=%s",str[14]);
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.snr=%s",str[15]);
		exec2(str2);

		string_tmp=str[14];

		try{
			rssi=stoi(string_tmp);
		}
		catch(...){

		}
	}
	else
	{
		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.cellid=Searching...");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mcc=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.network.plmn_mnc=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrp=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rsrq=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.snr=--");
		exec2(str2);

		memset(str2,0,sizeof(str2));
		sprintf(str2,"uci set modem.signal.rssi=--",rssi);
		exec2(str2);
		rssi=0;

	}


	//Drive RSSI LED
	if(rssi< -100 || module->status.SimReady==NO)
	{
		exec2(LED_SIG_PATT0);
		if(module->status.signum!=0)
		{
			exec2(CPY_SIGNALICON0);
			module->status.signum=0;
		}
	}
	else if(rssi>=-100 && rssi<-85 ){
		exec2(LED_SIG_PATT1);
		if(module->status.signum!=1)
		{
			exec2(CPY_SIGNALICON1);
			module->status.signum=1;
		}
	}
	else if(rssi>=-85 && rssi<-75 ){
		exec2(LED_SIG_PATT2);
		if(module->status.signum!=2)
		{
			exec2(CPY_SIGNALICON2);
			module->status.signum=2;
		}
	}
	else if(rssi>=-75 && rssi<-65){
		exec2(LED_SIG_PATT3);
		if(module->status.signum!=3)
		{
			exec2(CPY_SIGNALICON3);
			module->status.signum=3;
		}
	}
	else if(rssi>=-65 && rssi<0){
		exec2(LED_SIG_PATT4);
		if(module->status.signum!=4)
		{
			exec2(CPY_SIGNALICON4);
			module->status.signum=4;
		}
	}
	else
	{
		exec2(LED_SIG_PATT0);
		if(module->status.signum!=5)
		{
			exec2(CPY_SIGNALICON5);
			module->status.signum=5;
		}
	}


	exec2(UCI_COMMIT_MODEM);
	//============================================================================

}



void ECXX_Get_Status(ECXX *module)
{

	report("s","ECXX --> GET_STATUS_Thread Started!");

	//Check for Update Flag and send update completion message
	Update_Check();


	while(1)
	{
		//Check for new sms
		New_SMS_Check(module);

		//Get Modem status parameters from module
		ECXX_Get_atcmd_data1(module);
		ECXX_Get_atcmd_data2(module,1);
		ECXX_Get_atcmd_data2(module,2);
		ECXX_Get_atcmd_data2(module,3);

		sleep(2);

		//Check for new sms
		New_SMS_Check(module);

		//Get Modem status parameters from module
		ECXX_Get_atcmd_data1(module);
		ECXX_Get_atcmd_data2(module,4);
		ECXX_Get_atcmd_data2(module,5);
		ECXX_Get_atcmd_data2(module,6);
		ECXX_Get_atcmd_data2(module,7);

		//Check for Simcard switch conditions
		Sim_Switch_Check(module);
		sleep(2);


		//Check for any change in Modem settings in uci config file
		if(Settings_change_Check(module)==ERROR)
		{
			report("s","Modem settings has changes ! -Restarting Modem !");
			//Read and set modem parameters from uci config file
			ECXX_Set_Parameters(module);
			//initialize modem
			ECXX_Init(module,1);
		}
	}

}




result ECXX_Ping_Check()
{
	char str1[300],f1=0;
	memset(str1,0,sizeof(str1));
	exec(EC25_PING_CHECK,str1,2500);
	string ss=str1;

	f1=ss.find("time");
	if(f1 < 10)
	{
		return ERROR;
	}

	else return OK;

}

result ECXX_Port_Close(SerialPort *port)
{
	close(port->Port_fd);
	return OK;
}

result ECXX_Port_Open(SerialPort *port)
{
	result res=ERROR;

	struct termios tty;
	try
	{
		port->Port_fd = open(port->Parameters.File, O_RDWR);
	}
	catch (...)
	{

		report("s","ECXX --> SERIAL_FILE_NOT_EXIST!!");
		res=ERROR;
		return res;
	}

	if (port->Port_fd < 0) {
		report("ss","ECXX --> Error file open:", strerror(errno));

	}

	if(port->Parameters.Parity==PARITY_NONE)
		tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	else
		tty.c_cflag |= PARENB;  // Set parity bit, enabling parity

	if(port->Parameters.StopBits==STOPBITS_ONE)
		tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	else
		tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication

	tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size

	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed


	tty.c_cc[VTIME] = 0;    // Wait for up to 100ms (1 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate
	cfsetispeed(&tty, port->Parameters.BaudRate);
	cfsetospeed(&tty, port->Parameters.BaudRate);


	try
	{
		int r=tcsetattr(port->Port_fd , TCSAFLUSH, &tty);
		if (r != 0)
		{

			report("ss","ECXX --> PORT_OPEN_ERROR= ", strerror(errno));
			res=ERROR;
		}
		else
		{

			report("s","ECXX --> PORT_OPEN_SUCCESSFULL!");
			res=OK;
		}
	}
	catch (...)
	{

		report("s","ECXX --> SERIAL_FILE_NOT_EXIST!!!");
		res=ERROR;
	}

	return res;
}

result ECXX_Start(ECXX *module)
{
	pthread_t	Thread1,Thread2;

	//Set Module Type to Null
	module->ModuleType=MODULENONE;

	//Read and set Module configuration parameters from uci config file
	ECXX_Set_Parameters(module);

	//Try to find Module USB-Serial file in /dev
	while(1)
	{
		//Check for EC200U USB-Serial file
		module->PORT.Parameters.File=EC200U_SERIAL_FILE;
		if(ECXX_Port_Open(&module->PORT)==OK)
		{
			module->ModuleType=EC200U;
			//initialize Module
			ECXX_Init(module,0);
			module->status.Ready=YES;
			return OK;
		}

		//Check for EC200U USB-Serial file
		module->PORT.Parameters.File=EC200U_SERIAL_FILE2;				//EC200U Module check2
		if(ECXX_Port_Open(&module->PORT)==OK)
		{
			module->ModuleType=EC200U;
			//initialize Module
			ECXX_Init(module,0);
			module->status.Ready=YES;
			return OK;
		}

		//Check for EC200 or EC25 USB-Serial file
		module->PORT.Parameters.File=EC25_SERIAL_FILE;				//EC25-EC200 Module check
		if(ECXX_Port_Open(&module->PORT)==OK)
		{
			module->ModuleType=EC25;
			//initialize Module
			ECXX_Init(module,0);
			module->status.Ready=YES;
			pthread_create( &Thread1 , NULL , EC25_Manager_thread ,NULL);
			sleep(10);
			return OK;
		}

		//Try to Turn the Module off/on When the USB-Serial file is not found
		ECXX_Power_ON(module);
	}

	return ERROR;
}


//Restart Quectel module
result ECXX_Reset(ECXX *module)
{

	report("s","ECXX --> Restarting !");
	ECXX_Port_Close(&module->PORT);
	gpio.Write(GPIO_OUT_GSMRST, 1);
	sleep(1);
	gpio.Write(GPIO_OUT_GSMRST, 0);
	sleep(10);
	if(ECXX_Port_Open(&module->PORT)!=OK)
	{
		if(module->ModuleType==EC200U)
		{
			module->PORT.Parameters.File=EC200U_SERIAL_FILE2;
			if(ECXX_Port_Open(&module->PORT)==OK) return OK;
			else
			{
				module->PORT.Parameters.File=EC200U_SERIAL_FILE;
				if(ECXX_Port_Open(&module->PORT)==OK) return OK;
			}
		}
	}
	else return OK;

	ECXX_Start(module);
}


result ECXX_Power_ON(ECXX *module)
{
	report("s","ECXX --> Powering On module !");

	gpio.Write(GPIO_OUT_GSMPWR, 1);
	sleep(2);
	gpio.Write(GPIO_OUT_GSMPWR, 0);

	sleep(10);

	return OK;
}


result ECXX_Power_OFF(ECXX *module)
{

	gpio.Write(GPIO_OUT_GSMPWR, 1);
	report("s","EC25 --> Powering On !");

	sleep(2);

	gpio.Write(GPIO_OUT_GSMPWR, 0);
	report("s","EC25 --> Powered OFF !");

	return OK;
}


result ECXX_Stop(ECXX *module)
{

	return OK;
}


result Settings_change_Check(ECXX *module)
{
	char buffer[2048];

	memset(buffer,0,sizeof(buffer));
	exec("uci show modem.parameter", buffer, 10);


	for(uint16_t i=0; i< strlen(buffer) ; i++)
	{
		if( buffer[i]!= module->settingfile[i] )
		{
			return ERROR;
		}
	}

	return OK;

}

void Sim_Switch_Check(ECXX *module)
{
	//Check for  no simcard  Duration
	if(module->status.SimReady==NO)
	{
		module->status.NoSimCount++;
		module->status.NoSigCount=0;
		module->status.NoNetCount=0;


		if(module->status.NoSimCount == NO_SIM_TIMEOUT)
		{
			if(module->SimCard==0) module->SimCard=1;
			else module->SimCard=0;
			ECXX_Init(module,1);

			report("sd","ECXX --> SIM Card Changed Due No Sim! - New SIM: SIM",module->SimCard+1);
			module->status.NoSimCount=0;
		}
	}
	//Check for Weak Signal Duration
	else if(module->status.signum < 1)
	{
		module->status.NoSigCount++;
		module->status.NoSimCount=0;
		module->status.NoNetCount=0;


		if(module->status.NoSigCount == NO_SIG_TIMEOUT)
		{
			if(module->SimCard==0) module->SimCard=1;
			else module->SimCard=0;
			ECXX_Init(module,1);
			module->status.NoSigCount=0;

			report("sd","ECXX --> SIM Card Changed Due Weak Signal! - New SIM: SIM",module->SimCard+1);
		}
	}
	//Check for no net duration
	else if(module->status.Connected ==NO)
	{
		module->status.NoNetCount++;
		module->status.NoSigCount=0;
		module->status.NoSimCount=0;


		if(module->status.NoNetCount == NO_NET_TIMEOUT)
		{
			if(module->SimCard==0) module->SimCard=1;
			else module->SimCard=0;
			ECXX_Init(module,1);
			module->status.NoNetCount=0;

			report("sd","ECXX --> SIM Card Changed Due No Connection! - New SIM: SIM",module->SimCard+1);
		}
	}
	else
	{
		module->status.NoSigCount=0;
		module->status.NoSimCount=0;
		module->status.NoNetCount=0;
	}
}


void New_SMS_Check(ECXX *module)
{
	char str1[ECXX_BUFFER_SIZE],ret=0;
	int f1=0,f2=0;

	memset(str1,0,sizeof(str1));

	//Send AT command for read SMS index:0
	ECXX_Send_AT(module, AT_READ_SMS_0, str1, 1000);

	string ss=str1;
	string number,txt;

	//Find SMS record standard format in sms record buffer
	f1=ss.find(",");
	if(f1>0)
	{

		//Split SMS number from buffer
		f2=ss.find("\"",f1+3);
		f1+=2;
		number=ss.substr(f1,f2-f1);

		//Split SMS text from buffer
		f1=ss.find("\n",f2);
		if(f1>0)
		{
			f2=ss.find("\n",f1+3);
			txt=ss.substr(f1,f2-f1);
		}

		//Delete SMS record index:0
		ECXX_Send_AT(module, AT_DEL_SMS_ALL, str1, 1000);

		//Process the SMS text and Number
		ret=SMS_Process(number,txt);

		//Report SMS Receive in system logs
		report("ss","ECXX --> New SMS Received From : ", number.c_str());

		//Check for valid Lan configuration in SMS text and restart LAN interface
		if(ret==3)
		{
			exec2(LAN_down);
			sleep(4);
			exec2(LAN_up);
		}

	}


}


void Read_TelNumbers(void)
{
	char str[100],str1[100];

	//Read Tel numbers from filter list
	for(int x=0 ; x<5 ;x++)
	{
		memset(str,0,sizeof(str));
		sprintf(str,"uci get sms.number%d.number",x+1);
		memset(admin.numbers[x],0,sizeof(admin.numbers[x]));
		exec(str, admin.numbers[x], 10);
	}

	//Read enable status from filter list
	for(int y=0 ; y<5 ; y++ )
	{
		memset(str,0,sizeof(str));
		sprintf(str,"uci get sms.number%d.enable",y+1);
		exec(str, admin.enables[y], 10);
	}

	//Read admin status from filter list
	for(int z=0 ; z<5 ; z++ )
	{
		memset(str,0,sizeof(str));
		sprintf(str,"uci get sms.number%d.admin",z+1);
		exec(str, admin.admins[z], 10);
	}
}


int TelNumber_Check(string Number)
{
	int nonum=0,t=0;

	for(char n=0 ;n<5 ;n++)
	{
		//Count of empty/disable numbers
		if(admin.enables[n][0]!='1') nonum++;
		else
		{
			//Compare the filter number with sms number
			//compare will be for the last digits of numbers
			t=Number.length()-3;
			t=strlen(admin.numbers[n])-t-1;
			char s1[100];
			char s2[100];
			char s3[100];
			memset(s1,0,sizeof(s1));
			memset(s2,0,sizeof(s2));
			memset(s3,0,sizeof(s3));
			sprintf(s1,"%s",admin.numbers[n]+t);
			if(strlen(s1))	memcpy(s3,s1,strlen(s1)-1);
			sprintf(s2,"%s",Number.c_str()+3);

			//return filter number index if the numbers matches
			if( strcmp(s2,s3)==0 &&  strncmp(admin.enables[n],"1",1)==0)	return n+1;
		}
	}
	//return 10 if all number in the filter list are empty/disable numbers
	if(nonum==5)		return 10;

	//return 0 when numbers can not match
	else	return 0 ;

}


char SMS_Process(string Number,string text)
{
	char response[500],ret=0;
	memset(response,0,sizeof(response));

	//Read SMS filter numbers from uci config file
	Read_TelNumbers();

	//Check new SMS number with SMS filter numbers
	int rr=TelNumber_Check(Number);
	if(rr)
	{
		//Check new SMS text to find SMS commands
		ret=SMS_Settings_Extract(text,response,Number);
		//Send the result of SMS commands back to user
		if(strlen(response)>4) SMS_Send(Number,response,&modem);
	}
	return ret;
}


char SMS_Settings_Extract(string data,char *response,string number1)
{
	int f1;
	char Need_Init=0;
	char str[200],str1[20],datatostr[1000];

	//Convert string structure to char array
	memset(datatostr,0,sizeof(datatostr));
	sprintf(datatostr,"%s",data.c_str());

	//Convert all of SMS test characters to Upper case characters
	for(int i=0; i<strlen(datatostr);i++)
	{
		datatostr[i]=toupper(datatostr[i]);
	}
	//Convert char array to string structure
	data = datatostr;


	f1 = data.find("HELP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get system.@system[0].sn");
		exec(str,str1,10);
		help.SN=str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get system.@system[0].firmware");
		exec(str,str1,10);
		help.Firmware=str1;

		sprintf(response,"ZS-LRM-203\n");
		sprintf(response+strlen(response),"SN:%s",help.SN.c_str());
		sprintf(response+strlen(response),"Firmware:%s",help.Firmware.c_str());
		sprintf(response+strlen(response),"GPRS?\n");
		sprintf(response+strlen(response),"DataCenter?\n");
		sprintf(response+strlen(response),"LAN?\n");
		sprintf(response+strlen(response),"USER?\n");
		sprintf(response+strlen(response),"IO?\n");
		sprintf(response+strlen(response),"FTP?\n");
		sprintf(response+strlen(response),"Update\n");
		//printf("%s\n",response);
	}
	f1 = data.find("GPRS?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.sim1apn");
		exec(str,str1,10);
		if(strlen(str1)>0) gprs.APN1 = str1;
		else gprs.APN1 = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.sim2apn");
		exec(str,str1,10);
		if(strlen(str1)>0) gprs.APN2 = str1;
		else gprs.APN2 = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.defultsimcard");
		exec(str,str1,10);
		str1[0]++;
		gprs.DefSim=str1;   //change index  to sim number (sim1:0  sim2:1)

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.signal.rssi");
		exec(str,str1,10);
		if(strlen(str1)>0) gprs.RSSI = str1;
		else gprs.RSSI = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.usim.currentsim");
		exec(str,str1,10);
		if(strlen(str1)>0) gprs.Simcard = str1;
		else gprs.Simcard = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.data.ip");
		exec(str,str1,10);
		if(strlen(str1)>0) gprs.IP_ADD = str1;
		else gprs.IP_ADD = "\n";


		sprintf(response,"APN1:%s",gprs.APN1.c_str());
		sprintf(response+strlen(response),"APN2:%s",gprs.APN2.c_str());
		sprintf(response+strlen(response),"DefSim:%s",gprs.DefSim.c_str());
		sprintf(response+strlen(response),"RSSI:%s",gprs.RSSI.c_str());
		sprintf(response+strlen(response),"SimCard:%s",gprs.Simcard.c_str());
		sprintf(response+strlen(response),"IP_ADD:%s",gprs.IP_ADD.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("LAN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.ipaddr");
		exec(str,str1, 10);
		if(strlen(str1)>0) lan.LAN_IP = str1;
		else lan.LAN_IP = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.gateway");
		exec(str,str1, 10);
		if(strlen(str1)>0) lan.LAN_GW = str1;
		else lan.LAN_GW = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.netmask");
		exec(str,str1, 10);
		if(strlen(str1)>0) lan.LAN_NM = str1;
		else lan.LAN_NM = "\n";


		sprintf(response,"LAN_IP:%s",lan.LAN_IP.c_str());
		sprintf(response+strlen(response),"LAN_GW:%s",lan.LAN_GW.c_str());
		sprintf(response+strlen(response),"LAN_NM:%s",lan.LAN_NM.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("DATACENTER?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.mode");
		exec(str,str1,10);
		if(str1[0]=='1')				datacenter.TCP_M="Server";
		if(str1[0]=='2')				datacenter.TCP_M="Client";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server1address");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.S1_IP = str1;
		else datacenter.S1_IP = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server1port");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.S1_PRT = str1;
		else datacenter.S1_PRT = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server2address");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.S2_IP = str1;
		else datacenter.S2_IP = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server2port");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.S2_PRT = str1;
		else datacenter.S2_PRT = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server3address");
		exec(str,str1,10);
		datacenter.S3_IP = str1;
		if(strlen(str1)>0) datacenter.S3_IP = str1;
		else datacenter.S3_IP = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server3port");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.S3_PRT = str1;
		else datacenter.S3_PRT = "\n";

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.listenport");
		exec(str,str1,10);
		if(strlen(str1)>0) datacenter.L_PRT = str1;
		else datacenter.L_PRT = "\n";

		sprintf(response,"TCP_M:%s\n",datacenter.TCP_M.c_str());
		sprintf(response+strlen(response),"S1_IP:%s",datacenter.S1_IP.c_str());
		sprintf(response+strlen(response),"S1_PRT:%s",datacenter.S1_PRT.c_str());
		sprintf(response+strlen(response),"S2_IP:%s",datacenter.S2_IP.c_str());
		sprintf(response+strlen(response),"S2_PRT:%s",datacenter.S2_PRT.c_str());
		sprintf(response+strlen(response),"S3_IP:%s",datacenter.S3_IP.c_str());
		sprintf(response+strlen(response),"S3_PRT:%s",datacenter.S3_PRT.c_str());
		sprintf(response+strlen(response),"L_PRT:%s",datacenter.L_PRT.c_str());
		//printf("%s\n",response);
	}
	f1 = data.find("USER?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number1.number");
		exec(str,str1, 10);
		user.PH1 = str1;
		user.PH1=user.PH1.substr(0, strlen(user.PH1.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number2.number");
		exec(str,str1, 10);
		user.PH2 = str1;
		user.PH2=user.PH2.substr(0, strlen(user.PH2.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number3.number");
		exec(str,str1, 10);
		user.PH3 = str1;
		user.PH3=user.PH3.substr(0, strlen(user.PH3.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number4.number");
		exec(str,str1, 10);
		user.PH4 = str1;
		user.PH4=user.PH4.substr(0, strlen(user.PH4.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number5.number");
		exec(str,str1, 10);
		user.PH5 = str1;
		user.PH5=user.PH5.substr(0, strlen(user.PH5.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number1.enable");
		exec(str,str1, 10);
		user.PH1EN = str1;
		user.PH1EN=user.PH1EN.substr(0, strlen(user.PH1EN.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number2.enable");
		exec(str,str1, 10);
		user.PH2EN = str1;
		user.PH2EN=user.PH2EN.substr(0, strlen(user.PH2EN.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number3.enable");
		exec(str,str1, 10);
		user.PH3EN = str1;
		user.PH3EN=user.PH3EN.substr(0, strlen(user.PH3EN.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number4.enable");
		exec(str,str1, 10);
		user.PH4EN = str1;
		user.PH4EN=user.PH4EN.substr(0, strlen(user.PH4EN.c_str())-1);

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number5.enable");
		exec(str,str1, 10);
		user.PH5EN = str1;
		user.PH5EN=user.PH5EN.substr(0, strlen(user.PH5EN.c_str())-1);


		sprintf(response,"PH1:%s\n",user.PH1.c_str());
		sprintf(response+strlen(response),"PH2:%s\n",user.PH2.c_str());
		sprintf(response+strlen(response),"PH3:%s\n",user.PH3.c_str());
		sprintf(response+strlen(response),"PH4:%s\n",user.PH4.c_str());
		sprintf(response+strlen(response),"PH5:%s\n",user.PH5.c_str());
		sprintf(response+strlen(response),"PH1EN:%s\n",user.PH1EN.c_str());
		sprintf(response+strlen(response),"PH2EN:%s\n",user.PH2EN.c_str());
		sprintf(response+strlen(response),"PH3EN:%s\n",user.PH3EN.c_str());
		sprintf(response+strlen(response),"PH4EN:%s\n",user.PH4EN.c_str());
		sprintf(response+strlen(response),"PH5EN:%s\n",user.PH5EN.c_str());
		//printf("%s\n",response);


	}
	f1 = data.find("IO?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.output.relay1");
		exec(str,str1, 10);
		io.OUT1 = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.output.relay2");
		exec(str,str1, 10);
		io.OUT2 = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input1");
		exec(str,str1, 10);
		io.IN1 = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input2");
		exec(str,str1, 10);
		io.IN2 = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input3");
		exec(str,str1, 10);
		io.IN3 = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input4");
		exec(str,str1, 10);
		io.IN4 = str1;

		sprintf(response,"OUT1:%s",io.OUT1.c_str());
		sprintf(response+strlen(response),"OUT2:%s",io.OUT2.c_str());
		sprintf(response+strlen(response),"IN1:%s",io.IN1.c_str());
		sprintf(response+strlen(response),"IN2:%s",io.IN2.c_str());
		sprintf(response+strlen(response),"IN3:%s",io.IN3.c_str());
		sprintf(response+strlen(response),"IN4:%s",io.IN4.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("FTP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.address");
		exec(str,str1, 10);
		ftp.FTP_ADD = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.port");
		exec(str,str1, 10);
		ftp.FTP_PRT = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.user");
		exec(str,str1, 10);
		ftp.FTP_USR = str1;

		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.pass");
		exec(str,str1, 10);
		ftp.FTP_PAS = str1;



		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.path");
		exec(str,str1, 10);
		ftp.FTP_PATH = str1;



		sprintf(response,"FTP_ADD:%s",ftp.FTP_ADD.c_str());
		sprintf(response+strlen(response),"FTP_PRT:%s",ftp.FTP_PRT.c_str());
		sprintf(response+strlen(response),"FTP_USR:%s",ftp.FTP_USR.c_str());
		sprintf(response+strlen(response),"FTP_PAS:%s",ftp.FTP_PAS.c_str());
		sprintf(response+strlen(response),"FTP_PATH:%s",ftp.FTP_PATH.c_str());
		//printf("%s\n",response);

	}

	f1 = data.find("UPDATE");
	if(f1>0)
	{
		Update(number1);
	}




	f1 = data.find("APN1?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.sim1apn");
		exec(str,str1,10);
		gprs.APN1=str1;
		sprintf(response,"APN1:%s",gprs.APN1.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("APN2?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.sim2apn");
		exec(str,str1,10);
		gprs.APN2=str1;
		sprintf(response,"APN2:%s",gprs.APN2.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("DEFSIM?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.parameter.defultsimcard");
		exec(str,str1,10);
		gprs.DefSim=str1;
		sprintf(response,"DEFSIM:%s",gprs.DefSim.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("RSSI?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.signal.rssi");
		exec(str,str1,10);
		gprs.RSSI=str1;
		sprintf(response,"RSSI:%s",gprs.RSSI.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("SIMCARD?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.usim.currentsim");
		exec(str,str1,10);
		gprs.Simcard=str1;
		sprintf(response,"SIMCARD:%s",gprs.Simcard.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("IP_ADD?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get modem.data.ip");
		exec(str,str1,10);
		gprs.IP_ADD=str1;
		sprintf(response,"IP_ADD:%s",gprs.IP_ADD.c_str());
		//printf("%s\n",response);

	}
	f1 =data.find("APN1:");
	if(f1>0)
	{
		gprs.APN1=data.substr(f1+5);
		gprs.APN1=gprs.APN1.substr(0, strlen(gprs.APN1.c_str())-1);
		strcat(response,"APN1:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set modem.parameter.sim1apn=%s",gprs.APN1.c_str());
		//printf("str is %s\n",str);
		exec2(str);
		Need_Init=1;
	}
	f1 =data.find("APN2:");
	if(f1>0)
	{
		gprs.APN2=data.substr(f1+5);
		gprs.APN2=gprs.APN2.substr(0, strlen(gprs.APN2.c_str())-1);
		strcat(response,"APN2:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set modem.parameter.sim2apn=%s",gprs.APN2.c_str());
		exec2(str);
		//printf("str is %s\n",str);
		Need_Init=1;
	}

	f1 =data.find("DEFSIM:");
	if(f1>0)
	{
		gprs.DefSim=data.substr(f1+7);
		gprs.DefSim=gprs.DefSim.substr(0, strlen(gprs.DefSim.c_str())-1);
		gprs.DefSim[0]--;
		strcat(response,"DefSim:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set modem.parameter.defultsimcard=%s",gprs.DefSim.c_str());
		if(gprs.DefSim[0]=='1') modem.SimCard=1;
		else modem.SimCard=0;

		exec2(str);
		//printf("str is %s\n",str);
		Need_Init=1;
	}

	f1 = data.find("TCP_M?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.mode");
		exec(str,str1,10);
		if(str1[0]=='1')				datacenter.TCP_M="Server";
		if(str1[0]=='2')				datacenter.TCP_M="Client";
		sprintf(response,"TCP_M:%s",datacenter.TCP_M.c_str());
		//printf("%s\n",response);
	}
	f1 = data.find("S1_IP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server1address");
		exec(str,str1,10);
		datacenter.S1_IP = str1;
		sprintf(response,"S1_IP:%s",datacenter.S1_IP.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("S1_PRT?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server1port");
		exec(str,str1,10);
		datacenter.S1_PRT = str1;
		sprintf(response,"S1_PRT:%s",datacenter.S1_PRT.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("S2_IP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server2address");
		exec(str,str1,10);
		datacenter.S2_IP = str1;
		sprintf(response,"S2_IP:%s",datacenter.S2_IP.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("S2_PRT?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server2port");
		exec(str,str1,10);
		datacenter.S2_PRT = str1;
		sprintf(response,"S2_PRT:%s",datacenter.S2_PRT.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("S3_IP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server3address");
		exec(str,str1,10);
		datacenter.S3_IP = str1;
		sprintf(response,"S3_IP:%s",datacenter.S3_IP.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("S3_PRT?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.server3port");
		exec(str,str1,10);
		datacenter.S3_PRT = str1;
		sprintf(response,"S3_PRT:%s",datacenter.S3_PRT.c_str());
		//printf("%s\n",response);

	}
	f1 = data.find("L_PRT?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get communication.datacenter.listenport");
		exec(str,str1,10);
		datacenter.L_PRT = str1;
		sprintf(response,"L_PRT:%s",datacenter.L_PRT.c_str());
		//printf("%s\n",response);

	}


	f1 =data.find("TCP_M:");
	if(f1>0)
	{
		datacenter.TCP_M=data.substr(f1+6);
		//printf("datacenter.TCP_M is %s\n",datacenter.TCP_M.c_str());
		if(strstr(datacenter.TCP_M.c_str(),"SERVER"))
		{
			datacenter.TCP_M = '1';
			strcat(response,"TCP_M:OK\n");
		}
		else if(strstr(datacenter.TCP_M.c_str(),"CLIENT"))
		{
			datacenter.TCP_M = '2';
			strcat(response,"TCP_M:OK\n");
		}
		else	strcat(response,"TCP_M:ERROR\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.mode=%s",datacenter.TCP_M.c_str());
		exec2(str);
		//printf("%s\n",str);
		Need_Init=2;
	}

	f1 =data.find("S1_IP:");
	if(f1>0)
	{
		datacenter.S1_IP=data.substr(f1+6);
		datacenter.S1_IP=datacenter.S1_IP.substr(0, strlen(datacenter.S1_IP.c_str())-1);
		strcat(response,"S1_IP:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server1address=%s",datacenter.S1_IP.c_str());
		exec2(str);
		//printf("%s\n",str);
		Need_Init=2;
	}

	f1 =data.find("S1_PRT:");
	if(f1>0)
	{
		datacenter.S1_PRT=data.substr(f1+7);
		datacenter.S1_PRT=datacenter.S1_PRT.substr(0, strlen(datacenter.S1_PRT.c_str())-1);
		strcat(response,"S1_PRT:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server1port=%s",datacenter.S1_PRT.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;
	}

	f1 =data.find("S2_IP:");
	if(f1>0)
	{
		datacenter.S2_IP=data.substr(f1+6);
		datacenter.S2_IP=datacenter.S2_IP.substr(0, strlen(datacenter.S2_IP.c_str())-1);
		strcat(response,"S2_IP:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server1address=%s",datacenter.S2_IP.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;

	}


	f1 =data.find("S2_PRT:");
	if(f1>0)
	{
		datacenter.S2_PRT=data.substr(f1+7);
		datacenter.S2_PRT=datacenter.S2_PRT.substr(0, strlen(datacenter.S2_PRT.c_str())-1);
		strcat(response,"S2_PRT:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server2port=%s",datacenter.S2_PRT.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;
	}

	f1 =data.find("S3_IP:");
	if(f1>0)
	{
		datacenter.S3_IP=data.substr(f1+6);
		datacenter.S3_IP=datacenter.S3_IP.substr(0, strlen(datacenter.S3_IP.c_str())-1);
		strcat(response,"S3_IP:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server3address=%s",datacenter.S3_IP.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;
	}


	f1 =data.find("S3_PRT:");
	if(f1>0)
	{
		datacenter.S3_PRT=data.substr(f1+7);
		datacenter.S3_PRT=datacenter.S3_PRT.substr(0, strlen(datacenter.S3_PRT.c_str())-1);
		strcat(response,"S3_PRT:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.server3port=%s",datacenter.S3_PRT.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;
	}


	f1 =data.find("L_PRT:");
	if(f1>0)
	{
		datacenter.L_PRT=data.substr(f1+6);
		datacenter.L_PRT=datacenter.L_PRT.substr(0, strlen(datacenter.L_PRT.c_str())-1);
		strcat(response,"L_PRT:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set communication.datacenter.listenport=%s",datacenter.L_PRT.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=2;
	}


	f1 = data.find("LAN_IP?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.ipaddr");
		exec(str,str1, 10);
		lan.LAN_IP = str1;
		sprintf(response,"LAN_IP:%s",lan.LAN_IP.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("LAN_GW?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.gateway");
		exec(str,str1, 10);
		lan.LAN_GW = str1;
		sprintf(response,"LAN_GW:%s",lan.LAN_GW.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("LAN_NM?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get network.lan.netmask");
		exec(str,str1, 10);
		lan.LAN_NM = str1;
		sprintf(response,"LAN_NM:%s",lan.LAN_NM.c_str());
//		printf("%s\n",response);

	}

	f1 =data.find("LAN_IP:");
	if(f1>0)
	{
		lan.LAN_IP=data.substr(f1+7);
		lan.LAN_IP=lan.LAN_IP.substr(0, strlen(lan.LAN_IP.c_str())-1);
		strcat(response,"LAN_IP:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set network.lan.ipaddr=%s",lan.LAN_IP.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=3;

	}

	f1 =data.find("LAN_GW:");
	if(f1>0)
	{
		lan.LAN_GW=data.substr(f1+7);
		lan.LAN_GW=lan.LAN_GW.substr(0, strlen(lan.LAN_GW.c_str())-1);
		strcat(response,"LAN_GW:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set network.lan.gateway=%s",lan.LAN_GW.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=3;

	}


	f1 =data.find("LAN_NM:");
	if(f1>0)
	{
		lan.LAN_NM=data.substr(f1+7);
		lan.LAN_NM=lan.LAN_NM.substr(0, strlen(lan.LAN_NM.c_str())-1);
		strcat(response,"LAN_NM:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set network.lan.netmask=%s",lan.LAN_NM.c_str());
		exec2(str);
//		printf("%s\n",str);
		Need_Init=3;

	}

	f1 = data.find("PH1?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number1.number");
		exec(str,str1, 10);
		user.PH1 = str1;
		sprintf(response,"PH1:%s",user.PH1.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH2?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number2.number");
		exec(str,str1, 10);
		user.PH2 = str1;
		sprintf(response,"PH2:%s",user.PH2.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH3?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number3.number");
		exec(str,str1, 10);
		user.PH3 = str1;
		sprintf(response,"PH3:%s",user.PH3.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH4?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number4.number");
		exec(str,str1, 10);
		user.PH4 = str1;
		sprintf(response,"PH4:%s",user.PH4.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH5?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number5.number");
		exec(str,str1, 10);
		user.PH5 = str1;
		sprintf(response,"PH5:%s",user.PH5.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH1EN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number1.enable");
		exec(str,str1, 10);
		user.PH1EN = str1;
		sprintf(response,"PH1EN:%s",user.PH1EN.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH2EN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number2.enable");
		exec(str,str1, 10);
		user.PH2EN = str1;
		sprintf(response,"PH2EN:%s",user.PH2EN.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH3EN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number3.enable");
		exec(str,str1, 10);
		user.PH3EN = str1;
		sprintf(response,"PH3EN:%s",user.PH3EN.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH4EN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number4.enable");
		exec(str,str1, 10);
		user.PH4EN = str1;
		sprintf(response,"PH4EN:%s",user.PH4EN.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("PH5EN?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get sms.number5.enable");
		exec(str,str1, 10);
		user.PH5EN = str1;
		sprintf(response,"PH5EN:%s",user.PH5EN.c_str());
//		printf("%s\n",response);

	}
	f1 =data.find("PH1:");
	if(f1>0)
	{
		user.PH1=data.substr(f1+4);
		user.PH1=user.PH1.substr(0, strlen(user.PH1.c_str())-1);
		strcat(response,"PH1:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number1.number=%s",user.PH1.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("PH2:");
	if(f1>0)
	{
		user.PH2=data.substr(f1+4);
		user.PH2=user.PH2.substr(0, strlen(user.PH2.c_str())-1);
		strcat(response,"PH2:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number2.number=%s",user.PH2.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("PH3:");
	if(f1>0)
	{
		user.PH3=data.substr(f1+4);
		user.PH3=user.PH3.substr(0, strlen(user.PH3.c_str())-1);
		strcat(response,"PH3:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number3.number=%s",user.PH3.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("PH4:");
	if(f1>0)
	{
		user.PH4=data.substr(f1+4);
		user.PH4=user.PH4.substr(0, strlen(user.PH4.c_str())-1);
		strcat(response,"PH4:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number4.number=%s",user.PH4.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("PH5:");
	if(f1>0)
	{
		user.PH5=data.substr(f1+4);
		user.PH5=user.PH5.substr(0, strlen(user.PH5.c_str())-1);
		strcat(response,"PH5:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number5.number=%s",user.PH5.c_str());
		exec2(str);
//		printf("%s\n",str);
	}



	f1 =data.find("PH1EN:");
	if(f1>0)
	{
		user.PH1EN=data.substr(f1+6);
		user.PH1EN=user.PH1EN.substr(0, strlen(user.PH1EN.c_str())-1);
		strcat(response,"PH1EN:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number1.enable=%s",user.PH1EN.c_str());
		exec2(str);
//		printf("%s\n",str);
	}

	f1 =data.find("PH2EN:");
	if(f1>0)
	{
		user.PH2EN=data.substr(f1+6);
		user.PH2EN=user.PH2EN.substr(0, strlen(user.PH2EN.c_str())-1);
		strcat(response,"PH2EN:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number2.enable=%s",user.PH2EN.c_str());
		exec2(str);
//		printf("%s\n",str);
	}

	f1 =data.find("PH3EN:");
	if(f1>0)
	{
		user.PH3EN=data.substr(f1+6);
		user.PH3EN=user.PH3EN.substr(0, strlen(user.PH3EN.c_str())-1);
		strcat(response,"PH3EN:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number3.enable=%s",user.PH3EN.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 =data.find("PH4EN:");
	if(f1>0)
	{
		user.PH4EN=data.substr(f1+6);
		user.PH4EN=user.PH4EN.substr(0, strlen(user.PH4EN.c_str())-1);
		strcat(response,"PH4EN:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number4.enable=%s",user.PH4EN.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 =data.find("PH5EN:");
	if(f1>0)
	{
		user.PH5EN=data.substr(f1+6);
		user.PH5EN=user.PH5EN.substr(0, strlen(user.PH5EN.c_str())-1);
		strcat(response,"PH5EN:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set sms.number5.enable=%s",user.PH5EN.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 = data.find("OUT1?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.output.relay1");
		exec(str,str1, 10);
		io.OUT1 = str1;
		sprintf(response,"OUT1:%s",io.OUT1.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("OUT2?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.output.relay2");
		exec(str,str1, 10);
		io.OUT2 = str1;
		sprintf(response,"OUT2:%s",io.OUT2.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("IN1?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input1");
		exec(str,str1, 10);
		io.IN1 = str1;
		sprintf(response,"IN1:%s",io.IN1.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("IN2?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input2");
		exec(str,str1, 10);
		io.IN2 = str1;
		sprintf(response,"IN2:%s",io.IN2.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("IN3?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input3");
		exec(str,str1, 10);
		io.IN3 = str1;
		sprintf(response,"IN3:%s",io.IN3.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("IN4?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get inout.input.input4");
		exec(str,str1, 10);
		io.IN4 = str1;
		sprintf(response,"IN4:%s",io.IN4.c_str());
//		printf("%s\n",response);

	}

	f1 =data.find("OUT1:");
	if(f1>0)
	{

		io.OUT1=data.substr(f1+5);
		io.OUT1=io.OUT1.substr(0, strlen(io.OUT1.c_str())-1);
		strcat(response,"OUT1:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set inout.output.relay1=%s",io.OUT1.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 =data.find("OUT2:");
	if(f1>0)
	{
		io.OUT2=data.substr(f1+5);
		io.OUT2=io.OUT2.substr(0, strlen(io.OUT2.c_str())-1);
		strcat(response,"OUT2:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set inout.output.relay2=%s",io.OUT2.c_str());
		exec2(str);
//		printf("%s\n",str);

	}
	f1 = data.find("FTP_ADD?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.address");
		exec(str,str1, 10);
		ftp.FTP_ADD = str1;
		sprintf(response,"FTP_ADD:%s",ftp.FTP_ADD.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("FTP_PRT?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.port");
		exec(str,str1, 10);
		ftp.FTP_PRT = str1;
		sprintf(response,"FTP_PRT:%s",ftp.FTP_PRT.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("FTP_USR?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.user");
		exec(str,str1, 10);
		ftp.FTP_USR = str1;
		sprintf(response,"FTP_USR:%s",ftp.FTP_USR.c_str());
//		printf("%s\n",response);

	}
	f1 = data.find("FTP_PAS?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.pass");
		exec(str,str1, 10);
		ftp.FTP_PAS = str1;
		sprintf(response,"FTP_PAS:%s",ftp.FTP_PAS.c_str());
//		printf("%s\n",response);

	}


	f1 = data.find("FTP_PATH?");
	if(f1>0)
	{
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		sprintf(str,"uci get ftp.parameter.path");
		exec(str,str1, 1000);
		ftp.FTP_PATH = str1;
		sprintf(response,"FTP_PATH:%s",ftp.FTP_PATH.c_str());
//		printf("%s\n",response);

	}

	f1 =data.find("FTP_ADD:");
	if(f1>0)
	{
		ftp.FTP_ADD=data.substr(f1+8);
		ftp.FTP_ADD=ftp.FTP_ADD.substr(0, strlen(ftp.FTP_ADD.c_str())-1);
		strcat(response,"FTP_ADD:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.parameter.address=%s",ftp.FTP_ADD.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("FTP_PRT:");
	if(f1>0)
	{
		ftp.FTP_PRT=data.substr(f1+8);
		ftp.FTP_PRT=ftp.FTP_PRT.substr(0, strlen(ftp.FTP_PRT.c_str())-1);
		strcat(response,"FTP_PRT:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.parameter.port=%s",ftp.FTP_PRT.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 =data.find("FTP_USR:");
	if(f1>0)
	{
		ftp.FTP_USR=data.substr(f1+8);
		ftp.FTP_USR=ftp.FTP_USR.substr(0, strlen(ftp.FTP_USR.c_str())-1);
		strcat(response,"FTP_USR:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.parameter.user=%s",ftp.FTP_USR.c_str());
		exec2(str);
//		printf("%s\n",str);

	}


	f1 =data.find("FTP_PAS:");
	if(f1>0)
	{
		ftp.FTP_PAS=data.substr(f1+8);
		ftp.FTP_PAS=ftp.FTP_PAS.substr(0, strlen(ftp.FTP_PAS.c_str())-1);
		strcat(response,"FTP_PAS:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.parameter.pass=%s",ftp.FTP_PAS.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	f1 =data.find("FTP_PATH:");
	if(f1>0)
	{
		ftp.FTP_PATH=data.substr(f1+9);
		ftp.FTP_PATH=ftp.FTP_PATH.substr(0, strlen(ftp.FTP_PATH.c_str())-1);
		strcat(response,"FTP_PATH:OK\n");
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.parameter.path=%s",ftp.FTP_PATH.c_str());
		exec2(str);
//		printf("%s\n",str);

	}

	msleep(500);
	exec2(UCI_COMMIT);
	return Need_Init;
}


void SMS_Send(string Number,char *text,ECXX *module)
{
	char str1[ECXX_BUFFER_SIZE];

	//printf(" txt=%s , number= %s\n");


	ECXX_Send_AT(module, AT_TEXT_MODE, str1, 1000);



	msleep(500);
	ECXX_Send_AT(module, AT_SEND_MODE, str1, 1000);


	char tel[100];
	memset(tel,0,sizeof(tel));
	sprintf(tel,"AT+CMGS=\"%s\"\r\n",Number.c_str());
	char txt[500];
	memset(txt,0,sizeof(txt));
	sprintf(txt,"%s%c",text,26);


	msleep(500);
	ECXX_Send_AT(module, tel, str1, 1000);


	msleep(500);
	ECXX_Send_AT(module, txt, str1, 1000);


}


void Update(string number)
{
	char str[300] , str1[300] , FR[100] , response[200];
	int f1,f2,f3,f4,f5=0;
	string res;
	memset(response,0,sizeof(response));
	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.parameter.address");
	exec(str,str1, 10);
	ftp.FTP_ADD = str1;
	ftp.FTP_ADD=ftp.FTP_ADD.substr(0, strlen(ftp.FTP_ADD.c_str())-1);


	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.parameter.port");
	exec(str,str1, 10);
	ftp.FTP_PRT = str1;
	ftp.FTP_PRT=ftp.FTP_PRT.substr(0, strlen(ftp.FTP_PRT.c_str())-1);



	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.parameter.user");
	exec(str,str1, 10);
	ftp.FTP_USR = str1;
	ftp.FTP_USR=ftp.FTP_USR.substr(0, strlen(ftp.FTP_USR.c_str())-1);


	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.parameter.pass");
	exec(str,str1, 10);
	ftp.FTP_PAS = str1;
	ftp.FTP_PAS=ftp.FTP_PAS.substr(0, strlen(ftp.FTP_PAS.c_str())-1);



	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.parameter.path");
	exec(str,str1, 10);
	ftp.FTP_PATH = str1;
	ftp.FTP_PATH=ftp.FTP_PATH.substr(0, strlen(ftp.FTP_PATH.c_str())-1);


	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"wget -o report.txt -t 1 -T 10 -O fm.bin ftp://%s:%s@%s%s\0",ftp.FTP_USR.c_str(),ftp.FTP_PAS.c_str(),ftp.FTP_ADD.c_str(),ftp.FTP_PATH.c_str());
	exec2(str);

	memset(str,0,sizeof(str));
	sprintf(str,"cat report.txt |grep \"Giving up\"");
	exec(str,str1,10);
	res=str1;
	f1 =res.find("Giving up");

	memset(str,0,sizeof(str));
	sprintf(str,"cat report.txt |grep \"No incorrect\"");
	exec(str,str1,10);
	res=str1;
	f2 =res.find("Login incorrect");

	memset(str,0,sizeof(str));
	sprintf(str,"cat report.txt |grep \"No such file\"");
	exec(str,str1,10);
	res=str1;
	f3 =res.find("No such file");

	memset(str,0,sizeof(str));
	sprintf(str,"cat report.txt |grep \"saved\"");
	exec(str,str1,10);
	res=str1;
	f4 =res.find("saved");


	if(f1>0)
	{
//		printf("Connection Error\n");
		sprintf(response,"Update server connection Error\n");
		SMS_Send(number,response,&modem);
	}else if(f2>0)
	{
//		printf("Login Error\n");
		sprintf(response,"Update server login Error\n");
		SMS_Send(number,response,&modem);

	}
	else if(f3>0)
	{
//		printf("File Error\n");
		sprintf(response,"Firmware file download Error\n");
		SMS_Send(number,response,&modem);
	}
	else if(f4>0)
	{
//		printf("Download Ok\n");
		sprintf(response,"Firmware download Ok - Upgrade Started\n");
		SMS_Send(number,response,&modem);
		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.status.start=1");
		exec2(str);


		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.status.number=%s",number.c_str());
		exec2(str);


		exec2(UCI_COMMIT);
		memset(str,0,sizeof(str));
		sprintf(str,"sysupgrade fm.bin");
		exec2(str);
		sleep(10);


		memset(response,0,sizeof(response));
		sprintf(response,"Upgrade proccess Error");
		SMS_Send(number,response,&modem);



		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.status.start=0");
		exec2(str);

	}
	else
	{
//		printf("Process Error\n");
		sprintf(response,"Update process Error\n");
		SMS_Send(number,response,&modem);
	}

}


void Update_Check(void)
{
	char str[100] , str1[100];
	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	sprintf(str,"uci get ftp.status.start");
	exec(str,str1, 10);
	if(str1[0]=='1')
	{
		char update_state[100];
		memset(str,0,sizeof(str));
		memset(str1,0,sizeof(str1));
		memset(update_state,0,sizeof(update_state));
		sprintf(str,"uci get ftp.status.number");
		exec(str,str1, 10);
		string num = str1 ;
		num=num.substr(0, strlen(num.c_str())-1);
//		printf("num is %s",num.c_str());
		sprintf(update_state,"Update finished successfully\nNew firmware is running");
		SMS_Send(num,update_state,&modem);

		memset(str,0,sizeof(str));
		sprintf(str,"uci set ftp.status.start=0");
		exec2(str);
		exec2(UCI_COMMIT);
	}
}

