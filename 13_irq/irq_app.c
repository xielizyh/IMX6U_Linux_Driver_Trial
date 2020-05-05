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
	int fd;
	char *filename;
	int ret = 0;
	unsigned char data = 0;

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

	/* 读取设备 */
	while (1)
	{
		ret = read(fd, &data, sizeof(data));
		if (ret < 0)
		{
			/* 读取错误 */
		}
		else
		{
			if (data)
			{
				printf("key value = %#x\r\n", data);
			}
		}
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