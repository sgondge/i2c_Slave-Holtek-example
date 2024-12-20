
/* Includes ------------------------------------------------------------------------------------------------*/
#include "ht32.h"
#include "ht32_board.h"
#include "ht32_board_config.h"
#include <stdlib.h>
#include <stdio.h>

/* Private types -------------------------------------------------------------------------------------------*/
typedef enum {Fail = 0, Pass = !Fail} TestResult;

/* Private constants ---------------------------------------------------------------------------------------*/
#define I2C_MASTER_ADDRESS     0x0A
#define I2C_SLAVE_ADDRESS      0x50 				 //0X7F -> ATEC PBCI 
#define ClockSpeed             100000
#define TIMEOUT_VALUE 				 100						//changed from 2000 to 1000

#define BufferSize  18

#define REGISTER_ADDRESS       0x00  // Register address for read/write

#define PLATE1_REQ_PIN 				 GPIO_PIN_13   // PB13 used for Plate 1 Request
#define PLATE2_REQ_PIN 				 GPIO_PIN_14   // PB14 used for Plate 2 Request

/* Private function prototypes -----------------------------------------------------------------------------*/
void I2C_Configuration(void);
void plate_req_Configuration(void);
void I2C_Slave_Receiver(void);
void I2C_Slave_Transmitter(void);
int clear_request_line(uint8_t plate);
int set_request_line(uint8_t plate);
int send_command_to_pbci(uint8_t plateID, uint8_t power, uint8_t temperature, uint8_t crc16_ls, 
												 uint8_t crc16_ms,uint8_t padding_1, uint8_t padding_2, uint8_t padding_3);
int receive_data_from_pbci(uint8_t *memory_address, uint8_t *manufacturerID, uint8_t *plateIDReceived, uint8_t *errorCode, 
                           uint8_t *voltage_ls, uint8_t *voltage_ms, uint8_t *current, uint8_t *wattHour_ls,
                           uint8_t *wattHour_ms, uint8_t *igbtTemp, uint8_t *glassSurfaceTemp_ls, uint8_t *glassSurfaceTemp_ms,
													 uint8_t *crc16_ls, uint8_t *crc16_ms, uint8_t *padding_1, uint8_t *padding_2, uint8_t *padding_3); 
//unsigned int GetCRC16(volatile unsigned char *puchMsg, unsigned int DataLen);
void updateParametersAndSendCommand(void);
//void Clear_I2C_Buffer(void);
void I2C_Reset(void);

