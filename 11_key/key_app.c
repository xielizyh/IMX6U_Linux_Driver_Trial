/**
  ******************************************************************************
  * @file			key_app.c
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

/* Private constants ---------------------------------------------------------*/
#define	KEY_VALUE		0xF0		/*!< 按键值 */

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
	unsigned char key_value = 0;

	if (argc != 2)
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开KEY驱动 */
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("Can't open file %s\r\n", filename);
		return -1;
	}

	/* 读取按键值 */
	while (1)
	{
		read(fd, &key_value, sizeof(key_value));
		if (key_value == KEY_VALUE)
		{
			printf("KEY0 press, value = %#x\r\n", key_value);
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