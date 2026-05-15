#include "bme280-driver.h"
#include "bme280-regs.h"
#include "stdio.h"

static bme280_ctx_t bme280_ctx = {0};

static uint16_t dig_T1;
static int16_t dig_T2;
static int16_t dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2;
static int16_t dig_P3;
static int16_t dig_P4;
static int16_t dig_P5;
static int16_t dig_P6;
static int16_t dig_P7;
static int16_t dig_P8;
static int16_t dig_P9;
static uint8_t dig_H1;
static int16_t dig_H2;
static uint8_t dig_H3;
static int16_t dig_H4;
static int16_t dig_H5;
static int8_t dig_H6;
static int32_t t_fine;

static uint16_t read_u16_le(uint8_t* data)
{
	return ((uint16_t)data[1] << 8) | data[0];
}

static int16_t read_s16_le(uint8_t* data)
{
	return (int16_t)read_u16_le(data);
}

static int16_t sign_extend_12(uint16_t value)
{
	if (value & 0x0800) {
		value |= 0xF000;
	}
	return (int16_t)value;
}

void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length)
{
	uint8_t data[1] = {start_reg_address};
	bme280_ctx.i2c_write(data, sizeof(data));
	bme280_ctx.i2c_read(buffer, length);
}

void bme280_write_reg(uint8_t reg_address, uint8_t value)
{
	uint8_t data[2] = {reg_address, value};
	bme280_ctx.i2c_write(data, sizeof(data));
}

static void bme280_read_calibration()
{
	uint8_t calib00[26] = {0};
	uint8_t calib26[7] = {0};

	bme280_read_regs(BME280_REG_calib00, calib00, sizeof(calib00));
	bme280_read_regs(BME280_REG_calib26, calib26, sizeof(calib26));

	dig_T1 = read_u16_le(&calib00[0]);
	dig_T2 = read_s16_le(&calib00[2]);
	dig_T3 = read_s16_le(&calib00[4]);
	dig_P1 = read_u16_le(&calib00[6]);
	dig_P2 = read_s16_le(&calib00[8]);
	dig_P3 = read_s16_le(&calib00[10]);
	dig_P4 = read_s16_le(&calib00[12]);
	dig_P5 = read_s16_le(&calib00[14]);
	dig_P6 = read_s16_le(&calib00[16]);
	dig_P7 = read_s16_le(&calib00[18]);
	dig_P8 = read_s16_le(&calib00[20]);
	dig_P9 = read_s16_le(&calib00[22]);
	dig_H1 = calib00[25];
	dig_H2 = read_s16_le(&calib26[0]);
	dig_H3 = calib26[2];
	dig_H4 = sign_extend_12(((uint16_t)calib26[3] << 4) | (calib26[4] & 0x0F));
	dig_H5 = sign_extend_12(((uint16_t)calib26[5] << 4) | (calib26[4] >> 4));
	dig_H6 = (int8_t)calib26[6];
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write)
{
	bme280_ctx.i2c_read = i2c_read;
	bme280_ctx.i2c_write = i2c_write;

	uint8_t id_reg_buf[1] = {0};
	bme280_read_regs(BME280_REG_id, id_reg_buf, sizeof(id_reg_buf));
	if (id_reg_buf[0] != 0x60) {
		printf("BME280 id error: 0x%X\n", id_reg_buf[0]);
	}

	bme280_read_calibration();

	uint8_t ctrl_hum_reg_value = 0;
	ctrl_hum_reg_value |= (0b001 << 0);
	bme280_write_reg(BME280_REG_ctrl_hum, ctrl_hum_reg_value);

	uint8_t config_reg_value = 0;
	config_reg_value |= (0b0 << 0);
	config_reg_value |= (0b000 << 2);
	config_reg_value |= (0b001 << 5);
	bme280_write_reg(BME280_REG_config, config_reg_value);

	uint8_t ctrl_meas_reg_value = 0;
	ctrl_meas_reg_value |= (0b11 << 0);
	ctrl_meas_reg_value |= (0b001 << 2);
	ctrl_meas_reg_value |= (0b001 << 5);
	bme280_write_reg(BME280_REG_ctrl_meas, ctrl_meas_reg_value);
}

uint32_t bme280_read_temp_raw()
{
	uint8_t read[3] = {0};
	bme280_read_regs(BME280_REG_temp_msb, read, sizeof(read));
	uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | (read[2] >> 4);
	return value;
}

uint32_t bme280_read_pres_raw()
{
	uint8_t read[3] = {0};
	bme280_read_regs(BME280_REG_press_msb, read, sizeof(read));
	uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | (read[2] >> 4);
	return value;
}

uint16_t bme280_read_hum_raw()
{
	uint8_t read[2] = {0};
	bme280_read_regs(BME280_REG_hum_msb, read, sizeof(read));
	uint16_t value = ((uint16_t)read[0] << 8) | read[1];
	return value;
}

static int32_t bme280_compensate_temp_int(uint32_t adc_T)
{
	int32_t var1 = ((((int32_t)adc_T >> 3) - ((int32_t)dig_T1 << 1)) * (int32_t)dig_T2) >> 11;
	int32_t var2 = (((((int32_t)adc_T >> 4) - (int32_t)dig_T1) * (((int32_t)adc_T >> 4) - (int32_t)dig_T1)) >> 12) * (int32_t)dig_T3;
	var2 = var2 >> 14;
	t_fine = var1 + var2;
	return (t_fine * 5 + 128) >> 8;
}

float bme280_read_temp()
{
	int32_t temp_C_x100 = bme280_compensate_temp_int(bme280_read_temp_raw());
	return temp_C_x100 / 100.0f;
}

float bme280_read_pres()
{
	bme280_compensate_temp_int(bme280_read_temp_raw());
	uint32_t adc_P = bme280_read_pres_raw();

	int64_t var1 = (int64_t)t_fine - 128000;
	int64_t var2 = var1 * var1 * (int64_t)dig_P6;
	var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
	var2 = var2 + (((int64_t)dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
	if (var1 == 0) {
		return 0.0f;
	}

	int64_t p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
	return p / 256.0f;
}

float bme280_read_hum()
{
	bme280_compensate_temp_int(bme280_read_temp_raw());
	uint32_t adc_H = bme280_read_hum_raw();

	int32_t v_x1 = t_fine - 76800;
	v_x1 = (((((adc_H << 14) - ((int32_t)dig_H4 << 20) - ((int32_t)dig_H5 * v_x1)) + 16384) >> 15) *
		(((((((v_x1 * (int32_t)dig_H6) >> 10) * (((v_x1 * (int32_t)dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
		(int32_t)dig_H2 + 8192) >> 14));
	v_x1 = v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * (int32_t)dig_H1) >> 4);
	if (v_x1 < 0) {
		v_x1 = 0;
	}
	if (v_x1 > 419430400) {
		v_x1 = 419430400;
	}
	return (v_x1 >> 12) / 1024.0f;
}
