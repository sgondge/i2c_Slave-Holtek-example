
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
int send_command_to_pbci(uint8_t plateID, uint8_t power, uint8_t temperature, uint8_t crc16_ms, 
												 uint8_t crc16_ls,uint8_t padding_1, uint8_t padding_2, uint8_t padding_3);
int receive_data_from_pbci(uint8_t *memory_address, uint8_t *manufacturerID, uint8_t *plateIDReceived, uint8_t *errorCode, 
                           uint8_t *voltage_ms, uint8_t *voltage_ls, uint8_t *current, uint8_t *wattHour_ms,
                           uint8_t *wattHour_ls, uint8_t *igbtTemp, uint8_t *glassSurfaceTemp_ms, uint8_t *glassSurfaceTemp_ls,
													 uint8_t *crc16_ms, uint8_t *crc16_ls, uint8_t padding_1, uint8_t padding_2, uint8_t padding_3); 
unsigned int GetCRC16(volatile unsigned char *puchMsg, unsigned int DataLen);
void updateParametersAndSendCommand(void);
void Clear_I2C_Buffer(void);
void I2C_Reset(void);

/* Private variables ---------------------------------------------------------------------------------------*/
u8 I2C_Slave_Buffer_Tx[BufferSize] = {0};					
u8 I2C_Slave_Buffer_Rx[BufferSize] = {0};
u8 Tx_Index = 0;
u8 Rx_Index = 0;

volatile unsigned int CRC_16_value = 0;
uint8_t plateID, power, temperature;
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
	plateID = 0;
	power = 0;
	temperature = 0;
	crc16_ms= 0;
	crc16_ls= 0;
	padding_1= 0;
	padding_2= 0;
	padding_3 = 0;

	Clear_I2C_Buffer();	
	
  while (1){		
			I2C_Slave_Receiver();
			updateParametersAndSendCommand();
//			receive_data_from_pbci(memory_address,  manufacturerID,  plateIDReceived,  errorCode, 
//                           voltage_ms,  voltage_ls,  current,  wattHour_ms, wattHour_ls,
//                           igbtTemp,  glassSurfaceTemp_ms, glassSurfaceTemp_ls,
//  												  crc16ms, crc16ls, padding_2, padding_2, padding_2);
	}
}

void I2C_Reset(void) {
  // Disable I2C peripheral
  I2C_Cmd(HTCFG_I2C_SLAVE_PORT, DISABLE);
  // Reinitialize I2C peripheral
  I2C_Configuration();
}
void Clear_I2C_Buffer(void) {
	int i;
  for (i = 0; i < BufferSize; i++) {
    I2C_Slave_Buffer_Rx[i] = 0;
  }
	for (i = 0; i < BufferSize; i++) {
    I2C_Slave_Buffer_Tx[i] = 0;
  }
}


void updateParametersAndSendCommand() {	
    u8 i = 0;
		volatile unsigned int CRC_16_value = 0;
		volatile unsigned char Data[3];
		uint8_t power = 6;
    uint8_t plateID = 3;
    uint8_t temperature = 0x00; 	// Example temperature value
		uint8_t padding_1 	= 0x00; 	// 0 padding value
		
		Data[0] = plateID;
		Data[1]	= power;
		Data[2] = temperature;

		CRC_16_value = GetCRC16(Data, sizeof(Data));
		crc16_ms = (CRC_16_value >> 8) & 0xFF; // Most significant byte
		crc16_ls = CRC_16_value & 0xFF;        // Least significant byte
					
//		I2C_Slave_Buffer_Tx[0] = plateID;
//  	I2C_Slave_Buffer_Tx[1] = power;
//		I2C_Slave_Buffer_Tx[2] = temperature;
//		I2C_Slave_Buffer_Tx[3] = crc16_ms;
//		I2C_Slave_Buffer_Tx[4] = crc16_ls;
//		I2C_Slave_Buffer_Tx[5] = padding_1;
//		I2C_Slave_Buffer_Tx[6] = padding_1;
//		I2C_Slave_Buffer_Tx[7] = padding_1;
	
			for (i = 0; i <= BufferSize; i++){
					send_command_to_pbci(plateID, power, temperature, crc16_ms, crc16_ls, padding_1, padding_1, padding_1);
				}
	}

unsigned int GetCRC16(volatile unsigned char *puchMsg, unsigned int DataLen)
{
   unsigned int CRC16_Data = 0xFFFF; // CRC16 variable register
   unsigned char i = 0;
   
   while (DataLen--)  
   {
      CRC16_Data ^= *puchMsg++;  // CRC16 Register "XOR" DATA
      for (i = 0; i < 8; i++)  // Count 8
      {
         if (CRC16_Data & 0x01)    // 0 bit for Hi do >>1 and "XOR"0xA001 else do >>1
             CRC16_Data = (CRC16_Data >> 1) ^ 0xA001;
         else
             CRC16_Data = CRC16_Data >> 1;  
      }
   }  
   return CRC16_Data;  	
}

