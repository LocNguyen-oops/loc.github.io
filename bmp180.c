#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/init.h>

#define DRIVER_NAME "bmp180_driver"
#define DEVICE_NAME "bmp180_sensor"
#define CLASS_NAME  "bmp180"

#define BMP180_I2C_ADDR      0x77

// Địa chỉ thanh ghi cho BMP180
#define BMP180_REG_TEMP        0xF6
#define BMP180_REG_PRESSURE    0xF6
#define BMP180_REG_CONTROL     0xF4
#define BMP180_REG_CALIBRATION 0xAA

// Các lệnh IOCTL để tương tác với driver
#define IOCTL_CONFIG           0x01
#define IOCTL_SHUTDOWN         0x02
#define IOCTL_RESET            0x03
#define IOCTL_READ_REGISTER    0x04
#define IOCTL_WRITE_REGISTER   0x05

// Biến toàn cục
static int major_number;         
static struct class *bmp180_class = NULL; 
static struct device *bmp180_device = NULL; 
static struct i2c_client *bmp180_client; 

// Hàm ghi dữ liệu vào thanh ghi của BMP180
static int writeRegister(struct i2c_client *client, u8 reg, u8 value) { 
    uint8_t data[2] = {reg, value};
    int result = i2c_master_send(client, data, 2); 

    if (result != 2) {
        dev_err(&client->dev, "Failed to write to register 0x%02X\n", reg);
        return -EIO;
    }
    return 0;
}

// Hàm đọc dữ liệu từ thanh ghi của BMP180
static int readRegister(struct i2c_client *client, u8 reg, u8 *value) { 
    int result = i2c_master_send(client, &reg, 1); 
    if (result < 0) {
        dev_err(&client->dev, "Failed to send register address 0x%02X\n", reg);
        return result;
    }

    result = i2c_master_recv(client, value, 1);
    if (result < 0) {
        dev_err(&client->dev, "Failed to read from register 0x%02X\n", reg);
        return result;
    }
    return 0;
}

// Hàm đọc nhiệt độ từ BMP180
static int readTemperature(struct i2c_client *client) {
    u8 temperature_value;
    int result = readRegister(client, BMP180_REG_TEMP, &temperature_value);
    if (result < 0) {
        return result;
    }
    return temperature_value;
}

// Hàm đọc áp suất từ BMP180
static int readPressure(struct i2c_client *client) {
    u8 pressure_value;
    int result = readRegister(client, BMP180_REG_PRESSURE, &pressure_value);
    if (result < 0) {
        return result;
    }
    return pressure_value;
}

// Hàm reset BMP180
static void resetSensor(struct i2c_client *client) { 
    writeRegister(client, BMP180_REG_CONTROL, 0xB6);
    printk(KERN_INFO "BMP180: Sensor reset\n");
}

// Hàm tắt BMP180
static void shutdownSensor(struct i2c_client *client) { 
    writeRegister(client, BMP180_REG_CONTROL, 0x34);
    printk(KERN_INFO "BMP180: Sensor shutdown\n"); 
}

// Hàm cấu hình BMP180
static void configureSensor(struct i2c_client *client) { 
    // Cấu hình BMP180 để đo nhiệt độ và áp suất
    writeRegister(client, BMP180_REG_CONTROL, 0x2E); 
    writeRegister(client, BMP180_REG_CONTROL, 0x34); 
    printk(KERN_INFO "BMP180: Sensor configured\n"); 
}

// Hàm xử lý khi thiết bị được mở
static int deviceOpen(struct inode *inode, struct file *file) { 
    printk(KERN_INFO "BMP180: Device opened\n");
    return 0;
}

// Hàm xử lý khi thiết bị được đóng
static int deviceClose(struct inode *inode, struct file *file) { 
    shutdownSensor(bmp180_client);
    printk(KERN_INFO "BMP180: Device closed\n");
    return 0;
}

