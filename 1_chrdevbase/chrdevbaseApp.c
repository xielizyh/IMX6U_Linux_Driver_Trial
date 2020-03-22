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

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static char usrdata[] = {"usr data!"};

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
	char readbuf[100], writebuf[100];

	if (argc != 3)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开驱动文件 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	if (atoi(argv[2]) == 1)	/*!< 从驱动文件读取数据 */
	{
		retvalue = read(fd, readbuf, 50);
		if (retvalue < 0)
		{
			printf("read file %s failed!\r\n", filename);
		}
		else
		{
			printf("read data:%s\r\n", readbuf);
		}
	}
	else if (atoi(argv[2]) == 2)	/*!< 向设备驱动写数据 */
	{
		memcpy(writebuf, usrdata, sizeof(usrdata));
		retvalue = write(fd, writebuf, 50);
		if (retvalue < 0)
		{
			printf("write file %s failed!\r\n", filename);
		}
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