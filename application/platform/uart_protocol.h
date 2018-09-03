/* 
*  Author: Adam Huang
*  Date:2016/12/13
*/
#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include <stdint.h>
#include "serial_uart.h"
#include "RingBufferUtils.h"

#define PROTOCOL_DEBUG

#define FRAME_HEADER                    0x5A
#define FRAME_FOOTER                    0xA5

#define FRAME_TYPE_LEDS_CONTROL         0x01
#define FRAME_TYPE_VERSION              0x02

typedef struct _serial_frame_t {
  uint8_t               header;
  uint8_t               length;
} serial_frame_t;

typedef struct {
  uint8_t           detectHeader;
  uint8_t           detectLength;
  uint8_t           detectType;
} bufferHeader_t;

typedef struct recBuf_t {
  uint8_t               showLogFlag;
#define         CLOSE_SHOW_LOG  0x00
#define         OPEN_SHOW_LOG   0x01
  uint8_t               pData;
  uint8_t               *rxBuffer;
  bufferHeader_t        bufferHeader;
  uint8_t               rxCount;
  uint32_t              startTime;
#define         COMMUNICATION_TIMEOUT_TIME_MS           6000
  
} recBuf_t;

typedef struct _ram_buff_t {
  const uint8_t         *pBuffer;
  const uint8_t         *offset;
  uint8_t               *pData;
  const uint32_t        bufferSize;
  uint32_t              receiveStartTime;
} ram_buff_t;

typedef struct _serial_t {
  volatile uint8_t              isSerialInitialized;
  volatile uint8_t              isStartDmaReceive;
  recBuf_t                      *rx_info;
  uart_serial_t                 *uart_serial;
  ram_buff_t                    rx_buf;
  ram_buff_t                    tx_buf;
} serial_t;

typedef struct _recSerialLedsFrame_t {
  uint8_t               lightMode;
  uint8_t               lightEffectH;
  uint8_t               lightEffectL;
} recSerialLedsFrame_t;

typedef struct _ackSerialLedsFrame_t {
  uint8_t               ctype;
  uint8_t               curLightMode;
  uint8_t               curLightEffectH;
  uint8_t               curLightEffectL;
} ackSerialLedsFrame_t;

typedef struct _ackGeneralFrame_t {
  uint8_t               ctype;
  uint8_t               ack;
#define             ACK_SUCCESS           0x00
#define             ACK_FAIL              0x01
#define             HW_NO_SUPPORT         0xFF
} ackGeneralFrame_t;
/***************** end of upgrade defines *********************/
extern serial_t * const serial;

void protocol_period( void );
OSStatus Protocol_Init( void );
//OSStatus Protocol_Init( serial_t *serial );

#endif
