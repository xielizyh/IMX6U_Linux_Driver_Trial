/**
  ******************************************************************************
  * @file			keyinput.c
  * @brief			keyinput function
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
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* Private constants ---------------------------------------------------------*/
#define KEYINPUT_CNT			1			/*!< 设备号个数 */
#define KEYINPUT_NAME			"keyinput"	/*!< 设备名 */
#define	KEY0_VALUE				0x01		/*!< 按键值 */
#define INVALID_KEY				0xFF		/*!< 无效值 */
#define KEY_NUM					1			/*!< 按键数量 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* 中断IO描述结构体 */
/**
* @brief 中断IO描述结构体
*/
typedef struct {
	int gpio;		/*!< gpio */
	int irqnum;		/*!< 中断号 */
	unsigned char value;	/*!< 按键对应的键值 */
	char name[10];	/*!< 名字 */
	irqreturn_t (*handler)(int, void *);	/*!< 中断服务函数 */
}irq_keydesc_t;

/* keyinput设备结构体 */
typedef struct {
	dev_t devid;			/*!< 设备号 */
	struct cdev cdev;		/*!< cdev */
	struct class *class;	/*!< 类 */
	struct device *device;	/*!< 设备 */
	struct device_node *nd; /*!< 设备节点 */
	struct timer_list timer;	/*!< 定义一个定时器 */
	irq_keydesc_t irqkeydesc[KEY_NUM];	/*!< 按键描述数组 */
	unsigned char curkey_num;	/*!< 当前按键号 */
	struct input_dev *inputdev;	/*!< input结构体 */
}keyinput_dev_t;

/* Private variables ---------------------------------------------------------*/
static keyinput_dev_t keyinputdev;

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           中断服务函数	
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static irqreturn_t key0_handler(int irq, void *dev_id)
{
	keyinput_dev_t *dev = (keyinput_dev_t*)dev_id;

	dev->curkey_num = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));

	return IRQ_RETVAL(IRQ_HANDLED);
}

/**=============================================================================
 * @brief           定时器服务函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
void timer_function(unsigned long arg)
{
	unsigned char value = 0;
	unsigned char num = 0;
	irq_keydesc_t *keydesc;
	keyinput_dev_t *dev = (keyinput_dev_t*)arg;

	num = dev->curkey_num;
	keydesc = &dev->irqkeydesc[num];
	value = gpio_get_value(keydesc->gpio);

	if (value == 0)	/*!< 按键按下 */
	{
		//input_event(dev->inputdev, EV_KEY, keydesc->value, 1);
		input_report_key(dev->inputdev, keydesc->value, 1);
		input_sync(dev->inputdev);
	}
	else	/*!< 按键松开 */
	{
		//input_event(dev->inputdev, EV_KEY, keydesc->value, 0);
		input_report_key(dev->inputdev, keydesc->value, 0);
		input_sync(dev->inputdev);		
	}
}

/**=============================================================================
 * @brief           初始化按键IO
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static int key_gpio_init(void)
{
	unsigned char i = 0;
	char name[10] = {0};
	int ret = 0;

	/* 设置KEYINPUT所使用的GPIO */
	/* 1. 获取设备节点：keyinput */
	keyinputdev.nd = of_find_node_by_path("/key");
	if (keyinputdev.nd == NULL)
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
		keyinputdev.irqkeydesc[i].gpio = 
					of_get_named_gpio(keyinputdev.nd, "key-gpio", i);
		if (keyinputdev.irqkeydesc[i].gpio < 0)
		{
			printk("can't get key%d\r\n", i);
		}
	}

	/* 3. 设置KEY输入，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++)
	{
		memset(keyinputdev.irqkeydesc[i].name, 0, sizeof(name));
		sprintf(keyinputdev.irqkeydesc[i].name, "KEY%d", i);
		gpio_request(keyinputdev.irqkeydesc[i].gpio, name);	/*!< 请求IO */
		gpio_direction_input(keyinputdev.irqkeydesc[i].gpio);
		keyinputdev.irqkeydesc[i].irqnum = 
					irq_of_parse_and_map(keyinputdev.nd, i);
	}

	/* 4. 申请中断 */
	keyinputdev.irqkeydesc[0].handler = key0_handler;
	keyinputdev.irqkeydesc[0].value = KEY_0;
	for (i = 0; i < KEY_NUM; i++)
	{
		ret = request_irq(keyinputdev.irqkeydesc[i].irqnum,
						keyinputdev.irqkeydesc[i].handler,
						IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
						keyinputdev.irqkeydesc[i].name, &keyinputdev);
		if (ret < 0)
		{
			printk("irq %d request failed!\r\n",
					keyinputdev.irqkeydesc[i].irqnum);
			return -EFAULT;
		}
	}

	/* 创建定时器 */
	init_timer(&keyinputdev.timer);
	keyinputdev.timer.function = timer_function;

	/* 申请input_dev */
	keyinputdev.inputdev = input_allocate_device();
	keyinputdev.inputdev->name = KEYINPUT_NAME;
#if 0 
	__set_bit(EV_KEY, keyinputdev.inputdev->evbit);	/*!< 按键事件 */
	__set_bit(EV_REP, keyinputdev.inputdev->evbit);	/*!< 重复事件 */

	__set_bit(KEY_0, keyinputdev.inputdev->keybit);	/*!< 产生哪些按键 */
#endif

#if 0
	keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	keyinputdev.inputdev->keybit[BIT_WORD(KEY_0)] |= BIT_MASK(KEY_0);
#endif

#if 1
	keyinputdev.inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_set_capability(keyinputdev.inputdev, EV_KEY, KEY_0);
#endif

	/* 注册输入设备 */
	ret = input_register_device(keyinputdev.inputdev);
	if (ret)
	{
		printk("register input device failed!\r\n");
		return ret;
	}

	return 0;
}

/**=============================================================================
 * @brief           驱动入口函数
 *
 * @param[in]       none
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int __init _keyinput_init(void)
{
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
static void __exit _keyinput_exit(void)
{
	unsigned char i = 0;

	/* 删除定时器 */
	del_timer_sync(&keyinputdev.timer);

	/* 释放中断 */
	for (i = 0; i < KEY_NUM; i++)
	{
		free_irq(keyinputdev.irqkeydesc[i].irqnum, &keyinputdev);
	}

	/* 释放input_dev */
	input_unregister_device(keyinputdev.inputdev);
	input_free_device(keyinputdev.inputdev);
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(_keyinput_init);
module_exit(_keyinput_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
