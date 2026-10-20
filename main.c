/**************************************
I2C-UART BRidge
**************************************/

#include <device.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h> // The boolean header
#include <stdlib.h>
#include "common.h"
#include <math.h>
#include "cJSON.h"
#include <string.h>
#include "platform.h"
#include "vl53l7cx_api.h"
#include "examples.h"


/* Add an explicit reference to the floating point printf library to allow
the usage of floating point conversion specifier */
#if defined (__GNUC__)
    asm (".global _printf_float");
#endif


/* UART COmmunications */
uint8 errorStatus = 0u;

/* command rcv code */
#define MAX_CMD_SIZE 64
char rxbuffer[MAX_CMD_SIZE] = {'\0'};
bool receivingmsg = false;
bool Isnewcommand = false;

static size_t uart_read_line_blocking(char *buffer, size_t bufferSize)
{
	if((buffer == NULL) || (bufferSize == 0u))
	{
		return 0u;
	}

	Isnewcommand = false;
	receivingmsg = false;
	rxbuffer[0] = '\0';

	while(!Isnewcommand)
	{
		CyDelay(1u);
	}

	strncpy(buffer, rxbuffer, bufferSize - 1u);
	buffer[bufferSize - 1u] = '\0';
	Isnewcommand = false;

	return strlen(buffer);
}

static bool i2c_probe_7bit_address(uint8_t address)
{
    uint8_t status = I2C_MasterSendStart(address, I2C_WRITE_XFER_MODE);

    if(status == I2C_MSTR_NO_ERROR)
    {
        (void)I2C_MasterSendStop();
        return true;
    }

    (void)I2C_MasterSendStop();
    return false;
}

static void i2c_scan_and_print_table(void)
{
    uint8_t foundCount = 0u;

    printf("\r\nI2C scan (7-bit addresses 0x03..0x77)\r\n");
    printf("     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");

    for(uint8_t row = 0u; row < 8u; row++)
    {
        uint8_t base = (uint8_t)(row << 4);
        printf("%02X: ", base);

        for(uint8_t col = 0u; col < 16u; col++)
        {
            uint8_t addr = (uint8_t)(base + col);

            if((addr < 0x03u) || (addr > 0x77u))
            {
                printf("-- ");
            }
            else if(i2c_probe_7bit_address(addr))
            {
                printf("%02X ", addr);
                foundCount++;
            }
            else
            {
                printf("-- ");
            }
        }

        printf("\r\n");
    }

    printf("Detected devices: %u\r\n\r\n", foundCount);
    
}

static uint8_t i2c_collect_available_addresses(uint8_t *addresses7bit, uint8_t maxCount)
{
	uint8_t count = 0u;

	for(uint8_t addr = 0x03u; addr <= 0x77u; addr++)
	{
		if(i2c_probe_7bit_address(addr))
		{
			if((addresses7bit != NULL) && (count < maxCount))
			{
				addresses7bit[count] = addr;
			}
			count++;
		}
	}

	return count;
}

uint8_t prompt_sensor_address_8bit(uint8_t defaultAddress8bit)
{
	char inputLine[32] = {0};
	char *endptr;
	unsigned long parsedValue;
	uint8_t detectedAddresses[16] = {0};
	uint8_t detectedCount;

	detectedCount = i2c_collect_available_addresses(detectedAddresses, (uint8_t)(sizeof(detectedAddresses) / sizeof(detectedAddresses[0])));

	if(detectedCount > 0u)
	{
		printf("Detected sensors (7-bit address): ");
		for(uint8_t i = 0u; i < detectedCount; i++)
		{
			if(i < (uint8_t)(sizeof(detectedAddresses) / sizeof(detectedAddresses[0])))
			{
				printf("0x%02X(8b:0x%02X) ", detectedAddresses[i], (uint8_t)(detectedAddresses[i] << 1));
			}
		}
		if(detectedCount > (uint8_t)(sizeof(detectedAddresses) / sizeof(detectedAddresses[0])))
		{
			printf("...");
		}
		printf("\r\n");
	}
	else
	{
		printf("No I2C device detected during quick scan.\r\n");
	}

	while(true)
	{
		printf("Enter sensor address (8-bit like 0x52 or 7-bit like 0x29, ENTER=0x%02X), then ENTER\r\n", defaultAddress8bit);
        while( UART_GetTxBufferSize() != 0); //wait printf complete
		(void)uart_read_line_blocking(inputLine, sizeof(inputLine));
		printf("Address input: '%s'; bytes:", inputLine);
		for(size_t i = 0u; i < strlen(inputLine); i++)
		{
			printf(" %02X", (unsigned int)(unsigned char)inputLine[i]);
		}
		printf("\r\n");
		if(inputLine[0] == '\0')
		{
			return defaultAddress8bit;
		}

		parsedValue = strtoul(inputLine, &endptr, 0);
		if((*endptr != '\0') || (parsedValue > 0xFFu))
		{
			printf("Invalid value. Please retry.\r\n");
			continue;
		}

		if(parsedValue <= 0x77u)
		{
			return (uint8_t)(parsedValue << 1);
		}

		if((parsedValue >= 0x08u) && (parsedValue <= 0xFEu) && ((parsedValue & 0x01u) == 0u))
		{
			return (uint8_t)parsedValue;
		}

		printf("Address out of range or not write-address aligned. Please retry.\r\n");
	}
}

