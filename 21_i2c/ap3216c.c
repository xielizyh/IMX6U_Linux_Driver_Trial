/**
  ******************************************************************************
  * @file			ap3216c.c
  * @brief			ap3216c function
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
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "ap3216c.h"

/* Private constants ---------------------------------------------------------*/
#define AP3216C_CNT				1			/*!< 设备号个数 */
#define AP3216C_NAME			"ap3216c"	/*!< 设备名 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* ap3216c设备结构体 */
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	struct device_node *nd; /*!< 设备节点 */
	int major;				/*!< 主设备号 */
	void *private_data;		/*!< 私有数据 */
	unsigned short ir, als, ps;	/*!< 光传感数据 */
}ap3216c_dev_t;

/* Private variables ---------------------------------------------------------*/
static ap3216c_dev_t ap3216cdev;

/* 传统匹配方式列表 */
static const struct i2c_device_id ap3216c_id[] = {
	{"xli,ap3216c", 0},
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id ap3216c_of_match[] = {
	{ .compatible = "xli,ap3216c" },
	{ /* sentinel */}
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ap3216c_remove(struct i2c_client *client);

/* i2c驱动结构体 */
static struct i2c_driver ap3216c_driver = {
	.probe = ap3216c_probe,
	.remove = ap3216c_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "ap3216c",
		.of_match_table = ap3216c_of_match,
	},
	.id_table = ap3216c_id,
};

static int ap3216c_open(struct inode *inode, struct file *filp);
static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off);
static int ap3216c_release(struct inode *inode, struct file *filp);

/* ap3216c操作函数 */
static const struct file_operations ap3216c_ops = {
	.owner = THIS_MODULE,
	.open = ap3216c_open,
	.read = ap3216c_read,
	.release = ap3216c_release,
};

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           从ap3216c读取多个寄存器数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int ap3216c_read_regs(ap3216c_dev_t *dev, u8 reg, void *val, int len)
{
	int ret = 0;

	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client*)dev->private_data;

	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;	/*!< ap3216c地址 */
	msg[0].flags = 0;	/*!< 标记为发送数据 */
	msg[0].buf = &reg;	/*!< 读取的首地址 */
	msg[0].len = 1;		/*!< reg长度 */

	/* msg[1]为要读取数据 */
	msg[1].addr = client->addr;	/*!< ap3216c地址 */
	msg[1].flags = I2C_M_RD;	/*!< 标记为读取数据 */
	msg[1].buf = val;	/*!< 读取数据的缓冲区 */
	msg[1].len = len;		/*!< 读取数据的长度 */	

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret == 2)	ret = 0;
	else ret = -EREMOTEIO;

	return ret;
}

/**=============================================================================
 * @brief           向ap3216c多个寄存器写入数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static s32 ap3216c_write_regs(ap3216c_dev_t *dev, u8 reg, u8 *buf, u8 len)
{
	u8 byte[256] = {0};
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client*)dev->private_data;

	byte[0] = reg;	/*!< 寄存器首地址 */
	memcpy(&byte[1], buf, len);	/*!< 拷贝数据 */
	
	msg.addr = client->addr;	/*!< ap3216c地址 */
	msg.flags = 0;	/*!< 标记为写数据 */
	msg.buf = byte;	/*!< 要写入的数据缓冲区 */
	msg.len = len + 1;		/*!< 要写入数据的长度 */	
	
	return i2c_transfer(client->adapter, &msg, 1);
}

/**=============================================================================
 * @brief           读取ap3216c制定的寄存器
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static unsigned char ap3216c_read_reg(ap3216c_dev_t *dev, u8 reg)
{
	u8 data = 0;

	ap3216c_read_regs(dev, reg, &data, 1);

	return data;

#if 0
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	return i2c_smbus_read_byte_data(client, reg);
#endif
}

/**=============================================================================
 * @brief           向ap3216c指定的寄存器写入值
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void ap3216c_write_reg(ap3216c_dev_t *dev, u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	ap3216c_write_regs(dev, reg, &buf, 1);
}

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:节点
 * @param[in]		filp:设备文件
 *
 * @return          none
 *============================================================================*/
static int ap3216c_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ap3216cdev;

	/* 初始化ap3216c */
	ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04);
	mdelay(50);	/*!< ap3216c复位至少10ms */
	ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x03);

	return 0;
}

/**=============================================================================
 * @brief           从ap3216c读取传感数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void ap3216c_read_ir_als_ps(ap3216c_dev_t *dev)
{
	unsigned char i = 0;
	unsigned char buf[6] = {0};

	for (i = 0; i < 6; i++)
	{
		buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
	}

	/* IR */
	if (buf[0] & 0x80)	/*!< IR_OF位为1，数据无效 */
	{
		dev->ir = 0;
	}
	else
	{
		dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] * 0x03);
	}
	
	/* ALS */
	dev->als = ((unsigned short)buf[3] << 8) | buf[2];

	/* PS */
	if (buf[0] & 0x80)	/*!< IR_OF位为1，数据无效 */
	{
		dev->ps = 0;
	}
	else
	{
		dev->ps = ((unsigned short)buf[5] & 0x3F) | (buf[4] * 0x0F);
	}	
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
static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	short data[3] = {0};
	long err = 0;

	ap3216c_dev_t *dev = (ap3216c_dev_t*)filp->private_data;

	ap3216c_read_ir_als_ps(dev);

	data[0] = dev->ir;
	data[1] = dev->als;
	data[2] = dev->ps;
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
static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/**=============================================================================
 * @brief           i2c驱动的probe函数
 *
 * @param[in]       client:i2c设备
 * @param[in]		id:i2c设备ID
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	/* 1. 构建设备号 */
	if (ap3216cdev.major)
	{
		ap3216cdev.devid = MKDEV(ap3216cdev.major, 0);
		register_chrdev_region(ap3216cdev.devid, AP3216C_CNT, AP3216C_NAME);
	}
	else
	{
		alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT, AP3216C_NAME);
		ap3216cdev.major = MAJOR(ap3216cdev.devid);
	}

	/* 2. 注册设备 */
	cdev_init(&ap3216cdev.cdev, &ap3216c_ops);
	cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);

	/* 3. 创建类 */
	ap3216cdev.class = class_create(THIS_MODULE, AP3216C_NAME);
	if (IS_ERR(ap3216cdev.class))
	{
		return PTR_ERR(ap3216cdev.class);
	}
	
	/* 4. 创建设备 */
	ap3216cdev.device = device_create(ap3216cdev.class, NULL, ap3216cdev.devid, 
										NULL, AP3216C_NAME);
	if (IS_ERR(ap3216cdev.device))
	{
		return PTR_ERR(ap3216cdev.device);
	}

	ap3216cdev.private_data = client;

	return 0;
}

/**=============================================================================
 * @brief           i2c驱动的remove函数
 *
 * @param[in]       client:i2c设备	
 *
 * @return          none
 *============================================================================*/
static int ap3216c_remove(struct i2c_client *client)
{
	/* 删除设备 */
	cdev_del(&ap3216cdev.cdev);
	unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

	/* 注销类和设备 */
	device_destroy(ap3216cdev.class, ap3216cdev.devid);
	class_destroy(ap3216cdev.class);

	return 0;
}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init _ap3216c_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ap3216c_driver);

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit _ap3216c_exit(void)
{
	i2c_del_driver(&ap3216c_driver);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(_ap3216c_init);
module_exit(_ap3216c_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
