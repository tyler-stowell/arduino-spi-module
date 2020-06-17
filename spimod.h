#define BUF_SIZE 512

/* IOCTL STUFF */
#define IOC_MAGIC 'Q'
#define IOCTL_SEND_MESSEGE _IO(IOC_MAGIC, 0)


struct arduino_dev {
	struct spi_device *spi;
	unsigned char tx_buf[BUF_SIZE];
	unsigned char rx_buf[BUF_SIZE];
	unsigned int max_speed_hz;
	spinlock_t spinlock;
	bool opened;
};
