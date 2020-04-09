/**
  ******************************************************************************
  * @file			dtsled.c
  * @brief			dtsled function
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define DTSLED_CNT		1			/*!< 设备号个数 */
#define DTSLED_NAME		"dtsled"	/*!< 设备名 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	int major;				/*!< 主设备号 */
	int minor;				/*!< 次设备号 */
	struct device_node *nd; /*!< 设备节点 */
}dtsled_dev_t;

/* Private variables ---------------------------------------------------------*/
/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static dtsled_dev_t dtsled;

/* Private function ----------------------------------------------------------*/
static int led_open(struct inode *inode, struct file *flip);
static ssize_t led_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt);
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int led_release(struct inode *inode, struct file *flip);

static struct file_operations dtsled_fops = {
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
    filp->private_data = &dtsled;	/*!< 设置私有数据 */

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
	int ret = 0;
	const char *str;
	u32 regdata[14] = {0};
	struct property *proper;

	/* 获取设备树中的属性数据 */
	/* a. 获取设备节点:xli-led */
	dtsled.nd = of_find_node_by_path("/xliled");
	if (dtsled.nd == NULL)
	{
		printk("xliled node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("xliled node find!\r\n");
	}
	/* b. 获取compatible属性内容 */
	proper = of_find_property(dtsled.nd, "compatible", NULL);
	if (proper == NULL)
	{
		printk("compatible property find failed\r\n");
	}
	else
	{
		printk("compatible = %s\r\n", (char*)proper->value);
	}
	/* c. 获取status属性内容 */
	ret = of_property_read_string(dtsled.nd, "status", &str);
	if (ret < 0)
	{
		printk("status read failed!\r\n");
	}
	else
	{
		printk("status = %s\r\n", str);
	}
	/* d. 获取reg属性内容 */
	ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
	if (ret < 0)
	{
		printk("reg property read failed!\r\n");
	}
	else
	{
		u8 i = 0;
		printk("reg data:\r\n");
		for (i = 0; i < 10; i++)
		{
			printk("%#x ", regdata[i]);
		}
		printk("\r\n");
	}

#if 0
	/* 1. 寄存器地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
	SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
	SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
	GPIO1_DR = ioremap(regdata[6], regdata[7]);
	GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#else
	IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
	GPIO1_DR = of_iomap(dtsled.nd, 3);
	GPIO1_GDIR = of_iomap(dtsled.nd, 4);
#endif

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
	if (dtsled.major)	/*!< 定义了设备号 */
	{
		dtsled.devid = MKDEV(dtsled.major, 0);
		register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);	/*!< 申请设备号 */
		dtsled.major = MAJOR(dtsled.devid);	/*!< 获取主设备号 */
		dtsled.minor = MINOR(dtsled.devid);	/*!< 获取次设备号 */
	}
	printk("dtsled.major=%d, dtsled.minor=%d\r\n", dtsled.major, dtsled.minor);

	/* 2. 初始化cdev */
	dtsled.cdev.owner = THIS_MODULE;
	cdev_init(&dtsled.cdev, &dtsled_fops);

	/* 3. 添加一个cdev */
	cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

	/* 4. 创建类 */
	dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
	if (IS_ERR(dtsled.class))
	{
		return PTR_ERR(dtsled.class);
	}

	/* 5. 创建设备 */
	dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
	if (IS_ERR(dtsled.device))
	{
		return PTR_ERR(dtsled.device);
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
	cdev_del(&dtsled.cdev);
	unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

	device_destroy(dtsled.class, dtsled.devid);
	class_destroy(dtsled.class);
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
