/**
  ******************************************************************************
  * @file			ap3216c.h
  * @brief			ap3216c header file
  * @author			Xli
  * @email			xieliyzh@163.com
  * @version		1.0.0
  * @date			2020-05-16
  * @copyright		2020, EVECCA Co.,Ltd. All rights reserved
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AP3216C_H_
#define __AP3216C_H_

/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"{
#endif

/* Exported constants --------------------------------------------------------*/
#define AP3216C_ADDR    	0X1E	/* AP3216C器件地址 */

/* AP3316C寄存器 */
#define AP3216C_SYSTEMCONG	0x00	/* 配置寄存器 		*/
#define AP3216C_INTSTATUS	0X01	/* 中断状态寄存器 	*/
#define AP3216C_INTCLEAR	0X02	/* 中断清除寄存器 	*/
#define AP3216C_IRDATALOW	0x0A	/* IR数据低字节 	*/
#define AP3216C_IRDATAHIGH	0x0B	/* IR数据高字节 	*/
#define AP3216C_ALSDATALOW	0x0C	/* ALS数据低字节 	*/
#define AP3216C_ALSDATAHIGH	0X0D	/* ALS数据高字节	*/
#define AP3216C_PSDATALOW	0X0E	/* PS数据低字节 	*/
#define AP3216C_PSDATAHIGH	0X0F	/* PS数据高字节 	*/

/* Exported macros -----------------------------------------------------------*/
/* Exported typedef ----------------------------------------------------------*/
/* Exported variables ------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif  /* __AP3216C_H_ */