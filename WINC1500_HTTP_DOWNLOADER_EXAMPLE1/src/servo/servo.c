/*
 * servo.c
 *
 * Created: 4/20/2023 5:25:12 PM
 *  Author: haop
 */ 
#include "servo.h"

struct tcc_module tcc_instance;

void configure_tcc(void){
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV4;
	config_tcc.counter.period = 0xBA6E;
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[CONF_PWM_CHANNEL] = (0xBA6E / 4);
	config_tcc.pins.enable_wave_out_pin[CONF_PWM_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[CONF_PWM_OUTPUT]        = CONF_PWM_OUT_PIN;
	config_tcc.pins.wave_out_pin_mux[CONF_PWM_OUTPUT]    = CONF_PWM_OUT_MUX;
	//tcc_set_double_buffer_top_values(&tcc_instance,2,0x80,0x40);
	//tcc_enable_circular_buffer_compare(&tcc_instance,2);
	tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
}

void setPulse(float D){
	float compare = 0xBA6E / D;
	tcc_set_compare_value(&tcc_instance, 2, compare);
}