/*******************************************************************************
* Function Name: RxIsr
********************************************************************************
*
* Summary:
*  Interrupt Service Routine for RX portion of the UART
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
CY_ISR(RxIsr)
{
    uint8 rxStatus;         
    static char *rxData;
    static int charcount = 0;
    
    do
    {
        /* Read receiver status register */
        rxStatus = UART_RXSTATUS_REG;

        if((rxStatus & (UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR |
                        UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN)) != 0u)
        {
            /* ERROR handling. */
            errorStatus |= rxStatus & ( UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR | 
                                        UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN);
        }
        
        if((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
        {
            /* Read data from the RX data register */
            if(!receivingmsg){
               rxData = rxbuffer;
               receivingmsg = true;
               charcount = 0;
            }
            if(charcount < MAX_CMD_SIZE && receivingmsg){
                    *rxData = UART_RXDATA_REG;
                    charcount++;
                }
            else
                errorStatus = 1u;
            
            if(errorStatus == 0u)
            {
                /*Check for \n end of transmission*/                
                                
				if(*rxData == '\n'){
					Isnewcommand = true;
					receivingmsg = false;
					if((rxData > rxbuffer) && (*(rxData - 1) == '\r'))
						*(rxData - 1)='\0';
					else
						*rxData='\0';
				}
                else
                    rxData++;               
                
            }
        }
    }while((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u);

}

// Funzione per estrarre valori dal JSON
float get_value_from_json(const char* json_string, const char* key); 

// Funzione per pulire stringa JSON da UART
void clean_uart_string(char* str); 

/* End UART communications */

uint8_t vl53l7cx_init_timed(VL53L7CX_Configuration *p_dev)
{
	#if(VL53L7CX_INIT_TIMING_ENABLED == ENABLED)
	uint8_t status;
	uint32_t elapsedMs;

	Test_Timer_msec_WriteCounter(Test_Timer_msec_INIT_PERIOD );
	status = vl53l7cx_init(p_dev);	
    elapsedMs = (uint32_t)(Test_Timer_msec_INIT_PERIOD -
		Test_Timer_msec_ReadCounter());
	printf("vl53l7cx_init duration: %u us\r\n", (uint32_t)elapsedMs*10);
	return status;
	#else
	return vl53l7cx_init(p_dev);
	#endif
}


/**************************/
//initialize the sensor and check if it is alive
// Unless the sensor is alive, the function will return an error code
int8_t vl53l7cx_init_sensor(VL53L7CX_Configuration *p_dev)
{
	uint8_t status = 0u;
	uint8_t isAlive = 0u;

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(p_dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	/* (Mandatory) Init VL53L7CX sensor */
	status = vl53l7cx_init_timed(p_dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);

	return status;
}
// function to find connected sensors and ask user to select one of them then initialize it	(look at example0 for more details)	
int8_t vl53l7cx_init_sensor_with_prompt(void)
{
	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	uint8_t 				status, loop, isAlive, isReady, i;
	VL53L7CX_Configuration 	Dev;			/* Sensor configuration */
	VL53L7CX_ResultsData 	Results;		/* Results data from VL53L7CX */


	/*********************************/
	/*      Customer platform        */
	/*********************************/
    printf("Choose and reset the sensor..\r\n");
    // enable slave
    LPn_OUT_Write(1);      
    CyDelay(100);//wait setup end
    
	printf("Choose sensor address before connecting.\r\n");  
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);
	
	/* (Optional) Check if there is a VL53L7CX sensor connected */
   
	
	
    
    
	status = vl53l7cx_is_alive(&Dev, &isAlive);   
    if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
        printf("status = %d\r\n",status);
		return status;
	}
    printf("VL53L7CX detected at 8 bit address 0x%02X\r\n",Dev.platform.address );
    
	/* (Mandatory) Init VL53L7CX sensor */
	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed. Status: %u\r\n",status);
		return status;
	}
	
	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);
	
    //vl53l7cx_stop_ranging(&Dev);
	// disable slave
    //LPn_OUT_Write(0);
	return status;
}

