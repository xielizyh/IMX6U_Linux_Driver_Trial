/**
  ******************************************************************************
  * @file			icm20608.c
  * @brief			icm20608 function
  * @author			Xli
  * @email			xieliyzh@163.com
  * @version		1.0.0
  * @date			2020-03-17
  * @copyright		2020, XIELI Co.,Ltd. All rights reserved
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "icm20608.h"

/* Private constants ---------------------------------------------------------*/
#define ICM20608_CNT				1			/*!< 设备号个数 */
#define ICM20608_NAME				"icm20608"	/*!< 设备名 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* icm20608设备结构体 */
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	struct device_node *nd; /*!< 设备节点 */
	int major;				/*!< 主设备号 */
	void *private_data;		/*!< 私有数据 */
	int cs_gpio;			/*!< 片选所使用的GPIO编号 */
	signed int gyro_x_adc;		/* 陀螺仪X轴原始值 	*/
	signed int gyro_y_adc;		/* 陀螺仪Y轴原始值 	*/
	signed int gyro_z_adc;		/* 陀螺仪Z轴原始值 	*/
	signed int accel_x_adc;		/* 加速度计X轴原始值 */
	signed int accel_y_adc;		/* 加速度计Y轴原始值 */
	signed int accel_z_adc;		/* 加速度计Z轴原始值 */
	signed int temp_adc;		/* 温度原始值 	*/
}icm20608_dev_t;

/* Private variables ---------------------------------------------------------*/
static icm20608_dev_t icm20608dev;

/* 传统匹配方式列表 */
static const struct spi_device_id icm20608_id[] = {
	{"xli,icm20608", 0},
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id icm20608_of_match[] = {
	{ .compatible = "xli,icm20608" },
	{ /* sentinel */}
};

static int icm20608_probe(struct spi_device *spi);
static int icm20608_remove(struct spi_device *spi);

/* spi驱动结构体 */
static struct spi_driver icm20608_driver = {
	.probe = icm20608_probe,
	.remove = icm20608_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "icm20608",
		.of_match_table = icm20608_of_match,
	},
	.id_table = icm20608_id,
};

static int icm20608_open(struct inode *inode, struct file *filp);
static ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off);
static int icm20608_release(struct inode *inode, struct file *filp);

/* icm20608操作函数 */
static const struct file_operations icm20608_ops = {
	.owner = THIS_MODULE,
	.open = icm20608_open,
	.read = icm20608_read,
	.release = icm20608_release,
};

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           从icm20608读取多个寄存器数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int icm20608_read_regs(icm20608_dev_t *dev, u8 reg, void *buf, int len)
{
	int ret = 0;
	unsigned char txdata[len];
	struct spi_message msg;
	struct spi_transfer *tran;
	struct spi_device *spi = (struct spi_device*)dev->private_data;

	gpio_set_value(dev->cs_gpio, 0);	/*!< 片选拉低，选中 */
	tran = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/* 第一次：发送要读取的寄存器地址 */
	txdata[0] = reg | 0x80;				/*!< 读数据时寄存器地址BIT8置1 */
	tran->tx_buf = txdata;				/*!< 要发送的数据 */
	tran->len = 1;						/*!< 1个字节 */
	spi_message_init(&msg);				/*!< 初始化spi_message */
	spi_message_add_tail(tran, &msg);	/*!< spi_transfer添加到spi_message */
	ret = spi_sync(spi, &msg);			/*!< 同步发送 */

	/* 第二次：读取数据 */
	txdata[0] = 0xFF;					/*!< 此次无意义 */
	tran->rx_buf = buf;					/*!< 读取到的数据 */
	tran->len = len;					/*!< 要读取的数据长度 */
	spi_message_init(&msg);				/*!< 初始化spi_message */
	spi_message_add_tail(tran, &msg);	/*!< spi_transfer添加到spi_message */
	ret = spi_sync(spi, &msg);			/*!< 同步发送 */

	kfree(tran);						/*!< 释放内存 */	
	gpio_set_value(dev->cs_gpio, 1);	/*!< 片选拉高，释放 */

	return ret;
}

