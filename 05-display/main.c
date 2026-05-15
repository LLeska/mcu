#include "pico/stdio.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "stdio-task/stdio-task.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "mem-task/mem-task.h"
#include "ili9341-driver.h"
#include "ili9341-display.h"
#include "ili9341-font.h"

#define DEVICE_NAME "my-pico-display"
#define DEVICE_VRSN "v0.0.5"

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

uint32_t global_variable = 0;

const uint32_t constant_variable = 42;

static ili9341_display_t ili9341_display = {0};

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

void display_task_init()
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
}

void display_startup_draw()
{
	ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
	sleep_ms(300);
	ili9341_draw_filled_rect(&ili9341_display, 10, 10, 100, 60, COLOR_RED);
	ili9341_draw_filled_rect(&ili9341_display, 120, 10, 100, 60, COLOR_GREEN);
	ili9341_draw_filled_rect(&ili9341_display, 230, 10, 80, 60, COLOR_BLUE);
	ili9341_draw_rect(&ili9341_display, 10, 90, 300, 80, COLOR_WHITE);
	ili9341_draw_line(&ili9341_display, 0, 0, 319, 239, COLOR_YELLOW);
	ili9341_draw_line(&ili9341_display, 319, 0, 0, 239, COLOR_CYAN);
	ili9341_draw_text(&ili9341_display, 20, 100, "Hello, ILI9341!", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
	ili9341_draw_text(&ili9341_display, 20, 116, "RP2040 / Pico SDK", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);
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
void disp_screen_callback(const char* args)
{
	uint32_t c = 0;
	int result = sscanf(args, "%lx", &c);
	uint16_t color = COLOR_BLACK;
	if (result == 1) {
		color = RGB888_2_RGB565(c);
	}
	ili9341_fill_screen(&ili9341_display, color);
}
void disp_px_callback(const char* args)
{
	uint32_t x = 0, y = 0, c = 0;
	int result = sscanf(args, "%lu %lu %lx", &x, &y, &c);
	if (result != 3) {
		printf("Error incorrect pixel args\n");
		return;
	}
	ili9341_draw_pixel(&ili9341_display, x, y, RGB888_2_RGB565(c));
}
void disp_line_callback(const char* args)
{
	uint32_t x0 = 0, y0 = 0, x1 = 0, y1 = 0, c = 0;
	int result = sscanf(args, "%lu %lu %lu %lu %lx", &x0, &y0, &x1, &y1, &c);
	if (result != 5) {
		printf("Error incorrect line args\n");
		return;
	}
	ili9341_draw_line(&ili9341_display, x0, y0, x1, y1, RGB888_2_RGB565(c));
}
void disp_rect_callback(const char* args)
{
	uint32_t x = 0, y = 0, w = 0, h = 0, c = 0;
	int result = sscanf(args, "%lu %lu %lu %lu %lx", &x, &y, &w, &h, &c);
	if (result != 5) {
		printf("Error incorrect rect args\n");
		return;
	}
	ili9341_draw_rect(&ili9341_display, x, y, w, h, RGB888_2_RGB565(c));
}
void disp_frect_callback(const char* args)
{
	uint32_t x = 0, y = 0, w = 0, h = 0, c = 0;
	int result = sscanf(args, "%lu %lu %lu %lu %lx", &x, &y, &w, &h, &c);
	if (result != 5) {
		printf("Error incorrect filled rect args\n");
		return;
	}
	ili9341_draw_filled_rect(&ili9341_display, x, y, w, h, RGB888_2_RGB565(c));
}
void disp_text_callback(const char* args)
{
	uint32_t x = 0, y = 0, color = 0xFFFFFF, bg_color = 0x000000;
	char text[128] = {0};
	int result = sscanf(args, "%lu %lu %lx %lx %127[^\r\n]", &x, &y, &color, &bg_color, text);
	if (result != 5) {
		printf("Error incorrect text args\n");
		return;
	}
	ili9341_draw_text(&ili9341_display, x, y, text, &jetbrains_font, RGB888_2_RGB565(color), RGB888_2_RGB565(bg_color));
}

int main()
{
	stdio_init_all();
	led_task_init();
	display_task_init();
	display_startup_draw();
	stdio_task_init();
	api_t device_api[] =
	{
		{"version", version_callback, "get device name and firmware version"},
		{"on",      led_on_callback,  "switch on led"},
		{"off",     led_off_callback, "switch off led"},
		{"blink",   led_blink_callback, "provide unblocking"},
		{"mem", mem_callback, "Address value printed"},
		{"wmem", wmem_callback, "Addres value changed"},
		{"set_period",   led_blink_set_period_ms_callback, "blinking with arguments"},
		{"disp_screen", disp_screen_callback, "fill display screen"},
		{"disp_px", disp_px_callback, "draw display pixel"},
		{"disp_line", disp_line_callback, "draw display line"},
		{"disp_rect", disp_rect_callback, "draw display rectangle"},
		{"disp_frect", disp_frect_callback, "draw filled display rectangle"},
		{"disp_text", disp_text_callback, "draw display text"},

		{"help", NULL, "show commands list"},
		{NULL, NULL, NULL} 
	};
	protocol_task_init(device_api);
	while (1) {
		protocol_task_handle(stdio_task_handle());
		led_task_handle();
	}
}
