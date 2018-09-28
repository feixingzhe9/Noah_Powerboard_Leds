/* 
*  Author: Adam Huang
*  Date:2016/12/13
*/
#include "uart_protocol.h"
#include <stdlib.h>
#include "serial_leds.h"
#include "stringUtils.h"

#define protocol_log(M, ...) custom_log("Protocol", M, ##__VA_ARGS__)
#define protocol_log_trace() custom_log_trace("Protocol")


static  uint8_t    rxBuf[UART_RX_BUFFER_LENGTH];
static  uint8_t    txBuf[UART_TX_BUFFER_LENGTH];

recBuf_t                rxInfoInRam;

serial_t                serialDataInRam = {
    .rx_buf = {
        .pBuffer = rxBuf,
        .offset = rxBuf + 3, //header,length,ctype
        .bufferSize = UART_RX_BUFFER_LENGTH,
    },
    .tx_buf = {
        .pBuffer = txBuf,
        .offset = txBuf + 2, //header,length
        .bufferSize = UART_TX_BUFFER_LENGTH,
    },
};

uart_serial_t           uartSerialInRam;

serial_t * const serial = &serialDataInRam;
//UART_HandleTypeDef      ComUartHandle;

static OSStatus uart_frame_send( serial_t *serial, const uint8_t *pData, uint32_t size );
static OSStatus recSerialLedsFrameProcess( serial_t *serial );
static OSStatus ackSerialLedsFrameProcess( serial_t *serial );

OSStatus Protocol_Init( void )
{
  OSStatus err = kNoErr;
  
  if( serial->isSerialInitialized )
  {    
    goto exit;
  }
  
  serial->rx_info = &rxInfoInRam;
  require_action( serial->rx_info , exit, err = kGeneralErr );
  serial->uart_serial = &uartSerialInRam;
  require_action( serial->uart_serial , exit, err = kGeneralErr );

  serial->rx_info->pData = 0;
  serial->rx_info->rxBuffer = rxBuf;
  serial->rx_info->rxCount = 0;
  
  err = uart_ser_open( serial->uart_serial, 115200 );  
  require_action( err == kNoErr, exit,  protocol_log("open uart err") ); 
  
  require_action( serial->rx_buf.pBuffer , exit, err = kGeneralErr );  
  memset( (uint8_t *)serial->rx_buf.pBuffer, 0x00, serial->rx_buf.bufferSize );
#ifdef COMM_DMA_USE_INT
  startDmaRecive( serial->uart_serial->uartHandle, (uint8_t *)serial->rx_buf.pBuffer );
#else
  startDmaRecive( COMM_UART, (uint8_t *)serial->rx_buf.pBuffer );
#endif
  serial->isStartDmaReceive = 1;  
  serial->isSerialInitialized = 1;
  protocol_log( "protocol initialize success" );
  
exit:
  return err;
}

static OSStatus 
uart_frame_send( serial_t *serial, const uint8_t *pData, uint32_t size )
{
  OSStatus err = kNoErr;
  uint8_t  checkSum;
  ram_buff_t *tx_buff = NULL;
  
  require_action( serial , exit, err = kGeneralErr );
  require_action( serial->uart_serial , exit, err = kGeneralErr );  
  
  tx_buff = &serial->tx_buf;
  require_action( tx_buff->pBuffer , exit, err = kGeneralErr );  
  require_action( size <= tx_buff->bufferSize - 4, exit, err = kParamErr );
  
  *(uint8_t *)tx_buff->pBuffer = FRAME_HEADER;
  *(uint8_t *)(tx_buff->pBuffer + 1) = size + 4;//header,length,footer,checksum
  
  checkSum = 0;
  tx_buff->pData = (uint8_t *)tx_buff->pBuffer;
  if( pData != tx_buff->offset )
  {
    memmove( (void *)tx_buff->offset, (void *)pData, size );
  }
  for( uint8_t i = 0; i < size + 2; i++ )
  {
    checkSum += *tx_buff->pData ++;
  }
  *tx_buff->pData ++ = checkSum;  
  *tx_buff->pData ++ = FRAME_FOOTER;
  
  err = uart_ser_write( serial->uart_serial, \
    (uint8_t *)tx_buff->pBuffer, tx_buff->pData - tx_buff->pBuffer );
  
  memset( (uint8_t *)tx_buff->pBuffer, 0x00, tx_buff->bufferSize );
  
exit:
  return err;
}