unsigned short GetCrc_16(unsigned char * pData, int nLength, unsigned short init, const unsigned short *ptable);
unsigned short CRC_GetModbus16(unsigned char *pdata, int len);
const unsigned short g_McRctable_16[256] =
{
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 
  0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 
  0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 
  0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 
  0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 
  0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 
  0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 
  0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 
  0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 
  0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 
  0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 
  0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 
  0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, 
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 
  0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 
  0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 
  0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 
  0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 
  0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 
  0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 
  0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 
  0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 
  0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 
  0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 
  0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

/* Private variables ---------------------------------------------------------------------------------------*/
u8 I2C_Slave_Buffer_Tx[BufferSize] = {0};					
u8 I2C_Slave_Buffer_Rx[BufferSize] = {0};
u8 Tx_Index = 0;
u8 Rx_Index = 0;

//volatile unsigned int CRC_16_value = 0;
uint8_t plateID, powerSend, temperature;
uint8_t crc16_ms, crc16_ls, padding_1, padding_2, padding_3;
uint8_t *memory_address, *manufacturerID, *plateIDReceived, *errorCode, *igbtTemp, *voltage_ms, *voltage_ls, *current, *wattHour_ms, *wattHour_ls, *glassSurfaceTemp_ms, *glassSurfaceTemp_ls, *crc16ms, *crc16ls;

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  * @brief  Main program.
  * @retval None
  ***********************************************************************************************************/
int main(void)
{

  I2C_Configuration();                /* I2C configuration                                                  */
	plate_req_Configuration();
	padding_1= 0;
	padding_2= 0;
	padding_3 = 0;

  while (1){		
			I2C_Slave_Receiver();
			updateParametersAndSendCommand();
	}
}

void I2C_Reset(void) {
  I2C_Cmd(HTCFG_I2C_SLAVE_PORT, DISABLE);
  I2C_Configuration();
}

/* // Unnecessary Clear_I2C_Buffer function
void Clear_I2C_Buffer(void) {
	int i;
  for (i = 0; i < BufferSize; i++) {
    I2C_Slave_Buffer_Rx[i] = 0;
  }
	for (i = 0; i < BufferSize; i++) {
    I2C_Slave_Buffer_Tx[i] = 0;
  }
}
*/


void updateParametersAndSendCommand() {	
		unsigned int CRC_16_value = 0;
		unsigned char Data[3];
		powerSend = 9;
    plateID = 3;
    temperature = 0x00; 	// Example temperature value
		padding_1 	= 0x00; 	// 0 padding value
		
		Data[0] = plateID;
		Data[1]	= powerSend;
		Data[2] = temperature;

		CRC_16_value = CRC_GetModbus16(Data, sizeof(Data));
		crc16_ms = (CRC_16_value >> 8) & 0xFF; // Most significant byte
		crc16_ls = CRC_16_value & 0xFF;        // Least significant byte
					
		send_command_to_pbci(plateID, powerSend, temperature, crc16_ms, crc16_ls, padding_1, padding_1, padding_1);
	}

unsigned short GetCrc_16(unsigned char * pData, int nLength, unsigned short init, const unsigned short *ptable)
{
  unsigned short cRc_16 = init;
  unsigned char temp;
  while(nLength-- > 0)
  {
    temp = cRc_16 & 0xFF; 
    cRc_16 = (cRc_16 >> 8) ^ ptable[(temp ^ *pData++) & 0xFF];
  }
  return cRc_16;
}

unsigned short CRC_GetModbus16(unsigned char *pdata, int len)
{
  return GetCrc_16(pdata, len, 0xFFFF, g_McRctable_16);
}
	
// I2C send command function implementation
int send_command_to_pbci(uint8_t plateID, uint8_t power, uint8_t temperature,uint8_t crc16_ms, uint8_t crc16_ls,uint8_t padding_1, uint8_t padding_2, uint8_t padding_3) {

		I2C_Slave_Buffer_Tx[0] = plateID;              // Plate ID (1 or 2)
    I2C_Slave_Buffer_Tx[1] = power;                // Power level (0 to 10)
    I2C_Slave_Buffer_Tx[2] = temperature;          // Temperature value (0 to 240)
    I2C_Slave_Buffer_Tx[3] = crc16_ls;                    // CRC Low Byte (set to 0 for now)
    I2C_Slave_Buffer_Tx[4] = crc16_ms;                    // CRC High Byte (set to 0 for now)
    I2C_Slave_Buffer_Tx[5] = padding_1;                    // Padding
    I2C_Slave_Buffer_Tx[6] = padding_2;                    // Padding
    I2C_Slave_Buffer_Tx[7] = padding_3;                    // Padding
	
		I2C_Slave_Transmitter();
    return 0;
}

// I2C receive data function implementation (simplified for clarity)
int receive_data_from_pbci(uint8_t *memory_address, uint8_t *manufacturerID, uint8_t *plateIDReceived, uint8_t *errorCode, 
                           uint8_t *voltage_ls, uint8_t *voltage_ms, uint8_t *current, uint8_t *wattHour_ls,
                           uint8_t *wattHour_ms, uint8_t *igbtTemp, uint8_t *glassSurfaceTemp_ls, uint8_t *glassSurfaceTemp_ms,
													 uint8_t *crc16_ls, uint8_t *crc16_ms, uint8_t *padding_1,uint8_t *padding_2,uint8_t *padding_3){
		
		I2C_Slave_Receiver();	 
    // Parse received data into respective fields (as described earlier)
		*memory_address			= I2C_Slave_Buffer_Rx[0];
    *manufacturerID 		= I2C_Slave_Buffer_Rx[1];
    *plateIDReceived 		= I2C_Slave_Buffer_Rx[2];
    *errorCode 					= I2C_Slave_Buffer_Rx[3];
    *voltage_ls					= I2C_Slave_Buffer_Rx[4]; 
		*voltage_ms 				= I2C_Slave_Buffer_Rx[5];
    *current 						= I2C_Slave_Buffer_Rx[6];
    *wattHour_ls 				= I2C_Slave_Buffer_Rx[7];
		*wattHour_ms				= I2C_Slave_Buffer_Rx[8];
    *igbtTemp 					= I2C_Slave_Buffer_Rx[9];
    *glassSurfaceTemp_ls 	= I2C_Slave_Buffer_Rx[10];
		*glassSurfaceTemp_ms	= I2C_Slave_Buffer_Rx[11];
		*crc16_ls 						= I2C_Slave_Buffer_Rx[12];										 
    *crc16_ms 						= I2C_Slave_Buffer_Rx[13];  // Set CRC to 0 for now, since CRC is not calculated currently
		*padding_1 						= I2C_Slave_Buffer_Rx[14];
		*padding_2 						= I2C_Slave_Buffer_Rx[15];
		*padding_3 						= I2C_Slave_Buffer_Rx[16];	
														 
	return 0;
}

/*********************************************************************************************************//**
  * @brief  Configure the I2C.
  * @retval None
  ***********************************************************************************************************/
void I2C_Configuration(void)
{   	
  { /* Enable peripheral clock                                                                              */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    HTCFG_I2C_SLAVE_CLK(CKCUClock)  = 1;
    CKCUClock.Bit.AFIO              = 1;
    CKCUClock.Bit.PB								= 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);
  }

  /* Configure GPIO to I2C mode for Slave                                                                   */
  AFIO_GPxConfig(HTCFG_I2C_SLAVE_SCL_GPIO_ID, HTCFG_I2C_SLAVE_SCL_AFIO_PIN, AFIO_FUN_I2C);
  AFIO_GPxConfig(HTCFG_I2C_SLAVE_SDA_GPIO_ID, HTCFG_I2C_SLAVE_SDA_AFIO_PIN, AFIO_FUN_I2C);
		
  { /* I2C Slave configuration                                                                              */
    /* !!! NOTICE !!!
       Notice that the local variable (structure) did not have an initial value.
       Please confirm that there are no missing members in the parameter settings below in this function.
    */
    I2C_InitTypeDef  I2C_InitStructure;
    I2C_InitStructure.I2C_GeneralCall = DISABLE;
    I2C_InitStructure.I2C_AddressingMode = I2C_ADDRESSING_7BIT;
    I2C_InitStructure.I2C_Acknowledge = ENABLE;
    I2C_InitStructure.I2C_OwnAddress = I2C_SLAVE_ADDRESS;
    I2C_Init(HTCFG_I2C_SLAVE_PORT, &I2C_InitStructure);
  }
  /* Enable I2C                                                                                             */
  I2C_Cmd(HTCFG_I2C_SLAVE_PORT, ENABLE);
}