/**=============================================================================
 * @brief           向icm20608多个寄存器写入数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static s32 icm20608_write_regs(icm20608_dev_t *dev, u8 reg, u8 *buf, u8 len)
{
	int ret = 0;
	unsigned char txdata[len];
	struct spi_message msg;
	struct spi_transfer *tran;
	struct spi_device *spi = (struct spi_device*)dev->private_data;

	gpio_set_value(dev->cs_gpio, 0);	/*!< 片选拉低，选中 */
	tran = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	/* 第一次：发送要读取的寄存器地址 */
	txdata[0] = reg & ~0x80;			/*!< 写数据时寄存器地址BIT8置0 */
	tran->tx_buf = txdata;				/*!< 要发送的数据 */
	tran->len = 1;						/*!< 1个字节 */
	spi_message_init(&msg);				/*!< 初始化spi_message */
	spi_message_add_tail(tran, &msg);	/*!< spi_transfer添加到spi_message */
	ret = spi_sync(spi, &msg);			/*!< 同步发送 */

	/* 第二次：写入数据 */
	tran->tx_buf = buf;					/*!< 写入的数据 */
	tran->len = len;					/*!< 要写入的数据长度 */
	spi_message_init(&msg);				/*!< 初始化spi_message */
	spi_message_add_tail(tran, &msg);	/*!< spi_transfer添加到spi_message */
	ret = spi_sync(spi, &msg);			/*!< 同步发送 */

	kfree(tran);						/*!< 释放内存 */	
	gpio_set_value(dev->cs_gpio, 1);	/*!< 片选拉高，释放 */

	return ret;
}

/**=============================================================================
 * @brief           读取icm20608制定的寄存器
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static unsigned char icm20608_read_reg(icm20608_dev_t *dev, u8 reg)
{
	u8 data = 0;

	icm20608_read_regs(dev, reg, &data, 1);

	return data;
}

/**=============================================================================
 * @brief           向icm20608指定的寄存器写入值
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void icm20608_write_reg(icm20608_dev_t *dev, u8 reg, u8 data)
{
	u8 buf = data;
	icm20608_write_regs(dev, reg, &buf, 1);
}

/**=============================================================================
 * @brief           读取ICM20608原始数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void icm20608_read_raw_data(icm20608_dev_t *dev)
{
	unsigned char data[14] = {0};
	icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);

	dev->accel_x_adc = (signed short)((data[0] << 8) | data[1]);
	dev->accel_y_adc = (signed short)((data[2] << 8) | data[3]);
	dev->accel_z_adc = (signed short)((data[4] << 8) | data[5]);
	dev->temp_adc 	 = (signed short)((data[6] << 8) | data[7]);
	dev->gyro_x_adc  = (signed short)((data[8] << 8) | data[9]);
	dev->gyro_y_adc  = (signed short)((data[10] << 8) | data[11]);
	dev->gyro_z_adc  = (signed short)((data[12] << 8) | data[13]);

}

/**=============================================================================
 * @brief           icm20608内部寄存器初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void icm20608_reg_init(void)
{
	u8 value = 0;

	icm20608_write_reg(&icm20608dev, ICM20_PWR_MGMT_1, 0x80);
	mdelay(50);
	icm20608_write_reg(&icm20608dev, ICM20_PWR_MGMT_1, 0x01);
	mdelay(50);

	value = icm20608_read_reg(&icm20608dev, ICM20_WHO_AM_I);
	printk("ICM20608 ID = %#X\r\n", value);

	icm20608_write_reg(&icm20608dev, ICM20_SMPLRT_DIV, 0x00);
	icm20608_write_reg(&icm20608dev, ICM20_GYRO_CONFIG, 0x18);
	icm20608_write_reg(&icm20608dev, ICM20_ACCEL_CONFIG, 0x18);
	icm20608_write_reg(&icm20608dev, ICM20_CONFIG, 0x04);
	icm20608_write_reg(&icm20608dev, ICM20_ACCEL_CONFIG2, 0x04);
	icm20608_write_reg(&icm20608dev, ICM20_PWR_MGMT_2, 0x00);
	icm20608_write_reg(&icm20608dev, ICM20_LP_MODE_CFG, 0x00);
	icm20608_write_reg(&icm20608dev, ICM20_FIFO_EN, 0x00);

}

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:节点
 * @param[in]		filp:设备文件
 *
 * @return          none
 *============================================================================*/
static int icm20608_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &icm20608dev;

	return 0;
}

