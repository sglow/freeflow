/* uart.h */

#ifndef _DEF_INC_UART
#define _DEF_INC_UART

#include <stdint.h>

// prototypes
int UART_Init( void );
int UART_SendByte( uint8_t dat );
int UART_Send( uint8_t dat[], int ct );
int UART_SendStr( const char *str );
int UART_Recv( void );
int UART_FlushRx( void );
int UART_RxFull( void );
int UART_TxFree( void );
void UART_ISR( void );

#endif
