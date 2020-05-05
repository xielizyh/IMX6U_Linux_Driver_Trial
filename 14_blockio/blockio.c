/**
  ******************************************************************************
  * @file			blockio.c
  * @brief			blockio function
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
#include <linux/of_irq.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define KEYIRQ_CNT		1					/*!< 设备号个数 */
#define KEYIRQ_NAME		"blockio"			/*!< 设备名 */
#define KEY0_VALUE       0x01            	/*!< 按键值 */
#define INVALID_KEY     0xFF           		/*!< 无效值 */
#define KEY_NUM			1					/*!< 按键数量 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct {
	int gpio;				/*!< gpio */
	int irqnum;				/*!< 中断号 */
	unsigned char value;	/*!< 按键值 */
	char name[10];			/*!< 名字 */
	irqreturn_t (*handler)(int, void*);	/*!< 中断服务器函数 */
}keyirq_desc_t;

typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	int major;				/*!< 主设备号 */
	int minor;				/*!< 次设备号 */
	struct device_node *nd; /*!< 设备节点 */
	atomic_t key_value;		/*!< 按键值 */
	atomic_t release_key;	/*!< 释放的按键 */
	struct timer_list timer;/*!< 定义一个定时器 */
	keyirq_desc_t desc[KEY_NUM];	/*!< 按键描述数组 */
	unsigned char cur_key;	/*!< 当前按键号 */

	wait_queue_head_t r_wait;	/*!< 读等待队列头 */
}keyirq_dev_t;

/* Private variables ---------------------------------------------------------*/
static keyirq_dev_t keyirq;

/* Private function ----------------------------------------------------------*/
static int keyirq_open(struct inode *inode, struct file *flip);
static ssize_t keyirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt);
static void timer_callback(unsigned long arg);

static struct file_operations keyirq_fops = {
	.owner = THIS_MODULE,
	.open = keyirq_open,
	.read = keyirq_read,
};

/**=============================================================================
 * @brief           按键中断服务函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static irqreturn_t key0_handler(int irq, void *arg)
{
	keyirq_dev_t *dev = (keyirq_dev_t*)arg;

	dev->cur_key = 0;
	dev->timer.data = (unsigned long)arg;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	/*!< 消抖 */

	return IRQ_RETVAL(IRQ_HANDLED);
}

/**=============================================================================
 * @brief           KeyIO初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int key_gpio_init(void)
{
	int ret = 0;
	uint8_t i = 0;	

	/* 设置KEY所使用的GPIO */
	/* 1. 获取设备节点：keyirq */
	keyirq.nd = of_find_node_by_path("/key");
	if (keyirq.nd == NULL)
	{
		printk("key node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("key node find!\r\n");
	}

	/* 2. 获取设备树中的GPIO属性，得到KEY的GPIO编号 */
	for (i = 0; i < KEY_NUM; i++)
	{
		keyirq.desc[i].gpio = of_get_named_gpio(keyirq.nd, "key-gpio", i);
		if (keyirq.desc[i].gpio < 0)
		{
			printk("can't get key-gpio[%d]\r\n", i);
		}
	}

	/* 3. 设置KEY使用IO，并且设置中断模式 */
	for (i = 0; i < KEY_NUM; i++)
	{
		memset(keyirq.desc[i].name, 0, sizeof(keyirq.desc[i].name));
		sprintf(keyirq.desc[i].name, "KEY%d", i);
		gpio_request(keyirq.desc[i].gpio, keyirq.desc[i].name);
		gpio_direction_input(keyirq.desc[i].gpio);
		keyirq.desc[i].irqnum = irq_of_parse_and_map(keyirq.nd, i);
#if 0
		keyirq.desc[i].irqnum = gpio_to_irq(keyirq.desc[i].gpio);
#endif
		printk("key%d:gpio=%d, irqnum=%d\r\n", i, keyirq.desc[i].gpio, keyirq.desc[i].irqnum);
	}
	keyirq.desc[0].handler = key0_handler;
	keyirq.desc[0].value = KEY0_VALUE;

	for (i = 0; i < KEY_NUM; i++)
	{
		ret = request_irq(	keyirq.desc[i].irqnum,
							keyirq.desc[i].handler,
							IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
							keyirq.desc[i].name,
							&keyirq	);

		if (ret < 0)
		{
			printk("irq %d request failed!\r\n", keyirq.desc[i].irqnum);
			return -EFAULT;
		}
	}

	/* 创建定时器 */
	init_timer(&keyirq.timer);
	keyirq.timer.function = timer_callback;

	/* 初始化等待队列头 */
	init_waitqueue_head(&keyirq.r_wait);

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
static int keyirq_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &keyirq;	/*!< 设置私有数据 */
	
	return 0;
}

