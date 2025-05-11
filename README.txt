/*
Thành viên:
Nguyễn Thành Lộc - 22146345
Nguyễn Lê Vũ  - 22146449
Phan Thanh Phú - 22146369
*/

/* bmp180_open(): Mở thiết bị BMP180 và trả về file descriptor */
/* bmp180_close(int fd): Đóng thiết bị BMP180 */
/* bmp180_read_register(int fd, uint8_t reg, uint8_t *value): Đọc giá trị từ thanh ghi 'reg' vào 'value' */
/* bmp180_write_register(int fd, uint8_t reg, uint8_t value): Ghi 'value' vào thanh ghi 'reg' */
/* bmp180_read_calibration_data(int fd, BMP180_CalibrationData *cal_data): Đọc dữ liệu hiệu chuẩn vào struct 'cal_data' */
/* bmp180_read_raw_temp(int fd): Đọc giá trị nhiệt độ thô từ cảm biến */
/* bmp180_read_raw_pressure(int fd): Đọc giá trị áp suất thô từ cảm biến */
/* bmp180_calculate_temp(int fd, BMP180_CalibrationData *cal_data): Tính toán nhiệt độ từ dữ liệu thô và hiệu chuẩn */
/* bmp180_calculate_pressure(int fd, BMP180_CalibrationData *cal_data, int32_t raw_pressure): Tính toán áp suất từ dữ liệu thô và hiệu chuẩn */