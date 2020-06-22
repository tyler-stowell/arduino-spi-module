#define BUF_SIZE 1024
#define DEV_NAME "ArduinoSpidev"

struct arduino_dev {
	struct spi_device *spi;
	unsigned char tx_buf[BUF_SIZE];
	unsigned char rx_buf[BUF_SIZE];
	unsigned int max_speed_hz;
	spinlock_t spinlock;
	bool opened;
};
