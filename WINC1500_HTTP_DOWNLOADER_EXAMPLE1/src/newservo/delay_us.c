/*
 * delay_us.c
 *
 * Created: 2023/4/28 18:07:57
 *  Author: lize
 */ 
#include <stdint.h>
#include "delay_us.h"


// TC module instance
static struct tc_module tc_instance;


int8_t timerCounterInit(void)
{
	struct tc_config tc_config1;
	tc_get_config_defaults(&tc_config1);
	tc_config1.counter_size = TC_COUNTER_SIZE_32BIT;
	tc_config1.clock_source = CLOCK_SOURCE;
	// GCLK4 sources clock from main clock at 8 MHz
	// DIV8 will give an 1 us counter
	tc_config1.clock_prescaler = TC_CLOCK_PRESCALER_DIV8;
	int8_t res = tc_init(&tc_instance, TC_HW, &tc_config1);
	tc_enable(&tc_instance);
	return res;
}

uint32_t tcGetCountValue(void)
{
	return tc_get_count_value(&tc_instance);
}

uint32_t tcFastGetCountValue(void)
{
	return TC_HW->COUNT32.COUNT.reg;
}

void tcFastSetCountValue(uint32_t count)
{
	/* Wait for sync before writing to registers */
	while (tc_is_syncing(&tc_instance));
	TC_HW->COUNT32.COUNT.reg = count;
}

void tcMicrosecondDelay(uint32_t delay)
{
	// Check if TC won't reach top during delay
	// If so, reset it to avoid delays shorter than expected
	if ((TC_32BIT_TOP_VALUE - tcFastGetCountValue()) < delay)
	tcFastSetCountValue(0);
	uint32_t currentTime = tcFastGetCountValue();
	while (tcFastGetCountValue() < (currentTime + delay));
}

void tcMicrosecondDelayUntil(uint32_t *lastWakeTime, uint32_t delay)
{
	uint32_t timePassed = tcFastGetCountValue() - (*lastWakeTime);
	// Check if there's still time to delay
	if (timePassed < delay)
	tcMicrosecondDelay(delay - timePassed);
	// Update lastWakeTime
	*lastWakeTime = tcFastGetCountValue();
}