/*********************************************************************************************************//**
  * @brief  Configure the gpio for plate request.
  * @retval None
  ***********************************************************************************************************/
void plate_req_Configuration(void)
{ /* Configure GPIO as output mode                                                                 					*/
		AFIO_GPxConfig(GPIO_PB, AFIO_PIN_13, AFIO_FUN_GPIO);
		AFIO_GPxConfig(GPIO_PB, AFIO_PIN_14, AFIO_FUN_GPIO);
		/* Default value RESET/SET                                                                              */
		GPIO_WriteOutBits(HT_GPIOB, GPIO_PIN_13, RESET);    
		GPIO_WriteOutBits(HT_GPIOB, GPIO_PIN_14, RESET);
	
		GPIO_SetOutBits       (HT_GPIOB, GPIO_PIN_13);
		GPIO_SetOutBits       (HT_GPIOB, GPIO_PIN_14);
	
		/* Configure GPIO direction as output                                                                   */
		GPIO_DirectionConfig(HT_GPIOB, GPIO_PIN_13, GPIO_DIR_OUT);
		GPIO_DirectionConfig(HT_GPIOB, GPIO_PIN_14, GPIO_DIR_OUT);
}

// Function to set the request line high for the specified plate (1 or 2)
int set_request_line(uint8_t plate) {
    if (plate == 0) {
        GPIO_SetOutBits(HT_GPIOB, PLATE1_REQ_PIN);  // Set PB13 high
    } else if (plate == 1) {
        GPIO_SetOutBits(HT_GPIOB, PLATE2_REQ_PIN);  // Set PB14 high
    } else {
        return -1;  // Invalid plate number
    }
		I2C_DeInit(HTCFG_I2C_SLAVE_PORT);
    return 0;  // Return success
}

