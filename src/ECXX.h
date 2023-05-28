/*
 * rs485.h
 *
 *  Created on: Aug 6, 2021
 *      Author: mhn
 */

#ifndef LIB_ECXX_H_
#define LIB_ECXX_H_

//#include "gpio.h"
#include "main.h"
#include "tools.h"
#include "gpio.h"

#define		EC25_SERIAL_FILE 		"/dev/ttyUSB2\0"
#define		EC200U_SERIAL_FILE 		"/dev/ttyUSB6\0"
#define		EC200U_SERIAL_FILE2		"/dev/ttyUSB7\0"
#define		EC25_FILE_CHECK			"ls /dev/cdc-wdm0\0"
#define		EC200U_FILE_CHECK		"ls /dev/cdc-wdm0\0"

#define 	ECXX_BUFFER_SIZE		1024

#define		EC25_NET_RESET			"/etc/init.d/network restart\0"
#define		AT_AT					"AT\r\n\0"
#define		AT_CSQ					"AT+CSQ\r\n\0"
#define		AT_IMEI					"AT+GSN\r\n\0"
#define		AT_CREG					"AT+CEREG?\r\n\0"

#define		AT_SERV_CELL			"at+qeng=\"servingcell\"\r\n\0"
#define		AT_GET_IMSI				"at+cimi\r\n\0"
#define		AT_GET_CGATT			"AT+CGATT?\r\n\0"
#define		AT_GET_CGREG			"AT+CGREG?\r\n\0"
#define		AT_GET_IMEI				"at+gsn\r\n\0"
#define		AT_PIN_PRESENT			"at+qsimdet=1,1\r\n\0"
#define		AT_CPIN					"at+cpin?\r\n\0"
#define		AT_NETW_STS				"at+qnwinfo\r\n\0"
#define		AT_ICCID				"at+qccid\r\n\0"
#define		AT_ROAM_DISABLE			"AT+QCFG=\"roamservice\",1,1 \r\n\0"
#define		AT_ROAM_ENABLE			"AT+QCFG=\"roamservice\",2,1 \r\n\0"
#define		AT_TECH_LOCK_GSM		"AT+QCFG=\"nwscanmode\",1\r\n\0"
#define		AT_TECH_LOCK_WCDMA		"AT+QCFG=\"nwscanmode\",2\r\n\0"
#define		AT_TECH_LOCK_UMTS		"AT+QCFG=\"nwscanmode\",5\r\n\0"
#define		AT_TECH_LOCK_LTE		"AT+QCFG=\"nwscanmode\",3\r\n\0"
#define		AT_TECH_LOCK_ALL		"AT+QCFG=\"nwscanmode\",0\r\n\0"
#define		AT_BAND_LOCK			"AT+QCFG=\"band\"\0"
#define		AT_SELECT_SIM1			"at+qdsim=1\r\n\0"
#define		AT_SELECT_SIM0			"at+qdsim=0\r\n\0"
#define		AT_SET_APN				"at+qicsgp=1,1,\0"
#define		AT_ACTIVE_PDP			"AT+QIACT=1\r\n\0"
#define		AT_GET_PDP_STATUS		"AT+QIACT?\r\n\0"
#define		AT_DEACTIVE_PDP			"AT+QIDEACT=1\r\n\0"
#define		AT_ACTIVE_DUALSIM		"AT+QDSIMCFG=\"dsss\",1\r\n\0"
#define		AT_SETUP_DATACALL3		"AT+QNETDEVCTL=3,1,1\r\nv"
#define		AT_SETUP_DATACALL0		"AT+QNETDEVCTL=0\r\n\0"
#define		AT_SET_SMS_TXT  		"AT+CMGF=1\r\n\0"
#define		AT_DEL_SMS_ALL	  		"AT+CMGD=1,4\r\n\0"
#define		AT_READ_SMS_0	  		"AT+CMGR=0\r\n\0"
#define     AT_TEXT_MODE			"AT+CMGF=1\r\n\0"
#define     AT_SEND_MODE            "AT+CSCS=\"GSM\"\r\n\0"