/*************************/
//Change address example
uint32_t cycles_count = 0;
int main(void)
{
    #if(INTERRUPT_CODE_ENABLED == ENABLED)
        isr_rx_StartEx(RxIsr);
    #endif /* INTERRUPT_CODE_ENABLED == ENABLED */
    
    /* Enable global interrupts. */
    CyGlobalIntEnable;  	
    
	
    uint32_t command_count = 0;
    VDAC8_Start();
    UART_Start() ;
    I2C_Start();
    Sys_Timer_clock_Start();
    Sys_Timer_msec_Start();
	#if(VL53L7CX_INIT_TIMING_ENABLED == ENABLED)
    Test_Timer_msec_Start();
	#endif
    
   
    //disable then enable sensor's I2C 
    CyDelay(2000);//wait setup end      
    i2c_scan_and_print_table();
    
    /*
    //enable sensor's I2C
    LPn_OUT_Write(1); 
    CyDelay(1000);//wait setup end   
    i2c_scan_and_print_table();
    */
    
    SW_Running_LED_Write(1);
	printf("0. initialize sensor\r\n");
	printf("1. example1:ranging basic example\r\n");
	printf("2. example2:get/set params example\r\n");
	printf("3. example3:ranging mode example\r\n");
	printf("4. example4:power mode example\r\n");
	printf("5. example5:multiple targets per zone\r\n");
	printf("6. example6:I2C and RAM optimization\r\n");
	printf("7. example7:calibrate xtalk\r\n");
	printf("8. example8:visualize xtalk data\r\n");
	printf("9. example9:detection thresholds\r\n");
	printf("10. example10:motion indicator\r\n");
	printf("11. example11:motion + thresholds\r\n");
	printf("12. example0:change address\r\n");
	printf("13. example12:send ranging data to LabVIEW\r\n");
	printf("Enter item number then ENTER\r\n");
	for(cycles_count = 0;;cycles_count++)//forever loop
    {
         
        if(Isnewcommand){
            printf("Loop Start\r\n");
            //CLean Json msg
            //clean_uart_string(rxbuffer);
            //printf("\r\nCleaned UART data: '%s'\r\n", rxbuffer);

			switch(atoi(rxbuffer))
			{
				case 0:
					vl53l7cx_init_sensor_with_prompt();
					break;
				case 1:
					example1();
					break;
				case 2:
					example2();
					break;
				case 3:
					example3();
					break;
				case 4:
					example4();
					break;
				case 5:
					example5();
					break;
				case 6:
					example6();
					break;
				case 7:
					example7();
					break;
				case 8:
					example8();
					break;
				case 9:
					example9();
					break;
				case 10:
					example10();
					break;
				case 11:
					example11();
					break;
				case 12:
					example0();
					break;
				case 13:
					example12();
					break;
				default:
					printf("Invalid item. Please enter 0..13.\r\n");
					break;
			}
           
            printf("Cycle nbr= %u Command nbr= %u \r\n", cycles_count,++command_count);	
            
            Isnewcommand = false;
			printf("0. initialize sensor\r\n");
			printf("1. example1:ranging basic example\r\n");
			printf("2. example2:get/set params example\r\n");
			printf("3. example3:ranging mode example\r\n");
			printf("4. example4:power mode example\r\n");
			printf("5. example5:multiple targets per zone\r\n");
			printf("6. example6:I2C and RAM optimization\r\n");
			printf("7. example7:calibrate xtalk\r\n");
			printf("8. example8:visualize xtalk data\r\n");
			printf("9. example9:detection thresholds\r\n");
			printf("10. example10:motion indicator\r\n");
			printf("11. example11:motion + thresholds\r\n");
			printf("12. example0:change address\r\n");
			printf("13. example12:send ranging data to LabVIEW\r\n");
			printf("Enter item number then ENTER\r\n");
        }         
    }
}


float get_value_from_json(const char* json_string, const char* key) {
    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
			printf("Error parsing JSON: %s\r\n", error_ptr);
        }
        return NAN; // Usa NAN della math.h invece di un valore custom
    }

    cJSON* value_item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (!cJSON_IsNumber(value_item)) {
		printf("Error: '%s' is not a number or doesn't exist\r\n", key);
        cJSON_Delete(json);
        return NAN;
    }

    float value = (float)value_item->valuedouble;
    cJSON_Delete(json);
    return value;
}

// Funzione per pulire stringa UART
void clean_uart_string(char* str) {
    if (str == NULL) return;
    
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 32 && str[i] != 127) { // Mantieni caratteri stampabili
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}
/* [] END OF FILE */




   
  