// Function to clear the request line (set low) for the specified plate (1 or 2)
int clear_request_line(uint8_t plate) {
    if (plate == 0) {
        GPIO_ClearOutBits(HT_GPIOB, PLATE1_REQ_PIN);  // Set PB13 low
    } else if (plate == 1) {
        GPIO_ClearOutBits(HT_GPIOB, PLATE2_REQ_PIN);  // Set PB14 low
    } else {
        return -1;  // Invalid plate number
    }
    return 0;  // Return success
}

/*********************************************************************************************************//**
  * @brief  I2C Slave receives data to another device.
  * @retval None
  ***********************************************************************************************************/
void I2C_Slave_Receiver(void)
{
  /* Check on Slave Receiver ADRS condition and clear it, added timeout condition                           */
	u32 reg;
	uint8_t timeout; 
	timeout = TIMEOUT_VALUE;
	while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_ACK_RECEIVER_ADDRESS)){
		reg = HTCFG_I2C_SLAVE_PORT ->SR;
		if(reg & I2C_FLAG_RXNACK){I2C_ClearFlag(HTCFG_I2C_SLAVE_PORT, I2C_FLAG_RXNACK);}			 //handle condition for RXNACK
		else if(reg & I2C_FLAG_RXBF){uint8_t temp = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);}		 //handle condition for RXBF
		else if(reg & I2C_FLAG_RXDNE){
			for (Rx_Index= 0; Rx_Index<=BufferSize;Rx_Index++){
						I2C_Slave_Buffer_Rx[Rx_Index] = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);
						if(Rx_Index == BufferSize){
						//	clear_request_line(0);															// clear GPIO to stop further data transfer
					}
			}
		}   //handle condition for RXDNE
		else if(--timeout == 0) return;
	}
  /* Check on Slave Receiver RXDNE condition                                                              */
	while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_RX_NOT_EMPTY)){
			if(--timeout == 0) return;}
  
	/* Receive data                                                                                         */
  while (Rx_Index < BufferSize){
	/* Store received data on I2Cx                                                                          */
	  	I2C_Slave_Buffer_Rx[Rx_Index++] = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);
			if(Rx_Index >= BufferSize){
					Rx_Index=0;
						//clear_request_line(0);																												// clear GPIO to stop further data transfer
				}
	}
			
	if(!I2C_GetFlagStatus(HTCFG_I2C_SLAVE_PORT, I2C_FLAG_RXDNE)){
		clear_request_line(0);
	}
  /* Check on Slave Receiver STO condition                                                                  */
  while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_STOP_DETECTED)){	
// Reset I2C peripheral if bus is busy
//		if (I2C_GetFlagStatus(HTCFG_I2C_SLAVE_PORT, I2C_FLAG_BUSBUSY)) {
//			I2C_Reset();
//			return;
//		}
		if(--timeout == 0) return;
	}
}
/*********************************************************************************************************//**
  * @brief  I2C Slave transfers data from another device.
  * @retval None
  ***********************************************************************************************************/
void I2C_Slave_Transmitter(void)
{
  /* Check on Slave Receiver ADRS condition and clear it                                                    */
  u32 reg;
	uint8_t timeout; 
	timeout = TIMEOUT_VALUE;

		while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_ACK_TRANSMITTER_ADDRESS)){
			reg = HTCFG_I2C_SLAVE_PORT ->SR;
			if(reg & I2C_FLAG_TXDE){I2C_SendData(HTCFG_I2C_SLAVE_PORT, I2C_Slave_Buffer_Tx[Tx_Index++]);} //TXDE condition handle
			else if((reg & I2C_FLAG_TXNRX) == 0){return;}
			else if(--timeout == 0) return;
		}

		/* Check on Master Transmitter TXDE condition                                                           */
			while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_TX_EMPTY)){
			if(--timeout == 0) return;
			}
		while (Tx_Index <= BufferSize)
		{
			/* Send I2C data                                                                                       */
			I2C_SendData(HTCFG_I2C_SLAVE_PORT, I2C_Slave_Buffer_Tx[Tx_Index++]);
			if (Tx_Index >= BufferSize) {
        Tx_Index = 0;
      }
		}
	  /* Check on Slave Receiver STO condition                                                                  */
		while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_STOP_DETECTED)){
		 // Reset I2C peripheral if bus is busy
			if (I2C_GetFlagStatus(HTCFG_I2C_SLAVE_PORT, I2C_FLAG_BUSBUSY)) {
				I2C_Reset();
				return;
			}
			if(--timeout == 0) return;
		}
}

