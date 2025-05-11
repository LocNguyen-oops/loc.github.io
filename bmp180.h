#ifndef BMP180_LIB_H
#define BMP180_LIB_H

#include <stdint.h>

// I2C Address for BMP180
#define BMP180_I2C_ADDR 0x77

// Registers
#define BMP180_REG_CALIBRATION_DATA 0xAA
#define BMP180_REG_CTRL_MEASURE     0xF4
#define BMP180_REG_RESULT           0xF6
#define BMP180_REG_TEMP             0x2E
#define BMP180_REG_PRESSURE         0x34

// Control Command for BMP180
#define BMP180_CMD_TEMP             0x2E
#define BMP180_CMD_PRESSURE         0x34

// Calibration Data Structure
typedef struct {
    int16_t AC1, AC2, AC3;
    uint16_t AC4, AC5, AC6;
    int16_t B1, B2;
    int16_t MB, MC, MD;
} BMP180_CalibrationData;

// Function Declarations
int bmp180_open();
void bmp180_close(int fd);
int bmp180_read_register(int fd, uint8_t reg, uint8_t *value);
int bmp180_write_register(int fd, uint8_t reg, uint8_t value);
int bmp180_read_calibration_data(int fd, BMP180_CalibrationData *cal_data);
int bmp180_read_raw_temp(int fd);
int bmp180_read_raw_pressure(int fd);
float bmp180_calculate_temp(int fd, BMP180_CalibrationData *cal_data);
float bmp180_calculate_pressure(int fd, BMP180_CalibrationData *cal_data, int32_t raw_pressure);

#endif