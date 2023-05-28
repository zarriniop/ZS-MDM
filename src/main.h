/*
 * main.h
 *
 *  Created on: Aug 6, 2021
 *      Author: mhn
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <iostream>
#include <string>
#include <sstream>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include "tools.h"

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>  /* for sockaddr_in */
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <chrono>
#include <poll.h>
#include <stdarg.h>


 #include <sys/mman.h>


using namespace std;


#define		DEBUG_MODE					1

#define 	PACKET_EMPTY_CHAR			0x00
#define		BUFFER_SIZE					8192
#define		PACKET_SIZE					16
#define		MAX_CLIENT_NUMBER			5
#define		MAX_SERVER_NUMBER			3


#undef ra_inl
#undef ra_outl
#define ra_inl(addr)  (*(volatile uint32_t *)(addr))
#define ra_outl(addr, value)  (*(volatile uint32_t *)(addr) = (value))
#define ra_and(addr, value) ra_outl(addr, (ra_inl(addr) & (value)))
#define ra_or(addr, value) ra_outl(addr, (ra_inl(addr) | (value)))




#define GPIO_SYSCTL	0x10000000
#define GPIO2_MODE	0x00000064
#define GPIO1_MODE	0x00000060






#define		GPIO_BASE			480
#define		GPIO_IN1			16
#define		GPIO_IN2			17
#define		GPIO_IN3			18
#define		GPIO_IN4			11

#define		GPIO_LED_DATA3		0
#define		GPIO_LED_DATA2		1
#define		GPIO_LED_DATA1		2
#define		GPIO_LED_NET		3
#define		GPIO_LED_STATUS		4
#define		GPIO_OUT_RELAY2		5
#define		GPIO_OUT_RELAY1		6
#define		GPIO_OUT_GSMRST		14
#define		GPIO_OUT_GSMPWR		15
#define		GPIO_OUT_DIR		19


#define 	GPIO_REG		0x10000600
#define 	GPIO_DIR_0		0X00000000
#define 	GPIO_DIR_1		0X00000604
#define 	GPIO_DIR_2		0X00000608
#define 	GPIO_DATA_0		0X00000620
#define 	GPIO_DATA_1		0X00000624
#define	 	GPIO_DATA_2		0X00000628
#define 	GPIO_POL_0		0X00000610
#define 	GPIO_POL_1		0X00000614
#define 	GPIO_POL_2		0X00000618

#define 	GPIO_DSET_0		0X00000630
#define 	GPIO_DSET_1		0X00000634
#define 	GPIO_DSET_2		0X00000638
#define 	GPIO_DCLR_0		0X00000640
#define 	GPIO_DCLR_1		0X00000644
#define 	GPIO_DCLR_2		0X00000648


#define		UCI_GET_MODE			"uci get communication.datacenter.mode"
#define		UCI_GET_PROTOCOL		"uci get communication.datacenter.protocol"
#define		UCI_GET_ADDRESS1		"uci get communication.datacenter.server1address"
#define		UCI_GET_ADDRESS2		"uci get communication.datacenter.server2address"
#define		UCI_GET_ADDRESS3		"uci get communication.datacenter.server3address"
#define		UCI_GET_PORT1			"uci get communication.datacenter.server1port"
#define		UCI_GET_PORT2			"uci get communication.datacenter.server2port"
#define		UCI_GET_PORT3			"uci get communication.datacenter.server3port"
#define		UCI_GET_LISTENPORT		"uci get communication.datacenter.listenport"
#define		UCI_GET_KEEPALIVE		"uci get communication.datacenter.keepalive"
#define		UCI_GET_AES				"uci get communication.aes128.enable"
#define		UCI_GET_AESKEY			"uci get communication.aes128.key"



typedef enum {
	OK			=0,
	ERROR		=-1,
	NOT_ALLOW	=-2

}result;




typedef enum {
	BAUD0			=B0,
	BAUD50 			=B50,
	BAUD75 			=B75,
	BAUD110 		=B110,
	BAUD134 		=B134,
	BAUD150 		=B150,
	BAUD200 		=B200,
	BAUD300 		=B300,
	BAUD600 		=B600,
	BAUD1200 		=B1200,
	BAUD1800	 	=B1800,
	BAUD2400 		=B2400,
	BAUD4800 		=B4800,
	BAUD9600 		=B9600,
	BAUD19200 		=B19200,
	BAUD38400 		=B38400,
	BAUD57600 		=B57600,
	BAUD115200 		=B115200,
	BAUD230400 		=B230400,
	BAUD460800 		=B460800,

}Baud;


typedef enum {
	Bits5		=CS5,
	Bits6		=CS6,
	Bits7		=CS7,
	Bits8		=CS8,
}Bits;



typedef enum {
	PARITY_NONE			=1,
	PARITY_YES			=2,
}Pari;


typedef enum {
	STOPBITS_ONE		=1,
	STOPBITS_TWO		=2,
}StpBits;


typedef struct {
	char		*File;
	Baud		BaudRate;
	Bits		bitsPbyte;
	Pari		Parity;
	StpBits		StopBits;
}PortParameters;

typedef struct {
	int				Port_fd;
	PortParameters	Parameters;
	uint16_t		TX_Count;
	uint16_t		RX_Count;
	uint8_t 		TX_Buffer[BUFFER_SIZE];
	uint8_t 		RX_Buffer[BUFFER_SIZE];
	long			Timeout;
}SerialPort;



#endif /* MAIN_H_ */
