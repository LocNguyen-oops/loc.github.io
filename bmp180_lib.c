#include "bmp180.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h> // For I2C_SLAVE
#include <errno.h>
#include <string.h> // For strerror

// Hàm mở thiết bị BMP180
int bmp180_open() {
    int fd = open("/dev/i2c-1", O_RDWR); 
    if (fd < 0) {
        fprintf(stderr, "Failed to open I2C bus: %s\n", strerror(errno));
        return -1;
    }

    // Thiết lập địa chỉ slave cho I2C
    if (ioctl(fd, I2C_SLAVE, BMP180_I2C_ADDR) < 0) {
        fprintf(stderr, "Failed to set I2C slave address: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

// Hàm đóng thiết bị BMP180
void bmp180_close(int fd) {
    close(fd);
}

// Hàm đọc thanh ghi từ BMP180
int bmp180_read_register(int fd, uint8_t reg, uint8_t *value) {
    if (write(fd, &reg, 1) != 1) {
        fprintf(stderr, "Failed to write register address: %s\n", strerror(errno));
        return -1;
    }

    if (read(fd, value, 1) != 1) {
        fprintf(stderr, "Failed to read register data: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

// Hàm ghi giá trị vào thanh ghi của BMP180
int bmp180_write_register(int fd, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    if (write(fd, data, 2) != 2) {
        fprintf(stderr, "Failed to write register data: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

// Hàm đọc dữ liệu hiệu chuẩn từ BMP180
int bmp180_read_calibration_data(int fd, BMP180_CalibrationData *cal_data) {
    uint8_t data[22];
    uint8_t reg = BMP180_REG_CALIBRATION_DATA;

    if (write(fd, &reg, 1) != 1) {
        fprintf(stderr, "Failed to write calibration data register address: %s\n", strerror(errno));
        return -1;
    }

    if (read(fd, data, 22) != 22) {
        fprintf(stderr, "Failed to read calibration data: %s\n", strerror(errno));
        return -1;
    }

    // Giải mã dữ liệu hiệu chuẩn
    cal_data->AC1 = (int16_t)((data[0] << 8) | data[1]);
    cal_data->AC2 = (int16_t)((data[2] << 8) | data[3]);
    cal_data->AC3 = (int16_t)((data[4] << 8) | data[5]);
    cal_data->AC4 = (uint16_t)((data[6] << 8) | data[7]);
    cal_data->AC5 = (uint16_t)((data[8] << 8) | data[9]);
    cal_data->AC6 = (uint16_t)((data[10] << 8) | data[11]);
    cal_data->B1 = (int16_t)((data[12] << 8) | data[13]);
    cal_data->B2 = (int16_t)((data[14] << 8) | data[15]);
    cal_data->MB = (int16_t)((data[16] << 8) | data[17]);
    cal_data->MC = (int16_t)((data[18] << 8) | data[19]);
    cal_data->MD = (int16_t)((data[20] << 8) | data[21]);

    return 0;
}

// Hàm đọc dữ liệu nhiệt độ thô
int bmp180_read_raw_temp(int fd) {
    uint8_t reg = BMP180_CMD_TEMP;
    if (bmp180_write_register(fd, BMP180_REG_CTRL_MEASURE, reg) != 0) {
        fprintf(stderr, "Failed to write temperature measurement command\n");
        return -1;
    }

    usleep(50000); // Chờ chuyển đổi hoàn tất 

    uint8_t temp_data[2];
    if (bmp180_read_register(fd, BMP180_REG_RESULT, &temp_data[0]) != 0 ||
        bmp180_read_register(fd, BMP180_REG_RESULT + 1, &temp_data[1]) != 0) {
        fprintf(stderr, "Failed to read raw temperature data\n");
        return -1;
    }

    return (temp_data[0] << 8) | temp_data[1];
}

// Hàm đọc dữ liệu áp suất thô
int bmp180_read_raw_pressure(int fd) {
    uint8_t reg = BMP180_CMD_PRESSURE;
    if (bmp180_write_register(fd, BMP180_REG_CTRL_MEASURE, reg) != 0) {
        fprintf(stderr, "Failed to write pressure measurement command\n");
        return -1;
    }

    usleep(100000); // Chờ chuyển đổi hoàn tất 

    uint8_t pressure_data[3];
    if (bmp180_read_register(fd, BMP180_REG_RESULT, &pressure_data[0]) != 0 ||
        bmp180_read_register(fd, BMP180_REG_RESULT + 1, &pressure_data[1]) != 0 ||
        bmp180_read_register(fd, BMP180_REG_RESULT + 2, &pressure_data[2]) != 0) {
        fprintf(stderr, "Failed to read raw pressure data\n");
        return -1;
    }

    return (pressure_data[0] << 16) | (pressure_data[1] << 8) | pressure_data[2];
}

// Hàm tính toán nhiệt độ từ dữ liệu thô
float bmp180_calculate_temp(int fd, BMP180_CalibrationData *cal_data) {
    int raw_temp = bmp180_read_raw_temp(fd);

    // Tính toán nhiệt độ
    int X1 = ((raw_temp - cal_data->AC6) * cal_data->AC5) >> 15;
    int X2 = (cal_data->MC << 11) / (X1 + cal_data->MD);
    int B5 = X1 + X2;
    float temp = (B5 + 8) >> 4;
    temp /= 10.0;

    return temp;
}

// Hàm tính toán áp suất từ dữ liệu thô
float bmp180_calculate_pressure(int fd, BMP180_CalibrationData *cal_data, int32_t raw_pressure) {
    int raw_temp = bmp180_read_raw_temp(fd);
    int X1 = ((raw_temp - cal_data->AC6) * cal_data->AC5) >> 15;
    int X2 = (cal_data->MC << 11) / (X1 + cal_data->MD);
    int B5 = X1 + X2;

    // Tính toán áp suất
    int B6 = B5 - 4000;
    int X1_ = (cal_data->B2 * (B6 * B6 >> 12)) >> 11; 
    int X2_ = (cal_data->AC2 * B6) >> 11;
    int X3 = X1_ + X2_;
    int B3 = ((cal_data->AC1 * 4 + X3) << 3) + 2;
    B3 /= 4;

    

    return 0.0; // Trả về giá trị áp suất đã tính toán
}