/**=============================================================================
 * @brief           从设备读取数据
 *
 * @param[in]       filp:设备文件
 * @param[out]		buf:用户空间的数据缓冲区
 * @param[in]		cnt:要读取的数据长度
 * @param[in]		off:相对于文件首地址的偏移
 *
 * @return          读取的字节数
 *============================================================================*/
static ssize_t icm20608_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	signed int data[7] = {0};
	long err = 0;

	icm20608_dev_t *dev = (icm20608_dev_t*)filp->private_data;

	icm20608_read_raw_data(dev);

	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;
	err = copy_to_user(buf, data, sizeof(data));
	
	return 0;
}

/**=============================================================================
 * @brief           关闭设备
 *
 * @param[in]       inode:节点
 * @param[in]		filp:设备文件
 *
 * @return          none
 *============================================================================*/
static int icm20608_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/**=============================================================================
 * @brief           spi驱动的probe函数
 *
 * @param[in]       spi:spi设备
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int icm20608_probe(struct spi_device *spi)
{
	int ret = 0;

	printk("icm20608 devices and driver mathced\r\n");

	/* 1. 构建设备号 */
	if (icm20608dev.major)
	{
		icm20608dev.devid = MKDEV(icm20608dev.major, 0);
		register_chrdev_region(icm20608dev.devid, ICM20608_CNT, ICM20608_NAME);
	}
	else
	{
		alloc_chrdev_region(&icm20608dev.devid, 0, ICM20608_CNT, ICM20608_NAME);
		icm20608dev.major = MAJOR(icm20608dev.devid);
	}

	/* 2. 注册设备 */
	cdev_init(&icm20608dev.cdev, &icm20608_ops);
	cdev_add(&icm20608dev.cdev, icm20608dev.devid, ICM20608_CNT);

	/* 3. 创建类 */
	icm20608dev.class = class_create(THIS_MODULE, ICM20608_NAME);
	if (IS_ERR(icm20608dev.class))
	{
		return PTR_ERR(icm20608dev.class);
	}
	
	/* 4. 创建设备 */
	icm20608dev.device = device_create(icm20608dev.class, NULL, icm20608dev.devid, 
										NULL, ICM20608_NAME);
	if (IS_ERR(icm20608dev.device))
	{
		return PTR_ERR(icm20608dev.device);
	}

	/* 获取设备树中CS片选信号 */
	icm20608dev.nd = of_find_node_by_path(
		"/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02010000");
	if (icm20608dev.nd == NULL)
	{
		printk("ecspi3 node not find!\r\n");
		return -EINVAL;
	}
	
	/* 获取设备树中断GPIO属性，得到BEEP所使用的BEEP编号 */
	icm20608dev.cs_gpio = of_get_named_gpio(icm20608dev.nd, "cs-gpio", 0);
	if (icm20608dev.cs_gpio < 0)
	{
		printk("can't get cs-gpio");
		return -EINVAL;
	}
	
	/* 设置GPIO1_IO20为输出，高电平 */
	ret = gpio_direction_output(icm20608dev.cs_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!\r\n");
	}
	
	/* 初始化 spi_device */
	spi->mode = SPI_MODE_0;
	spi_setup(spi);
	icm20608dev.private_data = spi;	/*!< 设置私有数据 */

	/* 初始化ICM20608内部寄存器 */
	icm20608_reg_init();

	printk("icm20608 driver probe finished\r\n");

	return 0;
}

/**=============================================================================
 * @brief           i2c驱动的remove函数
 *
 * @param[in]       client:i2c设备	
 *
 * @return          none
 *============================================================================*/
static int icm20608_remove(struct spi_device *spi)
{
	/* 删除设备 */
	cdev_del(&icm20608dev.cdev);
	unregister_chrdev_region(icm20608dev.devid, ICM20608_CNT);

	/* 注销类和设备 */
	device_destroy(icm20608dev.class, icm20608dev.devid);
	class_destroy(icm20608dev.class);

	return 0;
}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init _icm20608_init(void)
{
	int ret = 0;

	ret = spi_register_driver(&icm20608_driver);

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit _icm20608_exit(void)
{
	spi_unregister_driver(&icm20608_driver);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(_icm20608_init);
module_exit(_icm20608_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
