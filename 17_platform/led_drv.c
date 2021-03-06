/**
  ******************************************************************************
  * @file			leddev.c
  * @brief			leddev function
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define GPIOLED_CNT		1			/*!< 设备号个数 */
#define GPIOLED_NAME	"platled"	/*!< 设备名 */

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
}led_dev_t;

/* Private variables ---------------------------------------------------------*/
/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static led_dev_t leddev;

/* Private function ----------------------------------------------------------*/
static int led_open(struct inode *inode, struct file *flip);
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int led_probe(struct platform_device *dev);
static int led_remove(struct platform_device *dev);

static struct file_operations leddev_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
};

/* paltform驱动结构体 */
static struct platform_driver led_driver = {
	.driver = {
		.name = "imx6ul-led",	/*!< 驱动名字，用于和设备匹配 */
	},
	.probe = led_probe,
	.remove = led_remove,
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
    filp->private_data = &leddev;	/*!< 设置私有数据 */

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

	/* 开关LED */
	databuf?led_switch(0):led_switch(1);

	return 0;
}

/**=============================================================================
 * @brief           platform驱动的probe函数
 *
 * @param[in]       dev:platform设备
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int led_probe(struct platform_device *dev)
{
	int i = 0;
	int ressize[5];
	u32 val = 0;
	struct resource *led_resource[5];

	printk("led driver and device has matched!\r\n");
	for (i = 0; i < 5; i++)
	{
		led_resource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
		if (!led_resource[i])
		{
			dev_err(&dev->dev, "NO MEM resource[%d] for always on\n", i);
			return -ENXIO;
		}
		ressize[i] = resource_size(led_resource[i]);
	}
	
	
	/* 1. 寄存器地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(led_resource[0]->start, ressize[0]);
	SW_MUX_GPIO1_IO03 = ioremap(led_resource[1]->start, ressize[1]);
	SW_PAD_GPIO1_IO03 = ioremap(led_resource[2]->start, ressize[2]);
	GPIO1_DR = ioremap(led_resource[3]->start, ressize[3]);
	GPIO1_GDIR = ioremap(led_resource[4]->start, ressize[4]);

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
	if (leddev.major)	/*!< 定义了设备号 */
	{
		leddev.devid = MKDEV(leddev.major, 0);
		register_chrdev_region(leddev.devid, GPIOLED_CNT, GPIOLED_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&leddev.devid, 0, GPIOLED_CNT, GPIOLED_NAME);	/*!< 申请设备号 */
		leddev.major = MAJOR(leddev.devid);	/*!< 获取主设备号 */
		leddev.minor = MINOR(leddev.devid);	/*!< 获取次设备号 */
	}
	printk("leddev.major=%d, leddev.minor=%d\r\n", leddev.major, leddev.minor);

	/* 2. 初始化cdev */
	leddev.cdev.owner = THIS_MODULE;
	cdev_init(&leddev.cdev, &leddev_fops);

	/* 3. 添加一个cdev */
	cdev_add(&leddev.cdev, leddev.devid, GPIOLED_CNT);

	/* 4. 创建类 */
	leddev.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(leddev.class))
	{
		return PTR_ERR(leddev.class);
	}

	/* 5. 创建设备 */
	leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(leddev.device))
	{
		return PTR_ERR(leddev.device);
	}

	return 0;
}

/**=============================================================================
 * @brief           移除platform驱动时会执行该函数
 *
 * @param[in]       dev:platform设备
 *
 * @return          none
 *============================================================================*/
static int led_remove(struct platform_device *dev)
{
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);
	
	/* 注销字符设备 */
	cdev_del(&leddev.cdev);
	unregister_chrdev_region(leddev.devid, GPIOLED_CNT);

	device_destroy(leddev.class, leddev.devid);
	class_destroy(leddev.class);	

	return 0;
}

/**=============================================================================
 * @brief           驱动模块加载函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int __init led_drv_init(void)
{
	return platform_driver_register(&led_driver);
}

/**=============================================================================
 * @brief           驱动模块卸载函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit led_drv_exit(void)
{
	platform_driver_unregister(&led_driver);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(led_drv_init);
module_exit(led_drv_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
