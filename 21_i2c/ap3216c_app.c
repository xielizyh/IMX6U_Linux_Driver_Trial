/**
  ******************************************************************************
  * @file			ap3216c_app.c
  * @brief			key_app function
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
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

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
	unsigned short data[3] = {0};
	int ret = 0;

	if (argc != 2)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开I2C驱动 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	/* 读取按键值 */
	while (1)
	{
		ret = read(fd, data, sizeof(data));
		if (ret == 0)
		{
			printf("ir = %d, als = %d, ps = %d\r\n", data[0], data[1], data[2]);
		}
		usleep(200000);
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