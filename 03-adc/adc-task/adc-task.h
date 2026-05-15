#pragma once

typedef enum
{
	ADC_TASK_STATE_IDLE = 0,
	ADC_TASK_STATE_RUN = 1,
} adc_task_state_t;

void adc_task_init();
float adc_task_get_voltage();
float adc_task_get_temp();
void adc_task_handle();
void adc_task_set_state(adc_task_state_t state);
