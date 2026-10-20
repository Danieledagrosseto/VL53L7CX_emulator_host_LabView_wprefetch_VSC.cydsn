#ifndef TEST_DEVICE_H
#define TEST_DEVICE_H

#include "cytypes.h"

#define CY_ISR(name) void name(void)
#define CyGlobalIntEnable ((void)0)
#define UART_RX_STS_BREAK 0x01u
#define UART_RX_STS_PAR_ERROR 0x02u
#define UART_RX_STS_STOP_ERROR 0x04u
#define UART_RX_STS_OVERRUN 0x08u
#define UART_RX_STS_FIFO_NOTEMPTY 0x10u
#define UART_RXSTATUS_REG mock_uart_status()
#define UART_RXDATA_REG mock_uart_data()
#define I2C_WRITE_XFER_MODE 0u
#define I2C_MSTR_NO_ERROR 0u
#define I2C_MSTR_BUS_BUSY 1u
#define I2C_MSTR_NOT_READY 2u
#define I2C_MSTR_ERR_LB_NAK 3u
#define Test_Timer_msec_INIT_PERIOD 65535u

uint8 mock_uart_status(void);
uint8 mock_uart_data(void);
uint8 I2C_MasterSendStart(uint8 address, uint8 mode);
uint8 I2C_MasterSendStop(void);
void UART_PutString(const char *text);
void SW_Running_LED_Write(uint8 value);

static inline uint8 CyEnterCriticalSection(void) { return 0u; }
static inline void CyExitCriticalSection(uint8 state) { (void)state; }
static inline void CyDelay(unsigned int delay) { (void)delay; }
static inline void isr_rx_StartEx(void (*handler)(void)) { (void)handler; }
static inline void VDAC8_Start(void) {}
static inline void UART_Start(void) {}
static inline void I2C_Start(void) {}
static inline void Sys_Timer_clock_Start(void) {}
static inline void Sys_Timer_msec_Start(void) {}
static inline void Test_Timer_msec_Start(void) {}
static inline void Test_Timer_msec_WriteCounter(unsigned int value) { (void)value; }
static inline unsigned int Test_Timer_msec_ReadCounter(void) { return 65535u; }
static inline unsigned int UART_GetTxBufferSize(void) { return 0u; }
static inline void LPn_OUT_Write(uint8 value) { (void)value; }

#endif
