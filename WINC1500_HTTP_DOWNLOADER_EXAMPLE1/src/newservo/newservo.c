/*
 * newservo.c
 *
 * Created: 2023/4/28 17:16:01
 *  Author: lize
 */ 
#include "port.h"
#include "newservo.h"



#define SERVO_PIN PIN_PA03
bool LOCKER_STATUS_LOCKED = true;
bool LOCKER_STATUS_UNLOCKED = false;
bool locker_status = true;
void configure_port_servo_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(SERVO_PIN, &config_port_pin);
	
}

void lock()
{
	int count = 0;
	port_pin_set_output_level(SERVO_PIN,0);
    for(int i = 0;i<1000;i++){
		if(count == 8)
		{
			port_pin_set_output_level(SERVO_PIN,1);
			count = 0;
			
		}
		else{
			port_pin_set_output_level(SERVO_PIN,0);
			
		}
		//MG90S
		vTaskDelay(1);
		//A0090
		//vTaskDelay(2.4);
		//delay_ms(1);
		count++;
	}
	port_pin_set_output_level(SERVO_PIN,0);
	locker_status = LOCKER_STATUS_LOCKED;
}

void unlock(){
	int count = 0;
	port_pin_set_output_level(SERVO_PIN,0);
	 for(int i = 0;i<300;i++){
		if(count == 4)
		{
			port_pin_set_output_level(SERVO_PIN,1);
			count = 0;
		}
		else{
			port_pin_set_output_level(SERVO_PIN,0);
		}
		//MG90S
		vTaskDelay(9);
		//A0090
		//vTaskDelay(3.8);
		//delay_ms(2);
		count++;
	}
	port_pin_set_output_level(SERVO_PIN,0);
	locker_status = LOCKER_STATUS_UNLOCKED;
}
bool getLock(){
	return locker_status;
}
void setLock(){
	locker_status = LOCKER_STATUS_LOCKED;
}
void setUnLock(){
	locker_status = LOCKER_STATUS_UNLOCKED;
}