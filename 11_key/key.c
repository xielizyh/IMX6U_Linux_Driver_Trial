/**
  ******************************************************************************
  * @file			key.c
  * @brief			key function
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
#define KEY_CNT			1			/*!< 设备号个数 */
#define KEY_NAME		"key"	/*!< 设备名 */
#define	KEY_VALUE		0xF0		/*!< 按键值 */
#define INVALID_KEY		0x00		/*!< 无效值 */

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
	int key_gpio;			/*!< key的GPIO编号 */
	atomic_t key_value;		/*!< 按键值 */
}key_dev_t;

/* Private variables ---------------------------------------------------------*/
static key_dev_t keydev;

/* Private function ----------------------------------------------------------*/
static int key_open(struct inode *inode, struct file *flip);
static ssize_t key_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt);
static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int key_release(struct inode *inode, struct file *flip);

static struct file_operations keydev_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.read = key_read,
	.write = key_write,
	.release = key_release,
};

/**=============================================================================
 * @brief           初始化按键IO
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int key_gpio_init(void)
{
	/* 设置KEY所使用的GPIO */
	/* 1. 获取设备节点：key */
	keydev.nd = of_find_node_by_path("/key");
	if (keydev.nd == NULL)
	{
		printk("key node not find!\r\n");
		return -EINVAL;
	}
	else
	{
		printk("key node find!\r\n");
	}

	/* 2. 获取设备树中的GPIO属性，得到KEY的GPIO编号 */
	keydev.key_gpio = of_get_named_gpio(keydev.nd, "key-gpio", 0);
	if (keydev.key_gpio < 0)
	{
		printk("can't get key-gpio\r\n");
		return -EINVAL;
	}
	else
	{
		printk("key-gpio num = %d\r\n", keydev.key_gpio);
	}

	/* 3. 设置KEY输入 */
	gpio_request(keydev.key_gpio, "key0");	/*!< 请求IO */
	gpio_direction_input(keydev.key_gpio);

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
static int key_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

    filp->private_data = &keydev;	/*!< 设置私有数据 */

	ret = key_gpio_init();
	if (ret < 0)
	{
		return ret;
	}

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
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char value = 0;
	key_dev_t *dev = filp->private_data;

	if (gpio_get_value(dev->key_gpio) == 0)
	{
		while (!gpio_get_value(dev->key_gpio));	/*!< 等待按键释放 */
		atomic_set(&dev->key_value, KEY_VALUE);
	}
	else
	{
		atomic_set(&dev->key_value, INVALID_KEY);
	}

	value = atomic_read(&dev->key_value);
	ret = copy_to_user(buf, &value, sizeof(value));

	return 0;
}

/**=============================================================================
 * @brief           向设备写数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

/**=============================================================================
 * @brief           释放设备
 *
 * @param[in]       filp:要关闭的设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int key_release(struct inode *inode, struct file *filp)
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
static int __init _key_init(void)
{
	/* 初始化原子变量 */
	atomic_set(&keydev.key_value, INVALID_KEY);

	/* 注册字符设备驱动 */
	/* 1. 创建设备号 */
	if (keydev.major)	/*!< 定义了设备号 */
	{
		keydev.devid = MKDEV(keydev.major, 0);
		register_chrdev_region(keydev.devid, KEY_CNT, KEY_NAME);
	}
	else					/*!< 未定义设备号 */
	{
		alloc_chrdev_region(&keydev.devid, 0, KEY_CNT, KEY_NAME);	/*!< 申请设备号 */
		keydev.major = MAJOR(keydev.devid);	/*!< 获取主设备号 */
		keydev.minor = MINOR(keydev.devid);	/*!< 获取次设备号 */
	}
	printk("keydev.major=%d, keydev.minor=%d\r\n", keydev.major, keydev.minor);

	/* 2. 初始化cdev */
	keydev.cdev.owner = THIS_MODULE;
	cdev_init(&keydev.cdev, &keydev_fops);

	/* 3. 添加一个cdev */
	cdev_add(&keydev.cdev, keydev.devid, KEY_CNT);

	/* 4. 创建类 */
	keydev.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(keydev.class))
	{
		return PTR_ERR(keydev.class);
	}

	/* 5. 创建设备 */
	keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, KEY_NAME);
	if (IS_ERR(keydev.device))
	{
		return PTR_ERR(keydev.device);
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
static void __exit _key_exit(void)
{
	/* 注销字符设备 */
	cdev_del(&keydev.cdev);
	unregister_chrdev_region(keydev.devid, KEY_CNT);

	device_destroy(keydev.class, keydev.devid);
	class_destroy(keydev.class);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(_key_init);
module_exit(_key_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
