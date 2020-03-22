/**
  ******************************************************************************
  * @file			newchrled.c
  * @brief			newchrled function
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define NEWCHRLED_CNT		1			/*!< 设备号个数 */
#define NEWCHRLED_NAME		"newchrled"	/*!< 设备名 */

/* 寄存器物理地址 */
#define CCM_CCGR1_BASE              (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE		(0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE		(0x020E02F4)
#define GPIO1_DR_BASE		        (0x0209C000)
#define GPIO1_GDIR_BASE		        (0x0209C004)

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	int major;				/*!< 主设备号 */
	int minor;				/*!< 次设备号 */
}newchrled_dev_t;

/* Private variables ---------------------------------------------------------*/
/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static newchrled_dev_t newchrled;

/* Private function ----------------------------------------------------------*/
static int led_open(struct inode *inode, struct file *flip);
static ssize_t led_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt);
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int led_release(struct inode *inode, struct file *flip);

static struct file_operations newchrled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = led_release,
};

/**=============================================================================
 * @brief           LED控制
 *
 * @param[in]       on:1---打开;0---关闭
 *
 * @return          none
 *============================================================================*/
static void led_switch(uint8_t on)
{
	uint32_t val = 0;

    val = readl(GPIO1_DR);
	if (on)
	{
		val &= ~(1<<3);
	}
	else
	{
		val |= (1<<3);
	}

    writel(val, GPIO1_DR);
}

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:传递给驱动的inode
 * @param[in]       filp:设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &newchrled;	/*!< 设置私有数据 */

	return 0;
}

/**=============================================================================
 * @brief           从设备读取数据
 *
 * @param[in]       filp:要打卡的设备文件
 * @param[in]		buf:返回给用户空间的数据缓冲区
 * @param[in]		cnt:要读取的数据长度
 * @param[in]		offt:相对于文件首地址的偏移
 *
 * @return          读取的字节数;如果为负值则读取失败
 *============================================================================*/
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

/**=============================================================================
 * @brief           向设备写数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	uint8_t databuf;

	retvalue = copy_from_user(&databuf, buf, cnt);
	if (retvalue < 0)
	{
		printk("kernel write failed!\r\n");
        return -EFAULT;
	}

    printk("databuf=%d\r\n", databuf);
	led_switch(databuf);

	return 0;
}

/**=============================================================================
 * @brief           释放设备
 *
 * @param[in]       filp:要关闭的设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init led_init(void)
{
	uint32_t val = 0;

	/* 1. 寄存器地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

	/* 2. 使能GPIO1时钟 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);
	val |= (3<<26);
	writel(val, IMX6U_CCM_CCGR1);

	/* 3. 设置复用功能，IO属性 */
	writel(5, SW_MUX_GPIO1_IO03);
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	/* 4. 设置GPIO1_IO03输出功能 */
	val = readl(GPIO1_DR);
	val &= ~(1 << 3);
	val |= (1 << 3);
	writel(val, GPIO1_GDIR);

	/* 5. 默认关闭LED */
	val = readl(GPIO1_DR);
	val |= (1 << 3);
	writel(val, GPIO1_DR);

	/* 注册字符设备驱动 */
	/* 1. 创建设备号 */
	if (newchrled.major)	/*!< 定义了设备号 */
	{
		newchrled.devid = MKDEV(newchrled.major, 0);
		register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);	/*!< 申请设备号 */
		newchrled.major = MAJOR(newchrled.devid);	/*!< 获取主设备号 */
		newchrled.minor = MINOR(newchrled.devid);	/*!< 获取次设备号 */
	}
	printk("newchrled.major=%d, newchrled.minor=%d\r\n", newchrled.major, newchrled.minor);

	/* 2. 初始化cdev */
	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &newchrled_fops);

	/* 3. 添加一个cdev */
	cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);

	/* 4. 创建类 */
	newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.class))
	{
		return PTR_ERR(newchrled.class);
	}

	/* 5. 创建设备 */
	newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.device))
	{
		return PTR_ERR(newchrled.device);
	}

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit led_exit(void)
{
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);
	
	/* 注销字符设备 */
	cdev_del(&newchrled.cdev);
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);

	device_destroy(newchrled.class, newchrled.devid);
	class_destroy(newchrled.class);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(led_init);
module_exit(led_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
