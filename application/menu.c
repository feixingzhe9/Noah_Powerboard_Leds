/**
 ******************************************************************************
 * @file    menu.c
 * @author  William Xu
 * @version V2.0.0
 * @date    05-Oct-2014
 * @brief   his file provides the software which contains the main menu routine.
 *          The main menu gives the options of:
 *             - downloading a new binary file,
 *             - uploading internal flash memory,
 *             - executing the binary file already loaded
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "mico.h"
#include "ymodem.h"
#include "platform_config.h"
#include "platform_internal.h"
#include "StringUtils.h"
#include "bootloader.h"
#include <ctype.h>
#include "serial_leds.h"
#include "uart_protocol.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CMD_STRING_SIZE       128
   
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern platform_flash_t platform_flash_peripherals[];

uint8_t tab_1024[1024] =
{
  0
};

char FileName[FILE_NAME_LENGTH];
char ERROR_STR [] = "\n\r*** ERROR: %s\n\r";    /* ERROR message string in code   */

extern char menu[];
#ifdef NO_BLOCK_MENU
extern int getline (char *line, int n);          /* input line               */
#else
extern void getline (char *line, int n);          /* input line               */
#endif
extern void startApplication( uint32_t app_addr );

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(mico_flash_t flash, uint32_t flashdestination, int32_t maxRecvSize);
void SerialUpload(mico_flash_t flash, uint32_t flashdestination, char * fileName, int32_t maxRecvSize);

/* Private functions ---------------------------------------------------------*/
/**
* @brief  Analyse a command parameter
* @param  commandBody: command string address
* @param  para: The para we are looking for
* @param  paraBody: A pointer to the buffer to receive the para body.
* @param  paraBodyLength: The length, in bytes, of the buffer pointed to by the paraBody parameter.
* @retval the actual length of the paraBody received, -1 means failed to find this paras 
*/
#ifdef NO_BLOCK_MENU
static void uart_putchar( int c )
{
  MicoUartSend( STDIO_UART, &c, 1 );
}
#endif
int findCommandPara(char *commandBody, char *para, char *paraBody, int paraBodyLength)
{
  int i = 0;
  int k, j;
  int retval = -1;
  char para_in_ram[100];
  strncpy(para_in_ram, para, 100);
  
  for (i = 0; para_in_ram[i] != 0; i++)  {                /* convert to upper characters */
    para_in_ram[i] = toupper(para_in_ram[i]);
  }
  
  i = 0;
  while(commandBody[i] != 0) {
    if(commandBody[i] == '-' ){
      for(j=i+1, k=0; *(para_in_ram+k)!=0x0; j++, k++ ){
        if(commandBody[j] != *(para_in_ram+k)){
          break;
        } 
      }
      
      if(*(para+k)!=0x0 || (commandBody[j]!=' '&& commandBody[j]!=0x0)){   /* para not found!             */
        i++;
        continue;
      }
      
      retval = 0;
      for (k = j+1; commandBody[k] == ' '; k++);      /* skip blanks                 */
      for(j = 0; commandBody[k] != ' ' && commandBody[k] != 0 && commandBody[k] != '-'; j++, k++){   /* para body found!             */
        if(paraBody) paraBody[j] = commandBody[k];
        retval ++;
        if( retval == paraBodyLength) goto exit;
      }
      goto exit;
    }
    i++;
  }
  
exit:
  if(paraBody) paraBody[retval] = 0x0;
  return retval;
}

/**
* @brief  Download a file via serial port
* @param  None
* @retval None
*/
void SerialDownload(mico_flash_t flash, uint32_t flashdestination, int32_t maxRecvSize)
{
  char Number[10] = "          ";
  int32_t Size = 0;
  
  printf("Waiting for the file to be sent ... (press 'a' to abort)\r\n");
  Size = Ymodem_Receive( &tab_1024[0], flash, flashdestination, maxRecvSize );
  if (Size > 0)
  {
    printf("\n\n\r Successfully!\n\r\r\n Name: %s", FileName);
    
    Int2Str((uint8_t *)Number, Size);
    printf("\n\r Size: %s Bytes\r\n", Number);
  }
  else if (Size == -1)
  {
    printf("\n\n\rImage size is higher than memory!\n\r");
  }
  else if (Size == -2)
  {
    printf("\n\n\rVerification failed!\r\n");
  }
  else if (Size == -3)
  {
    printf("\r\n\nAborted.\r\n");
  }
  else
  {
    printf("\n\rReceive failed!\r\n");
  }
}

