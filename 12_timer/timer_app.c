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
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

/* Private constants ---------------------------------------------------------*/
#define CLOSE_CMD		(_IO(0xEF, 0x1))	/*!< 关闭定时器 */
#define OPEN_CMD		(_IO(0xEF, 0x2))	/*!< 打开定时器 */
#define SETPERIOD_CMD	(_IO(0xEF, 0x3))	/*!< 设置定时器 */

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
	int fd, retvalue;
	char *filename;
	unsigned int cmd = 0;
	unsigned int arg = 0;
	unsigned char str[100] = {0};
	int ret = 0;
	unsigned int i = 0;

	if (argc != 2)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开LED驱动 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	while (1)
	{
		printf("Input CMD:");
		ret = scanf("%d", &cmd);
		if (ret != 1)
		{
			fgets(str, 100, stdin);
			if (!strncmp(str, "quit", strlen("quit")))
			{
				printf("exit!!!\r\n");
				break;
			}
		}

		switch (cmd)
		{
		case 1:
			cmd = CLOSE_CMD;
			break;

		case 2:
			cmd = OPEN_CMD;
			break;

		case 3:
			cmd = SETPERIOD_CMD;
			printf("Input Timer Period:");
			ret = scanf("%d", &arg);
			if (ret != 1)
			{
				fgets(str, 100, stdin);
			}
			break;			

		default:
			break;
		}

		ioctl(fd, cmd, arg);
	}

	/* 关闭设备 */
	retvalue = close(fd);
	if (retvalue < 0)
	{
		printf("Can't close file %s\r\n", filename);
		return -1;
	}

	return 0;
}