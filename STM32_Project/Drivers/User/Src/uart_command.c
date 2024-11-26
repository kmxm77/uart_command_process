/**
 ****************************************************************************************************
 * @file        uart_command.c
 * @author      kmxm
 * @version     V1.0
 * @date        2024-11-26
 * @brief       串口指令解析函数，支持参数设置指令和参数查询指令
 ****************************************************************************************************
 * @attention
 * 硬件平台：STM32H723ZGT6
 * 代码基于ST官方Demo中的代码修改而来，添加了查询指令
 * 参数设置指令格式：参数名称=参数1，参数2，参数3（假设该参数为一个3元素数组）
 * 参数查询指令格式：参数名称=？，查询结果的格式与参数设置指令相同
 * 代码不限制使用平台，目前已在STM32和ESP32上成功运行
 * 代码需要配合串口不定长接收使用
 *
 ****************************************************************************************************
 */

#include "uart_command.h"
#include <string.h>
#include <stdio.h>

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(a)  ((sizeof(a) / sizeof(a[0])))
#endif

/**
 * @struct Params_t
 * @brief 可设置参数结构体
 * 
 * 该结构体用于存储可通过串口指令设置的参数，参数变量类型支持整型和单精度浮点型
 */
struct Params_t
{
  int RGB[3];///<测试参数1,3元素整型数组
  int State;///<测试参数2，整型变量
	float Temper;///<测试参数3，单精度浮点型变量
	float XY[2];///<测试参数4，2元素单精度浮点型数组
};

/**
 * @struct Uart_command
 * @brief  串口指令结构体
 */
struct Uart_command
{
  const char * name;///<指令名称
  char * scanfmt;///<参数变量类型
  uint8_t para_num;///<参数元素个数
  int paraOffset;///<参数的存储位置在参数结构体中的偏移量
};

struct Params_t Params = {
.RGB = {0,0,0},
.State = 0,
.Temper = 0,
.XY={0,0}
};

/* 串口指令表，新增可设置参数时也需要在串口指令表中添加新的元素 */
struct Uart_command Uart_command_table[]={
{.name = "RGB",.para_num = 3,.scanfmt="%d",.paraOffset = offsetof(struct Params_t,RGB)},
{.name = "State",.para_num = 1,.scanfmt="%d",.paraOffset = offsetof(struct Params_t,State)},
{.name = "Temper",.para_num = 1,.scanfmt="%f",.paraOffset = offsetof(struct Params_t,Temper)},
{.name = "XY",.para_num = 2,.scanfmt="%f",.paraOffset = offsetof(struct Params_t,XY)}
};

/**
  * @brief 串口数据发送函数
  * @param[in]  * tx_buffer 发送数据缓冲地址
  * @param[in]  tx_num 发送数据长度
  * @return  None
  * @note 该函数用于实现多平台的兼容
*/
static void UART_Transmit_Buffer(const char * tx_buffer,uint16_t tx_num)
{
  HAL_UART_Transmit(&huart1,(const uint8_t *)tx_buffer,tx_num,10);
}

