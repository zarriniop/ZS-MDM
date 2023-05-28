/*
 * tools.h
 *
 *  Created on: Nov 13, 2021
 *      Author: mhn
 */
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>


#ifndef TOOLS_H_
#define TOOLS_H_


void exec(char* cmd,char *resultt,uint32_t time);
void exec2(const char* cmd);
void delay(int t);
void msleep(uint32_t delay);
char get_communication_mode();
const char * get_time(void);
void watchdog();
void AppStartPrint();
long diff_time_us(struct timeval *start);
long diff_time_sec(struct timeval *start);
void report(char *format, ... );

class Timer {
	std::atomic<bool> active{true};

    public:
        void setTimeout(auto function, int delay)
        {
        	active = true;
			std::thread t([=]() {
				if(!active.load()) return;
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				if(!active.load()) return;
				function();
			});
			t.detach();
        }
        void setInterval(auto function, int interval)
        {
        	active = true;
			std::thread t([=]() {
				while(active.load()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					if(!active.load()) return;
					function();
				}
			});
			t.detach();
        }
        void stop()
        {
        	active = false;
        }

};






#endif /* TOOLS_H_ */
