/**
  ******************************************************************************
  * @file			spinlock.c
  * @brief			spinlock function
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define GPIOLED_CNT		1			/*!< 设备号个数 */
#define GPIOLED_NAME		"gpioled"	/*!< 设备名 */

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
	int led_gpio;			/*!< LED的GPIO编号 */
	int dev_sta;			/*!< 设备状态 */
	spinlock_t lock;		/*!< 自旋锁 */
}gpioled_dev_t;

/* Private variables ---------------------------------------------------------*/
/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static gpioled_dev_t gpioled;

/* Private function ----------------------------------------------------------*/
static int led_open(struct inode *inode, struct file *flip);
static ssize_t led_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt);
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int led_release(struct inode *inode, struct file *flip);

static struct file_operations gpioled_fops = {
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
	unsigned long flags;

    filp->private_data = &gpioled;	/*!< 设置私有数据 */

	spin_lock_irqsave(&gpioled.lock, flags);
	if (gpioled.dev_sta)	/*!< 设备被使用 */
	{
		spin_unlock_irqrestore(&gpioled.lock, flags);	/*!< 解锁 */
		return -EBUSY;
	}
	gpioled.dev_sta++;
	spin_unlock_irqrestore(&gpioled.lock, flags);	/*!< 解锁 */

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
	gpioled_dev_t *dev = filp->private_data;

	retvalue = copy_from_user(&databuf, buf, cnt);
	if (retvalue < 0)
	{
		printk("kernel write failed!\r\n");
        return -EFAULT;
	}

    printk("databuf=%d\r\n", databuf);

	/* 开关LED */
	databuf?gpio_set_value(dev->led_gpio, 0):gpio_set_value(dev->led_gpio, 1);

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
	unsigned long flags;
	gpioled_dev_t *dev = filp->private_data;
	
	/* 关闭驱动文件dev_sta减一 */
	spin_lock_irqsave(&dev->lock, flags);	/*!< 上锁 */
	if (dev->dev_sta)
	{
		dev->dev_sta--;
	}
	spin_unlock_irqrestore(&dev->lock, flags);	/*!< 解锁 */

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
#if 0	
	uint32_t val = 0;
	int ret = 0;
	const char *str;
	u32 regdata[14] = {0};
	struct property *proper;

	/* 获取设备树中的属性数据 */
	/* a. 获取设备节点:xli-led */
	gpioled.nd = of_find_node_by_path("/gpioled");
	if (gpioled.nd == NULL)
	{
		printk("gpioled node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("gpioled node find!\r\n");
	}
	/* b. 获取compatible属性内容 */
	proper = of_find_property(gpioled.nd, "compatible", NULL);
	if (proper == NULL)
	{
		printk("compatible property find failed\r\n");
	}
	else
	{
		printk("compatible = %s\r\n", (char*)proper->value);
	}
	/* c. 获取status属性内容 */
	ret = of_property_read_string(gpioled.nd, "status", &str);
	if (ret < 0)
	{
		printk("status read failed!\r\n");
	}
	else
	{
		printk("status = %s\r\n", str);
	}
	/* d. 获取reg属性内容 */
	ret = of_property_read_u32_array(gpioled.nd, "reg", regdata, 10);
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
	IMX6U_CCM_CCGR1 = of_iomap(gpioled.nd, 0);
	SW_MUX_GPIO1_IO03 = of_iomap(gpioled.nd, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(gpioled.nd, 2);
	GPIO1_DR = of_iomap(gpioled.nd, 3);
	GPIO1_GDIR = of_iomap(gpioled.nd, 4);
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
#endif

	int ret = 0;

	/* 初始化自旋锁 */
	spin_lock_init(&gpioled.lock);

	/* 设置LED所使用的GPIO */
	/* 1. 获取设备节点：gpioled */
	gpioled.nd = of_find_node_by_path("/gpioled");
	if (gpioled.nd == NULL)
	{
		printk("gpioled node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("gpioled node find!\r\n");
	}

	/* 2. 获取设备树中的GPIO属性，得到LED的GPIO编号 */
	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
	if (gpioled.led_gpio < 0)
	{
		printk("can't get led-gpio");
		return -EINVAL;
	}
	else
	{
		printk("led-gpio num = %d\r\n", gpioled.led_gpio);
	}

	/* 3. 设置GPIO1_IO03为输出，默认关闭LED */
	ret = gpio_direction_output(gpioled.led_gpio, 1);

	/* 注册字符设备驱动 */
	/* 1. 创建设备号 */
	if (gpioled.major)	/*!< 定义了设备号 */
	{
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);	/*!< 申请设备号 */
		gpioled.major = MAJOR(gpioled.devid);	/*!< 获取主设备号 */
		gpioled.minor = MINOR(gpioled.devid);	/*!< 获取次设备号 */
	}
	printk("gpioled.major=%d, gpioled.minor=%d\r\n", gpioled.major, gpioled.minor);

	/* 2. 初始化cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);

	/* 3. 添加一个cdev */
	cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

	/* 4. 创建类 */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class))
	{
		return PTR_ERR(gpioled.class);
	}

	/* 5. 创建设备 */
	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device))
	{
		return PTR_ERR(gpioled.device);
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
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);
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