/**
* @brief  Upload a file via serial port.
* @param  None
* @retval None
*/
void SerialUpload(mico_flash_t flash, uint32_t flashdestination, char * fileName, int32_t maxRecvSize)
{
  uint8_t status = 0;
  uint8_t key;
  
  printf("Select Receive File\n\r");
  MicoUartRecv( STDIO_UART, &key, 1, MICO_NEVER_TIMEOUT );
  
  if (key == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit(flash, flashdestination, (uint8_t *)fileName, maxRecvSize);
    
    if (status != 0)
    {
      printf("\n\rError while Transmitting\n\r");
    }
    else
    {
      printf("\n\rSuccessfully\n\r");
    }
  }
}
#ifdef NO_BLOCK_MENU
static char *pCmdBuf = NULL;
static int cnt = 0;
int getline (char *line, int n)
{
  char c;
  
  pCmdBuf = line + cnt;
  if( cnt >= (CMD_STRING_SIZE-2) )
  {
    goto exit;
  }
  if ( MicoUartRecv( STDIO_UART, &c, 1, 0 ) != kNoErr )
  {
      return -1;
  }
  if (c == CR)      /* read character                 */
  {
    c = LF;
    goto exit;
  }
  if (c == BACKSPACE  ||  c == DEL)  {    /* process backspace              */
    if (cnt != 0)  {
      cnt--;                              /* decrement count                */
      pCmdBuf--;                             /* and line pointer               */
      uart_putchar (BACKSPACE);                /* echo backspace                 */
      uart_putchar (' ');
      uart_putchar (BACKSPACE);
    }
    return -1;
  }
  else if (c != CNTLQ && c != CNTLS)  {   /* ignore Control S/Q             */
    uart_putchar (*pCmdBuf = c);                  /* echo and store character       */
    pCmdBuf++;                               /* increment line pointer         */
    cnt++;                                /* and count                      */
    return -1;
  }
exit:
  *(pCmdBuf) = 0;                          /* mark end of string             */ 
  return 0;
}
#endif
/**
* @brief  Display the Main Menu on HyperTerminal
* @param  None
* @retval None
*/
void Main_Menu(void)
{
  static char cmdbuf [CMD_STRING_SIZE] = {0}; 
  char cmdname[15] = {0};     /* command input buffer        */
  int i, j;                                       /* index for command buffer    */
#if 0
  char idStr[4], startAddressStr[10], endAddressStr[10], flash_dev_str[4];
  int32_t id, startAddress, endAddress;
  bool inputFlashArea = false;
#endif
  mico_logic_partition_t *partition;
#if 0
  mico_flash_t flash_dev;
  OSStatus err = kNoErr;
#endif
  while (1)  {                                    /* loop forever                */   
#ifndef NO_BLOCK_MENU
    printf ("\n\rPowerBoard> ");
#endif
#if defined __GNUC__
    fflush(stdout);
#endif
#ifdef NO_BLOCK_MENU
    if( -1 == getline (&cmdbuf[0], sizeof (cmdbuf)) )        /* input command line          */
    {
      return;
    }
#else
    getline (&cmdbuf[0], sizeof (cmdbuf));
#endif
    for (i = 0; cmdbuf[i] == ' '; i++);           /* skip blanks on head         */
    for (; cmdbuf[i] != 0; i++)  {                /* convert to upper characters */
      cmdbuf[i] = toupper(cmdbuf[i]);
    }
    
    for (i = 0; cmdbuf[i] == ' '; i++);        /* skip blanks on head         */
    for(j=0; cmdbuf[i] != ' '&&cmdbuf[i] != 0; i++,j++)  {         /* find command name       */
      cmdname[j] = cmdbuf[i];
    }
    cmdname[j] = '\0';
    
    /***************** Command "0" or "BOOTUPDATE": Update the application  *************************/
    if(strcmp(cmdname, "BOOTUPDATE") == 0 || strcmp(cmdname, "0") == 0) {
      #if 0
      partition = MicoFlashGetInfo( MICO_PARTITION_BOOTLOADER );
      if (findCommandPara(cmdbuf, "r", NULL, 0) != -1){
        printf ("\n\rRead Bootloader...\n\r");
        SerialUpload( partition->partition_owner, partition->partition_start_addr, "BootLoaderImage.bin", partition->partition_length );
        continue;
      }
      printf ("\n\rUpdating Bootloader...\n\r");
      err = MicoFlashDisableSecurity( MICO_PARTITION_BOOTLOADER, 0x0, partition->partition_length );
      require_noerr( err, exit);

      SerialDownload( partition->partition_owner, partition->partition_start_addr, partition->partition_length );
      #else
      printf ("\n\rNot support...\n\r");
      break;
      #endif  
    }
    
    /***************** Command "1" or "FWUPDATE": Update the MICO application  *************************/
    else if(strcmp(cmdname, "FWUPDATE") == 0 || strcmp(cmdname, "1") == 0)	{
      #if 0
      partition = MicoFlashGetInfo( MICO_PARTITION_APPLICATION );
      if (findCommandPara(cmdbuf, "r", NULL, 0) != -1){
        printf ("\n\rRead application...\n\r");
        SerialUpload( partition->partition_owner, partition->partition_start_addr, "ApplicationImage.bin", partition->partition_length );
        continue;
      }
      printf ("\n\rUpdating application...\n\r");
      err = MicoFlashDisableSecurity( MICO_PARTITION_APPLICATION, 0x0, partition->partition_length );
      require_noerr( err, exit);
      SerialDownload( partition->partition_owner, partition->partition_start_addr, partition->partition_length ); 			
      #else
      printf ("\n\rNot support...\n\r");
      break;
      #endif				   	
    }
    
    /***************** Command "2" or "PARAUPDATE": Update the application  *************************/
    else if(strcmp(cmdname, "PARUPDATE") == 0 || strcmp(cmdname, "2") == 0)  {
      #if 0
      if (findCommandPara(cmdbuf, "id", idStr, 0) != -1){
        if(Str2Int((uint8_t *)idStr, &id)==0 && id > 0 && id < MICO_PARTITION_MAX ){ //Found Flash start address
          printf ("\n\rIllegal start address.\n\r");
          continue;
        }
        partition = MicoFlashGetInfo( (mico_partition_t)id );
      }else{
        printf ("\n\rMiCO partition not found.\n\r");
        continue;
      }

      if( findCommandPara(cmdbuf, "e", NULL, 0) != -1 ){
        printf( "\n\rErasing %s...\n\r", partition->partition_description );

        err = MicoFlashDisableSecurity( (mico_partition_t)id, 0x0, partition->partition_length );
        require_noerr( err, exit);
        MicoFlashErase( (mico_partition_t)id, 0x0, partition->partition_length );
        continue;
      }
      if (findCommandPara(cmdbuf, "r", NULL, 0) != -1){
        printf ( "\n\rRead %s...\n\r", partition->partition_description );
        SerialUpload( partition->partition_owner, partition->partition_start_addr, "Image.bin", partition->partition_length );
        continue;
      }
      printf ("\n\rUpdating %s...\n\r", partition->partition_description );
      err = MicoFlashDisableSecurity( (mico_partition_t)id, 0x0, partition->partition_length );
      require_noerr( err, exit);
      SerialDownload( partition->partition_owner, partition->partition_start_addr, partition->partition_length );   
      #else
      printf ("\n\rNot support...\n\r");
      break;
      #endif                       
    }
    
    /***************** Command "3" or "FLASHUPDATE": : Update the Flash  *************************/
    else if(strcmp(cmdname, "FLASHUPDATE") == 0 || strcmp(cmdname, "3") == 0) {
      #if 0
      if (findCommandPara(cmdbuf, "dev", flash_dev_str, 1) == -1  ){
        printf ("\n\rUnkown target type! Exiting...\n\r");
        continue;
      }
      
      if(Str2Int((uint8_t *)flash_dev_str, (int32_t *)&flash_dev)==0){ 
        printf ("\n\rDevice Number Err! Exiting...\n\r");
        continue;
      }
      if( flash_dev >= MICO_FLASH_MAX ){
        printf ("\n\rDevice Err! Exiting...\n\r");
        continue;
      }
      
      inputFlashArea = false;
      
      if (findCommandPara(cmdbuf, "start", startAddressStr, 10) != -1){
        if(Str2Int((uint8_t *)startAddressStr, &startAddress)==0){ //Found Flash start address
          printf ("\n\rIllegal start address.\n\r");
          continue;
        }else{
          if (findCommandPara(cmdbuf, "end", endAddressStr, 10) != -1){ //Found Flash end address
            if(Str2Int((uint8_t *)endAddressStr, &endAddress)==0){
              printf ("\n\rIllegal end address.\n\r");
              continue;
            }else{
              inputFlashArea = true;
            }
          }else{
            printf ("\n\rFlash end address not found.\n\r");
            continue;
          }
        }
      }
      
      if(endAddress<startAddress && inputFlashArea == true) {
        printf ("\n\rIllegal address.\n\r");
        continue;
      }
      
      if(inputFlashArea != true){
        startAddress = platform_flash_peripherals[ flash_dev ].flash_start_addr ;
        endAddress = platform_flash_peripherals[ flash_dev ].flash_start_addr 
          + platform_flash_peripherals[ flash_dev ].flash_length - 1;
      }
      
      if (findCommandPara(cmdbuf, "e", NULL, 0) != -1){
        printf ("\n\rErasing dev%d content From 0x%lx to 0x%lx\n\r", flash_dev, startAddress, endAddress);
        platform_flash_init( &platform_flash_peripherals[ flash_dev ] );
        platform_flash_disable_protect( &platform_flash_peripherals[ flash_dev ], startAddress, endAddress );
        platform_flash_erase( &platform_flash_peripherals[ flash_dev ], startAddress, endAddress );
        continue;
      }
      
      if (findCommandPara(cmdbuf, "r", NULL, 0) != -1){
        printf ("\n\rRead dev%d content From 0x%lx to 0x%lx\n\r", flash_dev, startAddress, endAddress);
        SerialUpload(flash_dev, startAddress, "FlashImage.bin", endAddress-startAddress+1);
        continue;
      }
      
      printf ("\n\rUpdating dev%d content From 0x%lx to 0x%lx\n\r", flash_dev, startAddress, endAddress);
      platform_flash_disable_protect( &platform_flash_peripherals[ flash_dev ], startAddress, endAddress );
      SerialDownload(flash_dev, startAddress, endAddress-startAddress+1);  
      #else
      printf ("\n\rNot support...\n\r");
      break;
      #endif                           
    }
    
    
    /***************** Command: MEMORYMAP *************************/
    else if(strcmp(cmdname, "MEMORYMAP") == 0 || strcmp(cmdname, "4") == 0)  {
      printf("\r");
      for( i = 0; i <= MICO_PARTITION_MAX - 1; i++ ){
        partition = MicoFlashGetInfo( (mico_partition_t)i );
        if (partition->partition_owner == MICO_FLASH_NONE)
            continue;
        printf( "|ID:%d| %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", i, partition->partition_description, partition->partition_owner,
               partition->partition_start_addr, partition->partition_length);
      }
    }
    /***************** Command: Excute the application *************************/
    else if(strcmp(cmdname, "COMLOGOFF") == 0 || strcmp(cmdname, "C") == 0)  {
      
      break;
    }
    else if(strcmp(cmdname, "COMLOGON") == 0 || strcmp(cmdname, "D") == 0)  {
      
      break;
    }
    else if(strcmp(cmdname, "c") == 0 || strcmp(cmdname, "M") == 0)  {
      if (findCommandPara(cmdbuf, "0", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds default mode\r\n");
      }
      else if (findCommandPara(cmdbuf, "1", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds welcome mode\r\n");
      }
      else if (findCommandPara(cmdbuf, "2", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds goodbye mode\r\n");
      }
      else if (findCommandPara(cmdbuf, "3", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds idle mode\r\n");
      }
      else if (findCommandPara(cmdbuf, "4", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds dance mode\r\n");
      }
      else if (findCommandPara(cmdbuf, "5", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds warming mode\r\n");
      }

      else
      {
        printf(   "\r\nM -0: default\r\n"
                      "M -1: welcome\r\n"
                      "M -2: goodbye\r\n"
                      "M -3: idle\r\n"
                      "M -4: dance\r\n"
                      "M -5: warming\r\n"
                      "M -6: communication fault\r\n"
                      "M -7: barrier\r\n"
                      "M -8: solicit\r\n"
                      "M -9: freedom\r\n"
                      "M -A: is charging\r\n"
                      "M -B: charge to on\r\n"
                      "M -C: charge finish\r\n");
      }
      break;
    }
    else if(strcmp(cmdname, "EFFECT") == 0 || strcmp(cmdname, "E") == 0)  {
      if (findCommandPara(cmdbuf, "0", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds default effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "1", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds flow effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "2", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds meteor effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "3", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds double meteor effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "4", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds shine effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "5", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds red beat effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "6", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds yellow beat effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "7", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds water green effect\r\n");
      }
      else if (findCommandPara(cmdbuf, "8", NULL, 0) != -1){
        SetSerialLedsEffect( LIGHTS_MODE_NOMAL, NULL, 0 );
        printf("\r\n set leds breath colorful effect\r\n");
      }

      else
      {
        printf(   "\r\nE -0: default\r\n"
                      "E -1: flow\r\n"
                      "E -2: meteor\r\n"
                      "E -3: double meteor\r\n"
                      "E -4: shine\r\n"
                      "E -5: red beat\r\n"
                      "E -6: yellow beat\r\n"
                      "E -7: water green\r\n"
                      "E -8: breath colorful\r\n"
                      "E -9: breath purple\r\n"
                      "E -A: red meteor\r\n"
                      "E -B: yellow meteor\r\n"
                      "E -C: green meteor\r\n"
                      "E -D: red long\r\n"
                      "E -E: green long\r\n"
                      "E -F: blue long\r\n");
      }
      break;
    }
    else if(strcmp(cmdname, "WATCHDOG") == 0 || strcmp(cmdname, "G") == 0)  {
      printf("\r\n watchdog test, goto while(1)\r\n");
      //while(1);
      *(uint32_t *)(0x80000213) = 0xFF;
      break;
    }
    else if(strcmp(cmdname, "RESET") == 0 || strcmp(cmdname, "R") == 0)  {
      MicoSystemReboot();
      break;
    }
    else if(strcmp(cmdname, "HELP") == 0 || strcmp(cmdname, "?") == 0)	{
      printf ( menu, MODEL, SW_VERSION, HW_VERSION );  /* display command menu        */
      break;
    }
    else if(strcmp(cmdname, "") == 0 )	{                     
      break;
    }
    else{
      printf (ERROR_STR, "UNKNOWN COMMAND");
      break;
    }
    
#ifdef NO_BLOCK_MENU
  }
//exit:
    memset( cmdbuf, 0x0, cnt );
    cnt = 0;
    printf("\n\r[%d] PowerBoard> ",HAL_GetTick());
    return;
#else
exit:
    continue;
  }
#endif

}