// Hàm xử lý các lệnh IOCTL
static long deviceIoctl(struct file *file, unsigned int cmd, unsigned long arg) {
    u8 reg;
    int result;

    switch (cmd) {
        case IOCTL_CONFIG: 
            configureSensor(bmp180_client);
            break;
        case IOCTL_SHUTDOWN: 
            shutdownSensor(bmp180_client);
            break;
        case IOCTL_RESET:   
            resetSensor(bmp180_client);
            break;
        case IOCTL_READ_REGISTER: 
            if (copy_from_user(&reg, (uint8_t *)arg, 1)) {
                return -EFAULT; 
            }
            result = readRegister(bmp180_client, reg, &reg);
            if (result < 0) {
                return result;
            }
            if (copy_to_user((uint8_t *)arg, &reg, 1)) {
                return -EFAULT; 
            }
            break;
        case IOCTL_WRITE_REGISTER: 
            if (copy_from_user(&reg, (uint8_t *)arg, 1)) {
                return -EFAULT; 
            }
            result = writeRegister(bmp180_client, reg, 0x00);  
            if (result < 0) {
                return result;
            }
            printk(KERN_INFO "BMP180: Register 0x%02X written with 0x00\n", reg);
            break;
        default:
            return -ENOTTY; 
    }
    return 0;
}

// Hàm đọc dữ liệu từ cảm biến
static ssize_t deviceRead(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    u8 data[2]; 
    int result;

    // Kiểm tra kích thước buffer
    if (len < 2) {
        printk(KERN_ERR "BMP180: Buffer too small\n");
        return -EINVAL; 
    }

    // Đọc nhiệt độ
    result = readTemperature(bmp180_client);
    if (result < 0) {
        return result;
    }
    data[0] = result;

    // Đọc áp suất
    result = readPressure(bmp180_client);
    if (result < 0) {
        return result;
    }
    data[1] = result;

    // Sao chép dữ liệu vào buffer của người dùng
    if (copy_to_user(buf, data, 2)) {
        return -EFAULT; 
    }

    return 2; 
}

// Cấu trúc file_operations cho các hoạt động trên thiết bị
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = deviceOpen,
    .release = deviceClose,
    .unlocked_ioctl = deviceIoctl,
    .read = deviceRead,
};

// Hàm probe được gọi khi thiết bị BMP180 được phát hiện
static int bmp180_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    bmp180_client = client; 

    // Đăng ký thiết bị ký tự
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "BMP180: Failed to register major number\n");
        return major_number;
    }

    // Tạo lớp thiết bị
    bmp180_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(bmp180_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "BMP180: Failed to create device class\n");
        return PTR_ERR(bmp180_class);
    }

    // Tạo thiết bị
    bmp180_device = device_create(bmp180_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(bmp180_device)) {
        class_destroy(bmp180_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "BMP180: Failed to create device\n");
        return PTR_ERR(bmp180_device);
    }

    printk(KERN_INFO "BMP180: Device initialized successfully\n");
    return 0;
}

// Hàm remove được gọi khi module bị gỡ bỏ
static void bmp180_remove(struct i2c_client *client) {
    device_destroy(bmp180_class, MKDEV(major_number, 0));
    class_destroy(bmp180_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "BMP180: Driver removed\n");
}

// ID table cho thiết bị BMP180
static const struct i2c_device_id bmp180_id[] = {
    { "bmp180", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bmp180_id);

// Cấu trúc i2c_driver cho driver BMP180
static struct i2c_driver bmp180_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe = bmp180_probe,
    .remove = bmp180_remove,
    .id_table = bmp180_id,
};

// Hàm init được gọi khi module được tải
static int __init bmp180_init(void) {
    printk(KERN_INFO "BMP180: Initializing driver\n");
    return i2c_add_driver(&bmp180_driver);
}

// Hàm exit được gọi khi module bị gỡ
static void __exit bmp180_exit(void) {
    i2c_del_driver(&bmp180_driver);
    printk(KERN_INFO "BMP180: Exiting driver\n");
}

module_init(bmp180_init);
module_exit(bmp180_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Driver for the BMP180 sensor");
MODULE_VERSION("1.0");