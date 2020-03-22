/**
  ******************************************************************************
  * @file			chrdevbase.c
  * @brief			chrdevbase function
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

/* Private constants ---------------------------------------------------------*/
#define CHRDEVBASE_MAJOR	200				/*!< 主设备号 */
#define CHRDEVBASE_NAME		"chrdevbase"	/*!< 设备名 */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static char readbuf[100];	/*!< 读缓冲区 */
static char writebuf[100];	/*!< 写缓冲区 */
static char kerneldata[] = {"kernel data!"};

/* Private function ----------------------------------------------------------*/
static int chrdevbase_open(struct inode *inode, struct file *flip);
static ssize_t chrdevbase_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt);
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt);
static int chrdevbase_release(struct inode *inode, struct file *flip);

static struct file_operations chrdevbase_fops = {
	.owner = THIS_MODULE,
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};

/**=============================================================================
 * @brief           打开设备
 *
 * @param[in]       inode:传递给驱动的inode
 * @param[in]		flip:设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int chrdevbase_open(struct inode *inode, struct file *flip)
{
	return 0;
}

/**=============================================================================
 * @brief           从设备读取数据
 *
 * @param[in]       flip:要打卡的设备文件
 * @param[in]		buf:返回给用户空间的数据缓冲区
 * @param[in]		cnt:要读取的数据长度
 * @param[in]		offt:相对于文件首地址的偏移
 *
 * @return          读取的字节数;如果为负值则读取失败
 *============================================================================*/
static ssize_t chrdevbase_read(struct file *flip, char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;

	memcpy(readbuf, kerneldata, sizeof(kerneldata));
	retvalue = copy_to_user(buf, readbuf, cnt);
	if (retvalue == 0 )
	{
		printk("kernel senddata ok!\r\n");
	}
	else
	{
		printk("kernel sendata failed!\r\n");
	}
	
	return 0;
}

/**=============================================================================
 * @brief           向设备写数据
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;

	retvalue = copy_from_user(writebuf, buf, cnt);
	if (retvalue == 0)
	{
		printk("kernel recvdata:%s\r\n", writebuf);
	}
	else
	{
		printk("kernel recvdata failed!\r\n");
	}
	
	return 0;
}

/**=============================================================================
 * @brief           释放设备
 *
 * @param[in]       flip:要关闭的设备文件
 *
 * @return          0:成功;其他:失败
 *============================================================================*/
static int chrdevbase_release(struct inode *inode, struct file *flip)
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
static int __init chrdevbase_init(void)
{
	int retvalue = 0;

	retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
	if (retvalue < 0)
	{
		printk("chrdevbase driver register failed\r\n");
	}
	printk("chrdevbase_init()\r\n");

	return 0;
}

/**=============================================================================
 * @brief           驱动出口函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void __exit chrdevbase_exit(void)
{
	unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
	printk("chrdevbase_exit()\r\n");
}

/**
 *	@brief 指定驱动的入口和出口函数 
 *
 */ 
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

/**
 *	@brief 添加LICENSE和作者信息 
 * 
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("xieli");