/**=============================================================================
 * @brief           从设备读取数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static ssize_t keyirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char key_value = 0;
	unsigned char release_key = 0;
	keyirq_dev_t *dev = (keyirq_dev_t*)filp->private_data;

#if 0
	ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->release_key));
	if (ret)
	{
		goto wait_error;
	}
#endif

	DECLARE_WAITQUEUE(wait, current);	/*!< 定义一个等待队列 */
	if (atomic_read(&dev->release_key) == 0)
	{
		add_wait_queue(&dev->r_wait, &wait);	/*!< 添加到等待队列头 */
		__set_current_state(TASK_INTERRUPTIBLE);	/*!< 设置任务状态 */
		schedule();	/*!< 任务切换 */
		if (signal_pending(current))	/*!< 判断是否由信号引起的唤醒 */
		{
			ret = -ERESTARTSYS;
			goto wait_error;
		}
	}

	remove_wait_queue(&dev->r_wait, &wait);

	key_value = atomic_read(&dev->key_value);
	release_key = atomic_read(&dev->release_key);

	if (release_key)
	{
			if (key_value & 0x80)
			{
					key_value &= ~0x80;
					ret = copy_to_user(buf, &key_value, sizeof(key_value));
			}
			else
			{
					return -EINVAL;
			}
			atomic_set(&dev->release_key, 0);
	}
	else
	{
			return -EINVAL;
	}

	return 0;	

wait_error:
	set_current_state(TASK_RUNNING);	/*!< 设置任务为运行态 */
	remove_wait_queue(&dev->r_wait, &wait);	/*!< 从等待队列移除 */
	return ret;
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
	unsigned char num = 0;
	keyirq_desc_t *key_desc = NULL;
	keyirq_dev_t *dev = (keyirq_dev_t*)arg;

	num = dev->cur_key;
	key_desc = &dev->desc[num];
	if (!gpio_get_value(key_desc->gpio))
	{
		atomic_set(&dev->key_value, key_desc->value);
	}
	else
	{
		atomic_set(&dev->key_value, 0x80 | key_desc->value);
		atomic_set(&dev->release_key, 1);	/*!< 标记松开按键 */
	}

	/* 唤醒进程 */
	if (atomic_read(&dev->release_key))
	{
		wake_up_interruptible(&dev->r_wait);
	}

}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init keyirq_init(void)
{
	/* 注册字符设备驱动 */
	/* 1. 创建设备号 */
	if (keyirq.major)	/*!< 定义了设备号 */
	{
		keyirq.devid = MKDEV(keyirq.major, 0);
		register_chrdev_region(keyirq.devid, KEYIRQ_CNT, KEYIRQ_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&keyirq.devid, 0, KEYIRQ_CNT, KEYIRQ_NAME);	/*!< 申请设备号 */
		keyirq.major = MAJOR(keyirq.devid);	/*!< 获取主设备号 */
		keyirq.minor = MINOR(keyirq.devid);	/*!< 获取次设备号 */
	}
	printk("keyirq.major=%d, keyirq.minor=%d\r\n", keyirq.major, keyirq.minor);

	/* 2. 初始化cdev */
	keyirq.cdev.owner = THIS_MODULE;
	cdev_init(&keyirq.cdev, &keyirq_fops);

	/* 3. 添加一个cdev */
	cdev_add(&keyirq.cdev, keyirq.devid, KEYIRQ_CNT);

	/* 4. 创建类 */
	keyirq.class = class_create(THIS_MODULE, KEYIRQ_NAME);
	if (IS_ERR(keyirq.class))
	{
		return PTR_ERR(keyirq.class);
	}

	/* 5. 创建设备 */
	keyirq.device = device_create(keyirq.class, NULL, keyirq.devid, NULL, KEYIRQ_NAME);
	if (IS_ERR(keyirq.device))
	{
		return PTR_ERR(keyirq.device);
	}

	/*6. 初始化keyirq */
	atomic_set(&keyirq.key_value, INVALID_KEY);
	atomic_set(&keyirq.release_key, 0);
	key_gpio_init();

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit keyirq_exit(void)
{
	unsigned char i = 0;

	/* 删除定时器 */
	del_timer_sync(&keyirq.timer);

	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++)
	{
		free_irq(keyirq.desc[i].irqnum, &keyirq);
	}

	/* 注销字符设备 */
	cdev_del(&keyirq.cdev);
	unregister_chrdev_region(keyirq.devid, KEYIRQ_CNT);

	device_destroy(keyirq.class, keyirq.devid);
	class_destroy(keyirq.class);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(keyirq_init);
module_exit(keyirq_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
