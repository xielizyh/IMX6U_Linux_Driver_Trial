/**
  ******************************************************************************
  * @file			va_demo.c
  * @brief			va_demo function
  * @author			Xli
  * @email			xieliyzh@163.com
  * @version		1.0.0
  * @date			2020-04-18
  * @copyright		2020, EVECCA Co.,Ltd. All rights reserved
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stdarg.h"

/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           可变参数Demo
 *
 * @param[in]       function: 函数功能
 * @param[in]       num: 可变参数个数
 *
 * @return          none
 *
 * @detail          利用可变参数几个数的均值
 *============================================================================*/
static unsigned int va_demo(char *function, unsigned int num, ...)
{
    va_list valist;
    unsigned int sum = 0;
    unsigned int i = 0;

    printf("function is %s\n", function);

    /* 为 num 个参数初始化 valist */
    va_start(valist, num);
    
    /* 访问所有赋给 valist 的参数 */
    for (i = 0; i < num; i++)
    {
        sum += va_arg(valist, unsigned int);
    }

    /* 清理为 valist 保留的内存 */
    va_end(valist);

    return sum / num;
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
    if (argc != 1)
	{
		printf("Error usage!\r\n");
		return -1;
	}

    printf("Average of 1, 2, 3 is %d\n", va_demo("average", 3, 1, 2, 3));
    printf("Average of 1, 2, 3, 4, 5 is %d\n", va_demo("average", 5, 1, 2, 3, 4, 5));
}