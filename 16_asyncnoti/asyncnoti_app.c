/**
  ******************************************************************************
  * @file			chrdevbaseApp.c
  * @brief			chrdevbaseApp function
  * @author			Xli
  * @email			xieliyzh@163.com
  * @version		1.0.0
  * @date			2020-03-17
  * @copyright		2020, EVECCA Co.,Ltd. All rights reserved
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/select.h"
#include "poll.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"
#include "signal.h"

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static int fd = 0;

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           信号处理函数
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void sigio_signal_func(int signum)
{
	int err = 0;
	unsigned int key_value = 0;

	err = read(fd, &key_value, sizeof(key_value));
	if (err < 0)
	{

	}
	else
	{
		printf("sigio signal! key value=%d\r\n", key_value);	
	}
}

/**=============================================================================
 * @brief           主程序
 *
 * @param[in]       argc:数组元素个数
 * @param[in]		argv:具体参数	
 *
 * @return          none
 *============================================================================*/
int main(int argc, char *argv[])
{
	char *filename;
	int ret = 0;
	int flags = 0;

	if (argc != 2)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开LED驱动 */
	fd = open(filename, O_RDWR | O_NONBLOCK);	/*!< 非阻塞式访问 */
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	/* 设置信号SIGIO的处理函数 */
	signal(SIGIO, sigio_signal_func);

	fcntl(fd, F_SETOWN, getpid());	/*!< 将当前进程号告诉内核 */
	flags = fcntl(fd, F_GETFD);	/*!< 获取当前的进程状态 */
	fcntl(fd, F_SETFL, flags | FASYNC);	/*!< 进程启用异步通知 */

	while (1)
	{
		sleep(2);
	}

	/* 关闭设备 */
	ret = close(fd);
	if (ret < 0)
	{
		printf("Can't close file %s\r\n", filename);
		return -1;
	}

	return 0;
}