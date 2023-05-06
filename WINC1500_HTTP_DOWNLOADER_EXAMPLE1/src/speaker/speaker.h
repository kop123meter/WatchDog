/*
 * speaker.h
 *
 * Created: 2023/5/3 13:13:27
 *  Author: HEY
 */ 

#include <asf.h>
#ifndef SPEAKER_H_
#define SPEAKER_H_

#define SPEAKER_PIN PIN_PA03
#define WARNING_FREQUENCY 5
#define MAX_PERIOD 20
#define MIN_PERIOD 0.2
void configure_port_speaker_pins(void);
void warning_speaker();
void noTone();
void deinit(void);



#endif /* SPEAKER_H_ */