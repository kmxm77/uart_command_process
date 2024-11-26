#ifndef __UART_COMMAND_H
#define __UART_COMMAND_H

#include "main.h"
#include "usart.h"

#define UART_COMMAND_PROCESS_OK 0
#define UART_COMMAND_PROCESS_ERROR 1

uint8_t Uart_Command_Process(const char * data);

#endif
