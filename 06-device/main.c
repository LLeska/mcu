#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "mem-task/mem-task.h"
#include "bme280-driver.h"
#include "ili9341-driver.h"
#include "ili9341-display.h"
#include "ili9341-font.h"

#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.6"

#define BME280_ADDR 0x76

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

#define MEAS_HISTORY_LEN 60*15

static ili9341_display_t ili9341_display = {0};

static uint32_t meas_period_ms = 1000/15;
static uint64_t meas_ts = 0;
static float last_temp_C = 0.0f;
static float last_pres_Pa = 0.0f;
static float last_hum_percent = 0.0f;
static float temp_history[MEAS_HISTORY_LEN] = {0};
static float pres_history[MEAS_HISTORY_LEN] = {0};
static float hum_history[MEAS_HISTORY_LEN] = {0};
static uint32_t meas_count = 0;
static bool gui_enabled = true;

void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
	i2c_read_timeout_us(i2c1, BME280_ADDR, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
	i2c_write_timeout_us(i2c1, BME280_ADDR, data, size, false, 100000);
}

void rp2040_spi_write(const uint8_t* data, uint32_t size)
{
	spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t* buffer, uint32_t length)
{
	spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level)
{
	gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level)
{
	gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level)
{
	gpio_put(ILI9341_PIN_RESET, level);
}

void rp2040_delay_ms(uint32_t ms)
{
	sleep_ms(ms);
}

static uint16_t color_by_temp(float temp_C)
{
	if (temp_C < 18.0f) {
		return COLOR_CYAN;
	}
	if (temp_C > 30.0f) {
		return COLOR_RED;
	}
	return COLOR_GREEN;
}

static uint16_t color_by_hum(float hum_percent)
{
	if (hum_percent < 30.0f) {
		return COLOR_ORANGE;
	}
	if (hum_percent > 70.0f) {
		return COLOR_CYAN;
	}
	return COLOR_GREEN;
}

static float clamp_float(float value, float min_value, float max_value)
{
	if (value < min_value) {
		return min_value;
	}
	if (value > max_value) {
		return max_value;
	}
	return value;
}

static void display_task_init()
{
	spi_init(spi0, 62500000);
	gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);
	gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(ILI9341_PIN_SCK, GPIO_FUNC_SPI);

	gpio_init(ILI9341_PIN_CS);
	gpio_init(ILI9341_PIN_DC);
	gpio_init(ILI9341_PIN_RESET);
	gpio_set_dir(ILI9341_PIN_CS, GPIO_OUT);
	gpio_set_dir(ILI9341_PIN_DC, GPIO_OUT);
	gpio_set_dir(ILI9341_PIN_RESET, GPIO_OUT);
	gpio_put(ILI9341_PIN_CS, 1);
	gpio_put(ILI9341_PIN_DC, 0);
	gpio_put(ILI9341_PIN_RESET, 0);

	ili9341_hal_t ili9341_hal = {0};
	ili9341_hal.spi_write = rp2040_spi_write;
	ili9341_hal.spi_read = rp2040_spi_read;
	ili9341_hal.gpio_cs_write = rp2040_gpio_cs_write;
	ili9341_hal.gpio_dc_write = rp2040_gpio_dc_write;
	ili9341_hal.gpio_reset_write = rp2040_gpio_reset_write;
	ili9341_hal.delay_ms = rp2040_delay_ms;

	ili9341_init(&ili9341_display, &ili9341_hal);
	ili9341_set_rotation(&ili9341_display, ILI9341_ROTATION_90);
	ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
}

static void sensor_task_init()
{
	i2c_init(i2c1, 100000);
	gpio_set_function(14, GPIO_FUNC_I2C);
	gpio_set_function(15, GPIO_FUNC_I2C);
	gpio_pull_up(14);
	gpio_pull_up(15);
	bme280_init(rp2040_i2c_read, rp2040_i2c_write);
}

static void history_push(float temp_C, float pres_Pa, float hum_percent)
{
	for (int i = 0; i < MEAS_HISTORY_LEN - 1; i++)
	{
		temp_history[i] = temp_history[i + 1];
		pres_history[i] = pres_history[i + 1];
		hum_history[i] = hum_history[i + 1];
	}
	temp_history[MEAS_HISTORY_LEN - 1] = temp_C;
	pres_history[MEAS_HISTORY_LEN - 1] = pres_Pa;
	hum_history[MEAS_HISTORY_LEN - 1] = hum_percent;
	if (meas_count < MEAS_HISTORY_LEN) {
		meas_count++;
	}
}

