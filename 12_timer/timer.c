/**
  ******************************************************************************
  * @file			timer.c
  * @brief			timer function
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
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define TIMER_CNT		1			/*!< 设备号个数 */
#define TIMER_NAME		"timer"		/*!< 设备名 */
#define CLOSE_CMD		(_IO(0xEF, 0x1))	/*!< 关闭定时器 */
#define OPEN_CMD		(_IO(0xEF, 0x2))	/*!< 打开定时器 */
#define SETPERIOD_CMD	(_IO(0xEF, 0x3))	/*!< 设置定时器 */

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
	int timer_period;		/*!< 定时器周期，ms */
	struct timer_list timer;/*!< 定义一个定时器 */
	spinlock_t lock;		/*!< 定义自旋锁 */
}timer_dev_t;

/* Private variables ---------------------------------------------------------*/
/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

static timer_dev_t timer;

/* Private function ----------------------------------------------------------*/
static int timer_open(struct inode *inode, struct file *flip);
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.unlocked_ioctl = timer_unlocked_ioctl,
};

/**=============================================================================
 * @brief           led初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int led_init(void)
{
	int ret = 0;
	/* 设置LED所使用的GPIO */
	/* 1. 获取设备节点：timer */
	timer.nd = of_find_node_by_path("/gpioled");
	if (timer.nd == NULL)
	{
		printk("led node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("led node find!\r\n");
	}

	/* 2. 获取设备树中的GPIO属性，得到LED的GPIO编号 */
	timer.led_gpio = of_get_named_gpio(timer.nd, "led-gpio", 0);
	if (timer.led_gpio < 0)
	{
		printk("can't get led-gpio");
		return -EINVAL;
	}
	else
	{
		printk("led-gpio num = %d\r\n", timer.led_gpio);
	}

	/* 3. 设置GPIO1_IO03为输出，默认关闭LED */
	gpio_request(timer.led_gpio, "led");
	ret = gpio_direction_output(timer.led_gpio, 1);
	if (ret < 0)
	{
		printk("can't set gpio!\r\n");
	}

	return 0;
}

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:传递给驱动的inode
 * @param[in]       filp:设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int timer_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

    filp->private_data = &timer;	/*!< 设置私有数据 */

	timer.timer_period = 1000;	/*!< 周期1s */
	ret = led_init();
	
	return ret;
}

/**=============================================================================
 * @brief           ioctl函数
 *
 * @param[in]       filp:设备文件
 * @parma[in]		cmd:命令
 * @param[in]		arg:参数
 *
 * @return          none
 *============================================================================*/
static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	timer_dev_t *dev = (timer_dev_t*)filp->private_data;
	int timer_period = 0;
	unsigned long flags = 0;

	switch (cmd)
	{	
	case CLOSE_CMD:
		del_timer_sync(&dev->timer);
		break;
	case OPEN_CMD:
		spin_lock_irqsave(&dev->lock, flags);
		timer_period = dev->timer_period;
		spin_unlock_irqrestore(&dev->lock, flags);
		mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timer_period));
		break;
	case SETPERIOD_CMD:
		spin_lock_irqsave(&dev->lock, flags);
		dev->timer_period = arg;
		spin_unlock_irqrestore(&dev->lock, flags);
		mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));		
		break;

	default:
		break;
	}

	return 0;
}

/**=============================================================================
 * @brief           定时器回调函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void timer_callback(unsigned long arg)
{
	static int sta = 0;
	int timer_period = 0;
	unsigned long flags;
	timer_dev_t *dev = (timer_dev_t *)arg;
	
	gpio_set_value(dev->led_gpio, sta);
	sta = !sta;

	/*!< 重启定时器 */
	spin_lock_irqsave(&dev->lock, flags);
	timer_period = dev->timer_period;
	spin_unlock_irqrestore(&dev->lock, flags);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timer_period));

}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init timer_init(void)
{
	/* 初始化自旋锁 */
	spin_lock_init(&timer.lock);

	/* 注册字符设备驱动 */
	/* 1. 创建设备号 */
	if (timer.major)	/*!< 定义了设备号 */
	{
		timer.devid = MKDEV(timer.major, 0);
		register_chrdev_region(timer.devid, TIMER_CNT, TIMER_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&timer.devid, 0, TIMER_CNT, TIMER_NAME);	/*!< 申请设备号 */
		timer.major = MAJOR(timer.devid);	/*!< 获取主设备号 */
		timer.minor = MINOR(timer.devid);	/*!< 获取次设备号 */
	}
	printk("timer.major=%d, timer.minor=%d\r\n", timer.major, timer.minor);

	/* 2. 初始化cdev */
	timer.cdev.owner = THIS_MODULE;
	cdev_init(&timer.cdev, &timer_fops);

	/* 3. 添加一个cdev */
	cdev_add(&timer.cdev, timer.devid, TIMER_CNT);

	/* 4. 创建类 */
	timer.class = class_create(THIS_MODULE, TIMER_NAME);
	if (IS_ERR(timer.class))
	{
		return PTR_ERR(timer.class);
	}

	/* 5. 创建设备 */
	timer.device = device_create(timer.class, NULL, timer.devid, NULL, TIMER_NAME);
	if (IS_ERR(timer.device))
	{
		return PTR_ERR(timer.device);
	}

	/*6. 初始化timer */
	init_timer(&timer.timer);
	timer.timer.function = timer_callback;
	timer.timer.data = (unsigned long)&timer;

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit timer_exit(void)
{
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);
	
	/* 注销字符设备 */
	cdev_del(&timer.cdev);
	unregister_chrdev_region(timer.devid, TIMER_CNT);

	device_destroy(timer.class, timer.devid);
	class_destroy(timer.class);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(timer_init);
module_exit(timer_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
