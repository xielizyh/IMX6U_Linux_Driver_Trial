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

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ----------------------------------------------------------*/

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
	int fd;
	char *filename;
	int ret = 0;
	unsigned char data = 0;
	struct pollfd fds;
	fd_set readfds;
	struct timeval timeout;

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

	/* 读取设备 */
#if 0	//poll方式
	fds.fd = fd;
	fds.events = POLLIN;
	while (1)
	{
		ret = poll(&fds, 1, 500);
		if (ret == 0)	/*!< 超时 */	
		{
			
		}
		else if (ret < 0)	/*!< 错误 */
		{

		}
		else
		{
			ret = read(fd, &data, sizeof(data));
			if (ret >= 0)
			{
				printf("key value = %d \r\n", data);
			}
		}
	}
#else
	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		/* 超时时间 */
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;	/*!< 500ms */
		ret = select(fd+1, &readfds, NULL, NULL, &timeout);
		switch (ret)
		{
		case 0: /*!< 超时 */
			break;
		case -1:	/*!< 错误 */
			break;
		default:
			if (FD_ISSET(fd, &readfds))
			{
				ret = read(fd, &data, sizeof(data));
				if (ret >= 0)
				{
					printf("key value = %d \r\n", data);
				}
			}
			break;
		}
	}
#endif

	/* 关闭设备 */
	ret = close(fd);
	if (ret < 0)
	{
		printf("Can't close file %s\r\n", filename);
		return -1;
	}

	return 0;
}