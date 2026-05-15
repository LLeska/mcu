#include "adc-task.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "stdio.h"

const uint ADC_GPIO_PIN = 26;
const uint ADC_VOLTAGE_CHANNEL = 0;
const uint ADC_TEMP_CHANNEL = 4;
uint64_t ADC_TASK_MEAS_PERIOD_US = 100000;

uint64_t adc_ts;
adc_task_state_t adc_state;

void adc_task_init() {
	adc_ts = 0;
	adc_state = ADC_TASK_STATE_IDLE;
	adc_init();
	adc_gpio_init(ADC_GPIO_PIN);
	adc_set_temp_sensor_enabled(true);
}

float adc_task_counts_to_voltage(uint16_t adc_counts)
{
	return adc_counts * 3.3f / 4095.0f;
}

float adc_task_get_voltage()
{
	adc_select_input(ADC_VOLTAGE_CHANNEL);
	uint16_t voltage_counts = adc_read();
	float voltage_V = adc_task_counts_to_voltage(voltage_counts);
	return voltage_V;
}

float adc_task_get_temp()
{
	adc_select_input(ADC_TEMP_CHANNEL);
	uint16_t temp_counts = adc_read();
	float temp_V = adc_task_counts_to_voltage(temp_counts);
	float temp_C = 27.0f - (temp_V - 0.706f) / 0.001721f;
	return temp_C;
}

void adc_task_handle()
{
	switch (adc_state)
	{
	case ADC_TASK_STATE_IDLE:
		break;
	case ADC_TASK_STATE_RUN:
		if (time_us_64() > adc_ts)
		{
			adc_ts = time_us_64() + ADC_TASK_MEAS_PERIOD_US;
			float voltage_V = adc_task_get_voltage();
			float temp_C = adc_task_get_temp();
			printf("%f %f\n", voltage_V, temp_C);
		}
		break;
	default:
		break;
	}
}

void adc_task_set_state(adc_task_state_t state)
{
	adc_state = state;
}