static void draw_bar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float value, float min_value, float max_value, uint16_t color)
{
	float normalized = (clamp_float(value, min_value, max_value) - min_value) / (max_value - min_value);
	uint16_t fill_width = (uint16_t)(normalized * width);
	ili9341_draw_filled_rect(&ili9341_display, x, y, width, height, COLOR_BLACK);
	ili9341_draw_rect(&ili9341_display, x, y, width, height, COLOR_WHITE);
	if (fill_width > 2) {
		ili9341_draw_filled_rect(&ili9341_display, x + 1, y + 1, fill_width - 2, height - 2, color);
	}
}

static void draw_graph(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float* values, float min_value, float max_value, uint16_t color)
{
	ili9341_draw_filled_rect(&ili9341_display, x, y, width, height, COLOR_BLACK);
	ili9341_draw_rect(&ili9341_display, x, y, width, height, COLOR_WHITE);
	if (meas_count < 2) {
		return;
	}

	uint32_t start = MEAS_HISTORY_LEN - meas_count;
	uint16_t prev_x = x + 1;
	uint16_t prev_y = y + height - 2;
	for (uint32_t i = 0; i < meas_count; i++)
	{
		uint32_t idx = start + i;
		float normalized = (clamp_float(values[idx], min_value, max_value) - min_value) / (max_value - min_value);
		uint16_t px = x + 1 + (uint16_t)((i * (width - 3)) / (meas_count - 1));
		uint16_t py = y + height - 2 - (uint16_t)(normalized * (height - 3));
		if (i > 0) {
			ili9341_draw_line(&ili9341_display, prev_x, prev_y, px, py, color);
		}
		prev_x = px;
		prev_y = py;
	}
}