static OSStatus recSerialLedsFrameProcess( serial_t *serial )
{
  OSStatus err = kNoErr;
  uint8_t       ligthMode;
  uint16_t      lightEffect;
  recSerialLedsFrame_t *recSerialLedsFrame;
    
  require_action( serial , exit, err = kGeneralErr );
  require_action( serial->uart_serial , exit, err = kGeneralErr );

  recSerialLedsFrame = (recSerialLedsFrame_t *)serial->rx_buf.offset;
  ligthMode = recSerialLedsFrame->lightMode;
  lightEffect = (recSerialLedsFrame->lightEffectH << 8) | recSerialLedsFrame->lightEffectL;
  
  set_serial_leds_effect( (light_mode_t)ligthMode, NULL, 0 );

  err = ackSerialLedsFrameProcess( serial );

exit:
  return err;
}

static OSStatus ackSerialLedsFrameProcess( serial_t *serial )
{
  OSStatus err = kNoErr;
  uint8_t  length = sizeof(ackSerialLedsFrame_t);
  ackSerialLedsFrame_t *ackSerialLedsFrame;

  require_action( serial , exit, err = kGeneralErr );
  require_action( serial->uart_serial , exit, err = kGeneralErr );  

  ackSerialLedsFrame = (ackSerialLedsFrame_t *)serial->tx_buf.offset;
  require_action( ackSerialLedsFrame , exit, err = kGeneralErr );

  ackSerialLedsFrame->ctype = FRAME_TYPE_LEDS_CONTROL;
  ackSerialLedsFrame->curLightMode = serial_leds->modeType;
  ackSerialLedsFrame->curLightEffectH = (serial_leds->effectType & 0xff00) >> 8;
  ackSerialLedsFrame->curLightEffectL = (serial_leds->effectType & 0x00ff);

  err = uart_frame_send( serial, (uint8_t *)ackSerialLedsFrame, length );

exit:
  return err;
}

static OSStatus ackNotSupportFrameProcess( serial_t *serial, uint8_t ctype )
{
  OSStatus err = kNoErr;
  uint8_t  length = sizeof( ackGeneralFrame_t );
  ackGeneralFrame_t *ackGeneralFrame;

  require_action( serial, exit, err = kGeneralErr );
  require_action( serial->uart_serial, exit, err = kGeneralErr );
  

  ackGeneralFrame = (ackGeneralFrame_t *)serial->tx_buf.offset;
  require_action( ackGeneralFrame, exit, err = kGeneralErr );

  ackGeneralFrame->ctype = ctype;
  ackGeneralFrame->ack = HW_NO_SUPPORT;

  err = uart_frame_send( serial, (uint8_t *)ackGeneralFrame, length );

exit:
  return err;  
}

static int is_receive_right_frame( void  );
static int is_receive_right_frame( void  )
{ 
  ram_buff_t *rx_buff = &serial->rx_buf;
  
  if( serial->rx_info->rxCount > 0 && *serial->rx_buf.pBuffer == FRAME_HEADER )
  {
    if( *(serial->rx_buf.pBuffer + 1) != 0)
    {
      if( *(serial->rx_buf.pBuffer + *(serial->rx_buf.pBuffer + 1) - 1 ) == FRAME_FOOTER )
      {
        return 0;
      }
    }
  }
  else if( serial->rx_info->rxCount > 0 && *serial->rx_buf.pBuffer != FRAME_HEADER )
  {
    rx_buff->pData = (uint8_t *)rx_buff->pBuffer;
    while( *++rx_buff->pData != FRAME_HEADER && rx_buff->pData - rx_buff->pBuffer < serial->rx_info->rxCount );
    if( *rx_buff->pData == FRAME_HEADER )
    {
      if( *(rx_buff->pData + 1) != 0 &&  *(rx_buff->pData + *(rx_buff->pData + 1) - 1) == FRAME_FOOTER )
      {
        memmove( (void *)rx_buff->pBuffer, rx_buff->pData, *(rx_buff->pData + 1) );
        return 0;
      }
    }
  }
  return -1;
}

