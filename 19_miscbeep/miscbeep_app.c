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
	unsigned char databuf = 0;

	if (argc != 3)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开BEEP驱动 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	databuf = atoi(argv[2]);
	retvalue = write(fd, &databuf, sizeof(databuf));
	if (retvalue < 0)
	{
		printf("LED control failed!\r\n");
		close(fd);
		return -1;
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