#define		UCI_GET_TECHLOCK		"uci get modem.parameter.technology"
#define		UCI_GET_ROAMLOCK		"uci get modem.parameter.roaming"
#define		UCI_GET_LTELOCK_ALL		"uci get modem.parameter.lteALL"
#define		UCI_GET_LTELOCK			"uci get modem.parameter.lteB"
#define		UCI_GET_UMTSLOCK_ALL	"uci get modem.parameter.umtsgsmALL"
#define		UCI_GET_UMTSLOCK		"uci get modem.parameter.umtsB"
#define		UCI_GET_GSMLOCK			"uci get modem.parameter.gsmB"

#define		UCI_GET_APN1			"uci get modem.parameter.sim1apn"
#define		UCI_GET_APN2			"uci get modem.parameter.sim2apn"
#define		UCI_GET_DEFSIM			"uci get modem.parameter.defultsimcard"
#define		UCI_GET_PIN				"uci get modem.parameter.pin"
#define		UCI_GET_SIMCHANGE		"uci get modem.parameter.simchange"
#define 	UCI_SET_CURRNETSIM1		"uci set modem.usim.currentsim=1"
#define 	UCI_SET_CURRNETSIM2		"uci set modem.usim.currentsim=2"

#define		UCI_SET_SIGNALICON0		"uci set modem.icons.signal=0"
#define		UCI_SET_SIGNALICON1		"uci set modem.icons.signal=1"
#define		UCI_SET_SIGNALICON2		"uci set modem.icons.signal=2"
#define		UCI_SET_SIGNALICON3		"uci set modem.icons.signal=3"
#define		UCI_SET_SIGNALICON4		"uci set modem.icons.signal=4"
#define		UCI_SET_SIGNALICON5		"uci set modem.icons.signal=5"

#define		UCI_SET_CMU_FLAG		"uci set communication.type.flag=1"

#define		CPY_SIGNALICON0			"cp /www/luci-static/bootstrap/signal0.png /www/luci-static/bootstrap/signal.png"
#define		CPY_SIGNALICON1			"cp /www/luci-static/bootstrap/signal1.png /www/luci-static/bootstrap/signal.png"
#define		CPY_SIGNALICON2			"cp /www/luci-static/bootstrap/signal2.png /www/luci-static/bootstrap/signal.png"
#define		CPY_SIGNALICON3			"cp /www/luci-static/bootstrap/signal3.png /www/luci-static/bootstrap/signal.png"
#define		CPY_SIGNALICON4			"cp /www/luci-static/bootstrap/signal4.png /www/luci-static/bootstrap/signal.png"
#define		CPY_SIGNALICON5			"cp /www/luci-static/bootstrap/signal5.png /www/luci-static/bootstrap/signal.png"

#define		UCI_SET_SIMICON0		"uci set modem.icons.sim=0"
#define		UCI_SET_SIMICON1		"uci set modem.icons.sim=1"

#define		CPY_SIMICON0			"cp /www/luci-static/bootstrap/sim0.png /www/luci-static/bootstrap/sim.png"
#define		CPY_SIMICON1			"cp /www/luci-static/bootstrap/sim1.png /www/luci-static/bootstrap/sim.png"

#define		UCI_SET_NETICON0		"uci set modem.icons.net=0"
#define		UCI_SET_NETICON1		"uci set modem.icons.net=1"

#define		CPY_NETICON0			"cp /www/luci-static/bootstrap/net0.png /www/luci-static/bootstrap/net.png"
#define		CPY_NETICON1			"cp /www/luci-static/bootstrap/net1.png /www/luci-static/bootstrap/net.png"

#define		UCI_COMMIT_MODEM		"uci commit modem"
#define		UCI_COMMIT				"uci commit"

#define		EC25_PING_CHECK			"ping 8.8.8.8 -w 2"
#define		EC25_FILE_CHECK			"ls /dev/cdc-wdm0"

#define		USBNET0_down			"ifdown modem"
#define		USBNET0_up				"ifup modem"

