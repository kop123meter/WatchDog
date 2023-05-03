/*
 * newservo.h
 *
 * Created: 2023/4/28 17:15:45
 *  Author: lize
 */ 

#include <asf.h>

#ifndef NEWSERVO_H_
#define NEWSERVO_H_


void configure_port_servo_pins(void);
void lock();
void unlock();
bool getLock();



#endif /* NEWSERVO_H_ */