/**
  * @brief 串口接收数据解析函数
  * @param[in]  * data 需要解析的数据
  * @return  函数执行结果
  *          UART_COMMAND_PROCESS_OK
  *          UART_COMMAND_PROCESS_ERROR
*/
uint8_t Uart_Command_Process(const char * data)
{
	uint8_t Status = UART_COMMAND_PROCESS_ERROR;
	uint8_t i,j;
	char * str_find;
	char * ParamValue;
	unsigned char tx_temp_buffer[64] = {0};
	uint8_t * ParamsPtr;
	uint8_t * CheckPtr = NULL;
	uint8_t check_request = 0;
	int print_num;	
	
	/* 查询接收的数据中是否包含'='字符，初步判断是否是指令 */
	str_find = strchr((const char *)data,'=');
	if(str_find != NULL)
	{
			ParamValue = str_find + 1;
			/* 遍历指令表 */
			for(i = 0;i < ARRAY_SIZE(Uart_command_table);i++)
			{
					int ParamLen;
					ParamLen=strlen(Uart_command_table[i].name);

					/* 比较指令名称，确认接收到的指令是否为当前指令 */
					if(strncmp((const char *)data,Uart_command_table[i].name, ParamLen) == 0)
					{
							/* 获取接收到的指令设置的参数变量的地址 */
							ParamsPtr = (uint8_t *)&Params;
							ParamsPtr += Uart_command_table[i].paraOffset;
						
							if(*ParamValue != '?')
							{
									/* 不是查询命令，开始遍历接收到的指令中的参数 */
									for(j = 0;j < Uart_command_table[i].para_num;j++)
									{
											/* 指令中多个参数通过字符','隔开 */
											if(j != 0)
											{
												 str_find = strchr(ParamValue, ','); 
												 if(str_find == NULL)
												 {
														break;
												 }
												 ParamValue = str_find +1 ;
											}
											/* 将指令中包含的参数值赋值给需要设置的参数 */
											sscanf(ParamValue,Uart_command_table[i].scanfmt,ParamsPtr);
											/* 通过Uart_command中的成员变量scanfmt确定自增数 */
											if(Uart_command_table[i].scanfmt[1] == 'd')
											{
											  ParamsPtr += 4;
											}
											else if(Uart_command_table[i].scanfmt[1] == 'f')
											{
											  ParamsPtr += 4;
											}
									}
									if(j == Uart_command_table[i].para_num)
									{
										Status = UART_COMMAND_PROCESS_OK;
									}
							}
							else
							{
									/* 是查询指令，记录当前指令在指令表中的位置 */
									check_request = i+1;
									CheckPtr = ParamsPtr;
							}
							break;
					}

			}
	}
	
	if(check_request != 0)
	{	
			/* 将被查询指令的名称和字符'='填充到发送缓冲 */
			memset(tx_temp_buffer, 0, sizeof(tx_temp_buffer));
			strcpy((char *)tx_temp_buffer,Uart_command_table[check_request-1].name);
			ParamsPtr = tx_temp_buffer + strlen(Uart_command_table[check_request-1].name);
			sprintf((char *)ParamsPtr,"=");
			ParamsPtr += 1;
		
			/* 遍历指令查询的参数 */
			for(i = 0;i < Uart_command_table[check_request-1].para_num;i++)
			{
					if(ParamsPtr + 1 - tx_temp_buffer > sizeof(tx_temp_buffer))
					{
							break;
					}
					if(i != 0)
					{
							/* 查询指令中查询的参数不只1个，用字符','分隔每一个参数 */
							sprintf((char *)ParamsPtr,",");
							ParamsPtr += 1;
					}
					if(Uart_command_table[check_request-1].scanfmt[1] == 'd')
					{
						  /* 参数的变量类型为整型 */
						  int store_temp = 0; 
							if(ParamsPtr + 5 - tx_temp_buffer > sizeof(tx_temp_buffer))
							{
									break;
							}
							store_temp = *(int *)CheckPtr;
							print_num = sprintf((char *)ParamsPtr,"%d",store_temp);
							ParamsPtr += print_num;
							CheckPtr += 4;
					}
					else if(Uart_command_table[check_request-1].scanfmt[1] == 'f')
					{
						  /* 参数的变量类型为浮点型 */
						  float store_temp = 0;
							if(ParamsPtr + 5 - tx_temp_buffer > sizeof(tx_temp_buffer))
							{
									break;
							}
              store_temp = *(float *)CheckPtr;
							print_num = sprintf((char *)ParamsPtr,"%f",store_temp);
							ParamsPtr += print_num;
							CheckPtr += 4;
					}
					
					if(i == (Uart_command_table[check_request-1].para_num - 1))
					{
							/* 查询到最后一个参数时添加回车换行 */
							if(ParamsPtr + 3 - tx_temp_buffer > sizeof(tx_temp_buffer))
							{
									break;
							}
							sprintf((char *)ParamsPtr,"\r\n");
							ParamsPtr += 2;     
					}
			}
			UART_Transmit_Buffer((const char *)tx_temp_buffer,ParamsPtr-tx_temp_buffer+1);
			check_request = 0;
			Status = UART_COMMAND_PROCESS_OK;
	}
	
  return Status;
}