#define		LAN_up					"ifup lan"
#define		LAN_down				"ifdown lan"



#define 	NO_SIG_TIMEOUT			75
#define 	NO_NET_TIMEOUT 			75
#define 	NO_SIM_TIMEOUT			3


extern	GPIO		gpio;



typedef enum {
	MODULENONE		=0,
	EC25			=1,
	EC200U			=2

}ECtype;


typedef enum {
	NOTDEFINE		=0,
	YES				=1,
	NO				=2

}StatusState;


typedef struct {

	int 		RSRP;
	int 		RSRQ;
	int 		RSSI;
	int 		SINR;

}ModemSignal;

typedef struct {

	StatusState			Ready;
	StatusState			Connected;
	StatusState 		SimReady;
	StatusState 		Registration;
	StatusState 		Roaming;
	uint8_t				signum;
	ModemSignal			signal;
	uint16_t			NoSigCount;
	uint16_t			NoNetCount;
	uint16_t			NoSimCount;
}ModemStatus;





typedef struct{       // The class

		SerialPort		PORT;
		ECtype			ModuleType;
		ModemStatus 	status;
		char			SimCard;
		uint16_t		NoSimTime;
		uint16_t		NoSigTime;
		uint16_t		NoNetTime;
		char			settingfile[2048];
}ECXX;




typedef struct{
	string APN1;
	string APN2;
	string DefSim;
	string RSSI;
	string Simcard;
	string IP_ADD;
}GPRS;


typedef struct{
	string TCP_M;
	string S1_IP;
	string S1_PRT;
	string S2_IP;
	string S2_PRT;
	string S3_IP;
	string S3_PRT;
	string L_PRT;
}DataCenter;


typedef struct{
	string LAN_IP;
	string LAN_GW;
	string LAN_NM;
}LAN;

typedef struct{
	string PH1;
	string PH2;
	string PH3;
	string PH4;
	string PH5;
	string PH1EN;
	string PH2EN;
	string PH3EN;
	string PH4EN;
	string PH5EN;
}USER;


typedef struct{
	string OUT1;
	string OUT2;
	string IN1;
	string IN2;
	string IN3;
	string IN4;
}IO;

typedef struct{
	string FTP_ADD;
	string FTP_PRT;
	string FTP_USR;
	string FTP_PAS;
	string FTP_PATH;
}FTP;


typedef struct{
	char numbers[5][100];
	char enables[5][100];
	char admins[5][100];
}ADMIN;

typedef struct{
	string		 Product_Name;
	string 		 SN;
	string 	 	 Firmware;
	GPRS  		 gprs;
	DataCenter	 datacenter;
	LAN   		 lan;
	USER   		 user;
	IO    		 io;
	FTP   		 ftp;
	ADMIN		 admin;
	string		 Update;
}HELP;

result ECXX_Port_Close(SerialPort *port);
result ECXX_Port_Open(SerialPort *port);

void ECXX_Init(ECXX *module,char reset);
void *EC25_Manager_thread(void *);
void EC200U_Manager(ECXX *module);
void ECXX_Get_atcmd_data1(ECXX *module);
void ECXX_Get_atcmd_data2(ECXX *module,uint8_t param);
void ECXX_Get_Status(ECXX *module);
result ECXX_Start(ECXX *module);
result ECXX_Stop(ECXX *module);
result ECXX_Reset(ECXX *module);
result ECXX_Power_ON(ECXX *module);
result ECXX_Power_OFF(ECXX *module);
result ECXX_Ping_Check();
result Settings_change_Check(ECXX *module);
void ECXX_Send_AT(ECXX *module,char *Request,char *Response,uint16_t timeout);

void Sim_Switch_Check(ECXX *module);
char SMS_Process(string Number,string text);
int TelNumber_Check(string Number);
void New_SMS_Check(ECXX *module);
void Read_TelNumbers(void);
char SMS_Settings_Extract(string data,char *response,string number1);
void SMS_Send(string Number,char *text,ECXX *module);
void Update(string number);
void Update_Check(void);



#endif /* LIB_RS485_H_ */
