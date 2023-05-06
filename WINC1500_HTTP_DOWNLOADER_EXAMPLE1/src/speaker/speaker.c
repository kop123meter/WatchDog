/*
 * speaker.c
 *
 * Created: 2023/5/3 13:13:37
 *  Author: HEY
 */ 
#include "speaker.h"
float flag = MAX_PERIOD;
void configure_port_speaker_pins(void)
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(SPEAKER_PIN, &config_port_pin);
	
}
void deinit(void){
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.powersave = true;
	port_pin_set_config(SPEAKER_PIN, &config_port_pin);
}
void noTone(){
	port_pin_set_output_level(SPEAKER_PIN,0);
}
void warning_speaker(){
	configure_port_speaker_pins();
	int count = 0;
	port_pin_set_output_level(SPEAKER_PIN,0);
	for(int i = 0;i<10000;i++){
		if(count == 4)
		{
			port_pin_set_output_level(SPEAKER_PIN,1);
			count = 0;
			if(flag >= MAX_PERIOD){
				flag = MIN_PERIOD;
				}	else{
				flag = 1000 / ( 1000/flag - ((1000/MIN_PERIOD - 1000/MAX_PERIOD) / 44100 ));
			}
		}
		else{
			port_pin_set_output_level(SPEAKER_PIN,0);
		}
		vTaskDelay(flag);
		//delay_ms(1);
		count++;
	}
	port_pin_set_output_level(SPEAKER_PIN,0);
	deinit();
}