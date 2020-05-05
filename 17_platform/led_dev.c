/**
  ******************************************************************************
  * @file			led_dev.c
  * @brief			led_dev function
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
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
/* 寄存器地址定义 */
#define CCM_CCGR1_BASE				(0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE		(0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0x020E02F4)
#define GPIO1_DR_BASE				(0x0209C000)
#define GPIO1_GDIR_BASE				(0x0209C004)
#define REGISTER_LENGTH				4

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* 设备资源信息 */
static struct resource led_resources[] = {
	[0] = {
		.start = CCM_CCGR1_BASE,
		.end = (CCM_CCGR1_BASE + REGISTER_LENGTH - 1),
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = SW_MUX_GPIO1_IO03_BASE,
		.end = (SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = SW_PAD_GPIO1_IO03_BASE,
		.end = (SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1),
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = GPIO1_DR_BASE,
		.end = (GPIO1_DR_BASE + REGISTER_LENGTH - 1),
		.flags = IORESOURCE_MEM,
	},			
	[4] = {
		.start = GPIO1_GDIR_BASE,
		.end = (GPIO1_GDIR_BASE + REGISTER_LENGTH - 1),
		.flags = IORESOURCE_MEM,
	},
};

static void led_release(struct device *dev);

/* platform设备结构体 */
static struct platform_device led_dev = {
	.name = "imx6ul-led",
	.id = -1,
	.dev = {
		.release = &led_release,
	},
	.num_resources = ARRAY_SIZE(led_resources),
	.resource = led_resources,
};

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           释放platform设备模块
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void led_release(struct device *dev)
{
	printk("led device released!\r\n");
}

/**=============================================================================
 * @brief           设备模块加载
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int __init led_dev_init(void)
{
	return platform_device_register(&led_dev);
}

/**=============================================================================
 * @brief           设备模块注销
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit led_dev_exit(void)
{
	platform_device_unregister(&led_dev);
}

/**
 *      @brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(led_dev_init);
module_exit(led_dev_exit);

/**
 *      @brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");