void protocol_period( void )
{
  uint8_t checksum;
  uint8_t detectType;
  
  if( !serial->isSerialInitialized )
  {
    Protocol_Init();
  }
#ifdef COMM_DMA_USE_INT
  serial->rx_info->rxCount = receviedDmaDataLength( serial->uart_serial->uartHandle );
#else
  serial->rx_info->rxCount = receviedDmaDataLength( COMM_UART );
#endif
  
  if( !is_receive_right_frame() )
  {
#ifdef PROTOCOL_DEBUG
    if( OPEN_SHOW_LOG == serial->rx_info->showLogFlag )
    {
      char *debugString;
      debugString = DataToHexStringWithSpaces(\
        (const uint8_t *)serial->rx_info->rxBuffer, receviedDmaDataLength( COMM_UART ) );
      protocol_log( "rxBuf: %s", debugString );
      free( debugString );
    }
#endif
    serial->rx_info->pData = 0;
    if( serial->rx_info->startTime == 0 )
    {
      set_serial_leds_effect( LIGHTS_MODE_NOMAL, NULL, 0 );
      protocol_log( "start communicating" );
    }
    serial->rx_info->startTime = os_get_time();
  }
  else
  {
    if( ( serial->rx_info->startTime != 0 ) && \
      ((os_get_time() - serial->rx_info->startTime) >= \
        COMMUNICATION_TIMEOUT_TIME_MS/SYSTICK_PERIOD) )
    {
      serial->rx_info->startTime = 0;
      protocol_log( "communicate timeout" );
      set_serial_leds_effect( LIGHTS_MODE_COM_ERROR, NULL, 0 );
#if 0
#ifdef COMM_DMA_USE_INT
      stopDmaRecive( serial->uart_serial->uartHandle );
#else
      stopDmaRecive( COMM_UART );
#endif
      serial->isStartDmaReceive = 0;
//      serial->rx_info->startTime = os_get_time();
      Protocol_Init();
#endif
    }
    goto exit;
  }
#ifdef COMM_DMA_USE_INT
  stopDmaRecive( serial->uart_serial->uartHandle );
#else
  stopDmaRecive( COMM_UART );
#endif
  serial->isStartDmaReceive = 0;
  serial->rx_buf.receiveStartTime = 0;
  
  checksum = 0;
  
  serial->rx_buf.pData = (uint8_t *)serial->rx_buf.pBuffer;
  
  checksum += *serial->rx_buf.pData;
  
  serial->rx_info->bufferHeader.detectLength = *++serial->rx_buf.pData; 
  if( *(serial->rx_buf.pData + serial->rx_info->bufferHeader.detectLength - 2) != FRAME_FOOTER )
  {
    protocol_log("not known cmd");
    goto exit;
  }
  checksum += *serial->rx_buf.pData;
  
  serial->rx_info->bufferHeader.detectType = *++serial->rx_buf.pData;
  checksum += *serial->rx_buf.pData;
    
  for( uint8_t i = 0; i < (serial->rx_info->bufferHeader.detectLength - 5); i++ )
  {
    checksum += *++serial->rx_buf.pData;
  }
  require_action_quiet( checksum == *++serial->rx_buf.pData, exit, \
    protocol_log("check sum not match") );

  detectType = serial->rx_info->bufferHeader.detectType;
  
  switch( detectType )
  {
  case FRAME_TYPE_LEDS_CONTROL:
        recSerialLedsFrameProcess( serial );
    break;
  default:
        ackNotSupportFrameProcess( serial, detectType );
    break;
  }

  if( serial->rx_info->rxCount )
  {
    memset( (uint8_t *)serial->rx_buf.pBuffer, 0x00, serial->rx_buf.bufferSize );  
  }
#ifdef COMM_DMA_USE_INT
  startDmaRecive( serial->uart_serial->uartHandle, (uint8_t *)serial->rx_buf.pBuffer );
#else
  startDmaRecive( COMM_UART, (uint8_t *)serial->rx_buf.pBuffer );
#endif 
  serial->isStartDmaReceive = 1;

exit:
  if( serial->rx_info->rxCount == 0xFF )
  {
    memset( (uint8_t *)serial->rx_buf.pBuffer, 0x00, serial->rx_buf.bufferSize );
#ifdef COMM_DMA_USE_INT
    startDmaRecive( serial->uart_serial->uartHandle, (uint8_t *)serial->rx_buf.pBuffer );
    stopDmaRecive( serial->uart_serial->uartHandle );
#else
    startDmaRecive( COMM_UART, (uint8_t *)serial->rx_buf.pBuffer );
    stopDmaRecive( COMM_UART );
#endif
    serial->isStartDmaReceive = 0;     
  }
  if( !serial->isStartDmaReceive )
  {
    serial->rx_info->rxCount = 0;
    memset( (uint8_t *)serial->rx_buf.pBuffer, 0x00, serial->rx_buf.bufferSize );   
#ifdef COMM_DMA_USE_INT
    startDmaRecive( serial->uart_serial->uartHandle, (uint8_t *)serial->rx_buf.pBuffer );
#else
    startDmaRecive( COMM_UART, (uint8_t *)serial->rx_buf.pBuffer );
#endif
    serial->isStartDmaReceive = 1;
  }
  return;
}
