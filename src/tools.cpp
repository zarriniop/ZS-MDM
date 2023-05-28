/*
 * tools.cpp
 *
 *  Created on: Nov 13, 2021
 *      Author: mhn
 */


#include "main.h"

#define		UCI_GET_INTERFACE	"uci get communication.type.type"

bool Executing=false;

void delay(int t)
{
	for (int c = 1; c <= t; c++)
	{
		for (int d = 1; d <= 32767; d++)
		{
		}
	}
}

void msleep(uint32_t delay)
{
	usleep(delay*1000);
}


void AppStartPrint()
{
	report("s","====================================");
	report("s","   LRM Modem Manager V2.4  ");
	report("s","   Build: 2022-08-25   ");
	report("s","   Company: ZarrinSamaneSharq  ");
	report("s","   Developer: M.H.Najafi  ");
	report("s","====================================");
}


//Execute a shell command and receive response
void exec(char* cmd,char *resultt,uint32_t time) {
    uint16_t cnt=0;
    clock_t clk,t=0;
    char buffer[1000],count;

	memset(buffer,0,sizeof(buffer));
	memset(resultt,0,sizeof(resultt));
	t=time*1000;
	clk=clock();

	FILE* pipe = popen(cmd, "r");
	if (!pipe) report("popen() failed!\n");
	try
	{
		while(clock()-clk<t)
		{
			if (fgets(buffer, sizeof(buffer), pipe) != NULL)
			{

				memcpy(resultt+cnt,buffer,strlen(buffer));
				cnt+=strlen(buffer);
				//memcpy(resultt,buffer,sizeof(buffer));
				count=0;
				//printf("---------------------%d\r\n",cnt);
			}
			else
			{
				count++;
				usleep(1000);
				//if(count>20 && cnt>0) break;
			}

		}

	}
	catch (...)
	{
		pclose(pipe);
		//throw;
	}
	pclose(pipe);


}

//Execute a shell command without response
void exec2(const char* cmd) {
	std::system(cmd);

}

char get_communication_mode()
{
	char str[5];
	exec(UCI_GET_INTERFACE, str, 1);
	return str[0];
}


long diff_time_us(struct timeval *start)
{
	 struct timeval  tv;
	 long diff=0;
	 gettimeofday (&tv, NULL);
	 diff=tv.tv_usec - start->tv_usec;
	 if(diff < 0) diff+=1000000;

	 return diff;
}

long diff_time_sec(struct timeval *start)
{
	 struct timeval  tv;
	 long diff=0;
	 gettimeofday (&tv, NULL);
	 diff=tv.tv_sec - start->tv_sec;

	 return diff;
}

//get system time and date and create a standard string format
const char * get_time(void) {
    static char time_buf[128];
    struct timeval  tv;
    time_t time;
    suseconds_t millitm;
    struct tm *ti;

    gettimeofday (&tv, NULL);

    time= tv.tv_sec;
    millitm = (tv.tv_usec + 500) / 1000;

    if (millitm == 1000) {
        ++time;
        millitm = 0;
    }

    ti = localtime(&time);
    sprintf(time_buf, "%02d-%02d_%02d:%02d:%02d:%03d", ti->tm_mon+1, ti->tm_mday, ti->tm_hour, ti->tm_min, ti->tm_sec, (int)millitm);
    return time_buf;
}


//log a report to system logs
void report(char *format, ... )
{
	va_list args;
	int i=0,count=0;
	char ss[100],str[400];
	va_start( args, format );

	int		d;
	float   f;
	char   *s;

	memset(str,0,sizeof(str));

	//check for input parameters and create string
	for( i = 0; format[i] != '\0'; ++i )
	{
		switch( format[i] )
		{
	         case 'd':
	        	d=va_arg(args, int );
	            sprintf(ss,"%d\0",d);
	         break;

	         case 'f':
	        	 f=va_arg(args, double );
	             sprintf(ss,"%f\0",f);
	         break;


	         case 's':
	        	 s=va_arg(args, char *);
	             sprintf(ss,"%s\0",s);
	         break;

	         default:
	         break;
	      }

			memcpy(str+count,ss,strlen(ss));
			count=strlen(str);

	}

	va_end( args );

	printf("((%s)) %s\n",get_time(),str);
	char str1[500];
	memset(str1,0,sizeof(str1));
	sprintf(str1,"logger -t \"=======(  ZS-LRM200  )=======\" \"%s\" ",str);
	std::system(str1);

}


