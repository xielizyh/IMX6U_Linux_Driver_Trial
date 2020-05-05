/**
  ******************************************************************************
  * @file			beep.c
  * @brief			beep function
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define MISCBEEP_NAME		"miscbeep"		/*!< 设备名 */
#define MISCBEEP_MINOR		144				/*!< 子设备号 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	struct device_node *nd; /*!< 设备节点 */
	int beep_gpio;			/*!< beep的GPIO编号 */
}miscbeep_dev_t;

/* Private variables ---------------------------------------------------------*/
static miscbeep_dev_t miscbeep;

/* Private function ----------------------------------------------------------*/
static int miscbeep_open(struct inode *inode, struct file *flip);
static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);

/* 设备操作函数 */
static struct file_operations miscbeep_fops = {
	.owner = THIS_MODULE,
	.open = miscbeep_open,
	.write = miscbeep_write,
};

/* misc设备结构体 */
static struct miscdevice beep_miscdev = {
	.minor = MISCBEEP_MINOR,
	.name = MISCBEEP_NAME,
	.fops = &miscbeep_fops,
};

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:传递给驱动的inode
 * @param[in]       filp:设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int miscbeep_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &miscbeep;

	return 0;
}

/**=============================================================================
 * @brief           向设备写数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;

	unsigned char databuf;
	miscbeep_dev_t *dev = filp->private_data;

	ret = copy_from_user(&databuf, buf, cnt);
	if (ret < 0)
	{
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	databuf?gpio_set_value(dev->beep_gpio, 0):gpio_set_value(dev->beep_gpio, 1);

	return 0;

}

/**=============================================================================
 * @brief           platform驱动的probe函数
 *
 * @param[in]       dev:platform设备
 *
 * @return          none
 *============================================================================*/
static int miscbeep_probe(struct platform_device *dev)
{
	int ret = 0;

	printk("beep driver and device was matched!\r\n");
	/* 设置beep所使用的GPIO */
	/* 1、获取设备节点：beep */
	miscbeep.nd = of_find_node_by_path("/beep");
	if (miscbeep.nd == NULL)
	{
		printk("beep node not find!\r\n");
		return -EINVAL;
	}

	/* 2、获取设备数中的GPIO属性，得到BEEP编号 */
	miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep-gpio", 0);
	if (miscbeep.beep_gpio < 0)
	{
		printk("can't get beep-gpio");
		return -EINVAL;
	}

	/* 3、设置GPIO5_IO01为输出，并且输出高电平，默认关闭BEEP */
	ret = gpio_direction_output(miscbeep.beep_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!\r\n");
	}	

	/* 注册misc设备驱动 */
	ret = misc_register(&beep_miscdev);
	if (ret < 0)
	{
		printk("misc device register failed!\r\n");
		return -EFAULT;
	}

	return 0;
}

/**=============================================================================
 * @brief           platform驱动的remove函数
 *
 * @param[in]       dev:platform设备
 *
 * @return          none
 *============================================================================*/
static int miscbeep_remove(struct platform_device *dev)
{
	/* 注销设备关闭LED */
	gpio_set_value(miscbeep.beep_gpio, 1);

	/* 注销misc设备驱动 */
	misc_deregister(&beep_miscdev);

	return 0;
}

/* 匹配列表 */
static const struct of_device_id beep_of_match[] = {
	{.compatible = "xli-beep"},
	{/*sentinel*/}
};

/* platform驱动结构体 */
static struct platform_driver beep_driver = {
	.driver = {
		.name = "imx6ul-beep",	/* 驱动名字 */
		.of_match_table = beep_of_match, /* 设备数匹配表 */
	},
	.probe = miscbeep_probe,
	.remove = miscbeep_remove,
};

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int __init miscbeep_init(void)
{
	return platform_driver_register(&beep_driver);
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit miscbeep_exit(void)
{
	platform_driver_unregister(&beep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xli");