// I2C send command function implementation
int send_command_to_pbci(uint8_t plateID, uint8_t power, uint8_t temperature,uint8_t crc16_ms, uint8_t crc16_ls,uint8_t padding_1, uint8_t padding_2, uint8_t padding_3) {

		I2C_Slave_Buffer_Tx[0] = plateID;              // Plate ID (1 or 2)
    I2C_Slave_Buffer_Tx[1] = power;                // Power level (0 to 10)
    I2C_Slave_Buffer_Tx[2] = temperature;          // Temperature value (0 to 240)
    I2C_Slave_Buffer_Tx[3] = crc16_ms;                    // CRC High Byte (set to 0 for now)
    I2C_Slave_Buffer_Tx[4] = crc16_ls;                    // CRC Low Byte (set to 0 for now)
    I2C_Slave_Buffer_Tx[5] = padding_1;                    // Padding
    I2C_Slave_Buffer_Tx[6] = padding_2;                    // Padding
    I2C_Slave_Buffer_Tx[7] = padding_3;                    // Padding
	
		I2C_Slave_Transmitter();
    return 0;
}

// I2C receive data function implementation (simplified for clarity)
int receive_data_from_pbci(uint8_t *memory_address, uint8_t *manufacturerID, uint8_t *plateIDReceived, uint8_t *errorCode, 
                           uint8_t *voltage_ms, uint8_t *voltage_ls, uint8_t *current, uint8_t *wattHour_ms,
                           uint8_t *wattHour_ls, uint8_t *igbtTemp, uint8_t *glassSurfaceTemp_ms, uint8_t *glassSurfaceTemp_ls,
													 uint8_t *crc16_ms, uint8_t *crc16_ls, uint8_t padding_1, uint8_t padding_2, uint8_t padding_3){
		
		I2C_Slave_Receiver();	 
    // Parse received data into respective fields (as described earlier)
		*memory_address			= I2C_Slave_Buffer_Rx[0];
    *manufacturerID 		= I2C_Slave_Buffer_Rx[1];
    *plateIDReceived 		= I2C_Slave_Buffer_Rx[2];
    *errorCode 					= I2C_Slave_Buffer_Rx[3];
    *voltage_ms					= I2C_Slave_Buffer_Rx[4]; 
		*voltage_ls 				= I2C_Slave_Buffer_Rx[5];
    *current 						= I2C_Slave_Buffer_Rx[6];
    *wattHour_ms 				= I2C_Slave_Buffer_Rx[7];
		*wattHour_ls				= I2C_Slave_Buffer_Rx[8];
    *igbtTemp 					= I2C_Slave_Buffer_Rx[9];
    *glassSurfaceTemp_ms 	= I2C_Slave_Buffer_Rx[10];
		*glassSurfaceTemp_ls	= I2C_Slave_Buffer_Rx[11];
		*crc16_ms 						= I2C_Slave_Buffer_Rx[12];										 
    *crc16_ls 						= I2C_Slave_Buffer_Rx[13];  // Set CRC to 0 for now, since CRC is not calculated currently
		padding_1 						= I2C_Slave_Buffer_Rx[14];
		padding_2 						= I2C_Slave_Buffer_Rx[15];
		padding_3 						= I2C_Slave_Buffer_Rx[16];	
														 
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
	u8 t;
	u32 reg;
	uint32_t timeout; 
	timeout = TIMEOUT_VALUE;
	while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_ACK_RECEIVER_ADDRESS)){
		reg = HTCFG_I2C_SLAVE_PORT ->SR;
		if(reg & I2C_FLAG_RXNACK){I2C_ClearFlag(HTCFG_I2C_SLAVE_PORT, I2C_FLAG_RXNACK);}			 //handle condition for RXNACK
		else if(reg & I2C_FLAG_RXBF){uint8_t temp = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);}		 //handle condition for RXBF
		else if(reg & I2C_FLAG_RXDNE){
			for (Rx_Index= 0; Rx_Index<=BufferSize;Rx_Index++){
						I2C_Slave_Buffer_Rx[Rx_Index] = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);
					}
		//I2C_Slave_Buffer_Rx[Rx_Index++] = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);
		}   //handle condition for RXDNE
		//else if(reg & I2C_FLAG_ADRS){}																											 // ADRS condition not handled
		else if(--timeout == 0) return;
	}
    /* Check on Slave Receiver RXDNE condition                                                              */
		while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_RX_NOT_EMPTY)){
			if(--timeout == 0) return;}
		  /* Receive data                                                                                           */
		while (Rx_Index < BufferSize){
				/* Store received data on I2Cx                                                                          */
				I2C_Slave_Buffer_Rx[Rx_Index++] = I2C_ReceiveData(HTCFG_I2C_SLAVE_PORT);
				if(Rx_Index >= BufferSize){
					Rx_Index=0;
					}
				}
//		}
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
	uint32_t timeout; 
	timeout = TIMEOUT_VALUE;

		while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_ACK_TRANSMITTER_ADDRESS)){
			reg = HTCFG_I2C_SLAVE_PORT ->SR;
			if(reg & I2C_FLAG_TXDE){	
					I2C_SendData(HTCFG_I2C_SLAVE_PORT, I2C_Slave_Buffer_Tx[Tx_Index++]);} //TXDE condition handle
			else if((reg & I2C_FLAG_TXNRX) == 0){return;}
			else if(--timeout == 0) return;
		}
		while (Tx_Index < BufferSize)
		{
		/* Check on Master Transmitter TXDE condition                                                           */
			while (!I2C_CheckStatus(HTCFG_I2C_SLAVE_PORT, I2C_SLAVE_TX_EMPTY)){
			if(--timeout == 0) return;
			}
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

