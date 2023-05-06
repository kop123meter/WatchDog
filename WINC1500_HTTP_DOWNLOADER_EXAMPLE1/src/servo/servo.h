/*
 * servo.h
 *
 * Created: 4/20/2023 5:23:09 PM
 *  Author: haop
 */ 
#include "tcc.h"
#include "asf.h"

#ifndef SERVO_H_
#define SERVO_H_


#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     2
#define CONF_PWM_OUTPUT      2
#define CONF_PWM_OUT_PIN     PIN_PA10F_TCC0_WO2 
#define CONF_PWM_OUT_MUX     MUX_PA10F_TCC0_WO2


void configure_tcc(void);
void setPulse(float D);

#endif /* SERVO_H_ */