static void gui_draw()
{
	char line[48] = {0};
	//ili9341_fill_screen(&ili9341_display, COLOR_BLACK);

	ili9341_draw_text(&ili9341_display, 8, 8, "Portable atmosphere meter", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);

	snprintf(line, sizeof(line), "Temp: %2.2f C", last_temp_C);
	ili9341_draw_text(&ili9341_display, 8, 30, line, &jetbrains_font, color_by_temp(last_temp_C), COLOR_BLACK);
	draw_bar(170, 28, 135, 14, last_temp_C, 0.0f, 50.0f, color_by_temp(last_temp_C));

	snprintf(line, sizeof(line), "Pres: %6.1f Pa", last_pres_Pa);
	ili9341_draw_text(&ili9341_display, 8, 50, line, &jetbrains_font, COLOR_CYAN, COLOR_BLACK);
	draw_bar(170, 48, 135, 14, last_pres_Pa, 90000.0f, 110000.0f, COLOR_CYAN);

	snprintf(line, sizeof(line), "Hum : %2.2f %%", last_hum_percent);
	ili9341_draw_text(&ili9341_display, 8, 70, line, &jetbrains_font, color_by_hum(last_hum_percent), COLOR_BLACK);
	draw_bar(170, 68, 135, 14, last_hum_percent, 0.0f, 100.0f, color_by_hum(last_hum_percent));

	snprintf(line, sizeof(line), "Period: %lu ms", meas_period_ms);
	ili9341_draw_text(&ili9341_display, 8, 92, line, &jetbrains_font, COLOR_WHITE, COLOR_BLACK);

	ili9341_draw_text(&ili9341_display, 8, 114, "Last 60 samples", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
	draw_graph(8, 132, 95, 90, temp_history, 0.0f, 50.0f, COLOR_RED);
	draw_graph(112, 132, 95, 90, pres_history, 90000.0f, 110000.0f, COLOR_CYAN);
	draw_graph(216, 132, 95, 90, hum_history, 0.0f, 100.0f, COLOR_GREEN);
	ili9341_draw_text(&ili9341_display, 26, 224, "T", &jetbrains_font, COLOR_RED, COLOR_BLACK);
	ili9341_draw_text(&ili9341_display, 150, 224, "P", &jetbrains_font, COLOR_CYAN, COLOR_BLACK);
	ili9341_draw_text(&ili9341_display, 256, 224, "H", &jetbrains_font, COLOR_GREEN, COLOR_BLACK);
}

static void measurement_task_handle()
{
	if (time_us_64() < meas_ts) {
		return;
	}
	meas_ts = time_us_64() + ((uint64_t)meas_period_ms * 1000);

	last_temp_C = bme280_read_temp();
	last_pres_Pa = bme280_read_pres();
	last_hum_percent = bme280_read_hum();
	history_push(last_temp_C, last_pres_Pa, last_hum_percent);

	printf("%f %f %f\n", last_temp_C, last_pres_Pa, last_hum_percent);
	if (gui_enabled) {
		gui_draw();
	}
}

void version_callback(const char* args)
{
	printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
	printf("Led turned on\n");
	led_task_state_set(LED_STATE_ON);
}

void led_off_callback(const char* args)
{
	printf("Led turned off\n");
	led_task_state_set(LED_STATE_OFF);
}

void led_blink_callback(const char* args)
{
	printf("Led is blinking now\n");
	led_task_state_set(LED_STATE_BLINK);
}

void led_blink_set_period_ms_callback(const char* args){
	uint period_ms = 0;
	sscanf(args, "%u", &period_ms);
	if (period_ms == 0){
		printf("Error incorrect period\n");
		return;
	}
	led_task_set_blink_period_ms(period_ms);
}

void meas_period_callback(const char* args)
{
	uint32_t period_ms = 0;
	int scanned = sscanf(args, "%lu", &period_ms);
	if (scanned != 1 || period_ms < 250) {
		printf("Error incorrect measurement period\n");
		return;
	}
	meas_period_ms = period_ms;
	printf("measurement period: %lu ms\n", meas_period_ms);
}

void measure_callback(const char* args)
{
	last_temp_C = bme280_read_temp();
	last_pres_Pa = bme280_read_pres();
	last_hum_percent = bme280_read_hum();
	history_push(last_temp_C, last_pres_Pa, last_hum_percent);
	printf("%f %f %f\n", last_temp_C, last_pres_Pa, last_hum_percent);
	if (gui_enabled) {
		gui_draw();
	}
}

void gui_on_callback(const char* args)
{
	gui_enabled = true;
	gui_draw();
}

void gui_off_callback(const char* args)
{
	gui_enabled = false;
}

void gui_clear_callback(const char* args)
{
	ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
}

void mem_callback(const char* args)
{
	uint32_t addr = 0;
	int scanned = sscanf(args, "%lx", &addr);
	if (scanned == 1) {
		mem(addr);
	}
	else {
		printf("Error incorrect address\n");
	}
}

void wmem_callback(const char* args) {
	uint32_t addr = 0, value = 0;
	int scanned = sscanf(args, "%lx %lx", &addr, &value);
	if (scanned == 2) {
		wmem(addr, value);
	}
	else {
		printf("Error incorrect address or value\n");
	}
}

int main()
{
	stdio_init_all();
	led_task_init();
	stdio_task_init();
	sensor_task_init();
	display_task_init();
	meas_ts = 0;

	api_t device_api[] =
	{
		{"version", version_callback, "get device name and firmware version"},
		{"on",      led_on_callback,  "switch on led"},
		{"off",     led_off_callback, "switch off led"},
		{"blink",   led_blink_callback, "blink led"},
		{"set_period", led_blink_set_period_ms_callback, "set led blink period"},
		{"meas_period", meas_period_callback, "set measurement period ms"},
		{"measure", measure_callback, "measure now"},
		{"gui_on", gui_on_callback, "enable display gui"},
		{"gui_off", gui_off_callback, "disable display gui"},
		{"gui_clear", gui_clear_callback, "clear display"},
		{"mem", mem_callback, "Address value printed"},
		{"wmem", wmem_callback, "Addres value changed"},
		{"help", NULL, "show commands list"},
		{NULL, NULL, NULL}
	};
	protocol_task_init(device_api);
	if(gui_enabled){
		ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
	}
	while (1) {
		protocol_task_handle(stdio_task_handle());
		led_task_handle();
		measurement_task_handle();
	}
}
