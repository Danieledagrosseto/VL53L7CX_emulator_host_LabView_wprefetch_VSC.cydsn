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
volatile uint16_t errorStatus = 0u;

/* command rcv code */
#define MAX_JSON_SIZE 255u
#define MAX_CMD_SIZE (MAX_JSON_SIZE + 2u)
#define RX_ERROR_OVERFLOW 0x100u
#define RX_ERROR_INVALID_CHARACTER 0x200u
#define I2C_FIRST_ADDRESS 0x03u
#define I2C_LAST_ADDRESS 0x77u
#define I2C_ADDRESS_COUNT (I2C_LAST_ADDRESS - I2C_FIRST_ADDRESS + 1u)
volatile char rxbuffer[MAX_CMD_SIZE] = {'\0'};
volatile bool receivingmsg = false;
volatile bool Isnewcommand = false;
static uint8_t savedAddresses8bit[I2C_ADDRESS_COUNT];
static uint8_t savedAddressCount = 0u;

static uint16_t uart_take_command(char *buffer, size_t bufferSize)
{
    uint16_t status = errorStatus;

    /* RxIsr leaves the completed buffer untouched until it is consumed. */
    for(size_t i = 0u; i < bufferSize - 1u; i++)
    {
        buffer[i] = rxbuffer[i];
        if(buffer[i] == '\0')
        {
            break;
        }
    }
    buffer[bufferSize - 1u] = '\0';

    uint8 interruptState = CyEnterCriticalSection();
    errorStatus = 0u;
    Isnewcommand = false;
    CyExitCriticalSection(interruptState);
    return status;
}

static size_t uart_read_line_blocking(char *buffer, size_t bufferSize)
{
	if((buffer == NULL) || (bufferSize == 0u))
	{
		return 0u;
	}

    for(;;)
    {
        while(!Isnewcommand)
        {
            CyDelay(1u);
        }
        if(uart_take_command(buffer, bufferSize) == 0u)
        {
            break;
        }
        printf("UART receive error. Please retry.\r\n");
    }

	return strlen(buffer);
}

static bool i2c_probe_7bit_address(uint8_t address, uint8_t *scanStatus)
{
    uint8_t status = I2C_MasterSendStart(address, I2C_WRITE_XFER_MODE);
    uint8_t stopStatus = I2C_MasterSendStop();

    if((status != I2C_MSTR_NO_ERROR) && (status != I2C_MSTR_ERR_LB_NAK))
    {
        *scanStatus = status;
    }
    else if((stopStatus != I2C_MSTR_NO_ERROR) &&
            !((status == I2C_MSTR_ERR_LB_NAK) && (stopStatus == I2C_MSTR_NOT_READY)))
    {
        *scanStatus = stopStatus;
    }

    return (status == I2C_MSTR_NO_ERROR) && (*scanStatus == I2C_MSTR_NO_ERROR);
}

static uint8_t i2c_collect_available_addresses(uint8_t *addresses7bit, uint8_t maxCount,
                                              uint8_t *scanStatus)
{
	uint8_t count = 0u;
    *scanStatus = I2C_MSTR_NO_ERROR;

	for(uint8_t addr = I2C_FIRST_ADDRESS; addr <= I2C_LAST_ADDRESS; addr++)
	{
		if(i2c_probe_7bit_address(addr, scanStatus))
		{
			if((addresses7bit != NULL) && (count < maxCount))
			{
				addresses7bit[count] = addr;
			}
			count++;
		}
        if(*scanStatus != I2C_MSTR_NO_ERROR)
        {
            break;
        }
	}

	return count;
}

static uint8_t i2c_scan_and_save_addresses(void)
{
    uint8_t addresses7bit[I2C_ADDRESS_COUNT];
    uint8_t status;
    uint8_t count = i2c_collect_available_addresses(addresses7bit,
        (uint8_t)sizeof(addresses7bit), &status);

    savedAddressCount = 0u;
    if(status == I2C_MSTR_NO_ERROR)
    {
        for(uint8_t i = 0u; i < count; i++)
        {
            savedAddresses8bit[i] = (uint8_t)(addresses7bit[i] << 1);
        }
        savedAddressCount = count;
    }
    return status;
}

uint8_t prompt_sensor_address_8bit(uint8_t defaultAddress8bit)
{
	char inputLine[32] = {0};
	char *endptr;
	unsigned long parsedValue;
	uint8_t detectedAddresses[16] = {0};
	uint8_t detectedCount;
    uint8_t scanStatus;

	detectedCount = i2c_collect_available_addresses(detectedAddresses, (uint8_t)(sizeof(detectedAddresses) / sizeof(detectedAddresses[0])), &scanStatus);
    if(scanStatus != I2C_MSTR_NO_ERROR)
    {
        printf("I2C scan failed: %u\r\n", scanStatus);
        detectedCount = 0u;
    }

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
    static size_t charcount = 0u;
    
    do
    {
        /* Read receiver status register */
        rxStatus = UART_RXSTATUS_REG;

        if((rxStatus & (UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR |
                        UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN)) != 0u)
        {
            /* ERROR handling. */
            if(!Isnewcommand)
            {
                errorStatus |= rxStatus & (UART_RX_STS_BREAK | UART_RX_STS_PAR_ERROR |
                                          UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN);
            }
        }
        
        if((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
        {
            uint8 data = UART_RXDATA_REG;
            if(Isnewcommand)
            {
                continue;
            }
            if(!receivingmsg)
            {
                receivingmsg = true;
                charcount = 0u;
            }
            if(data == '\n')
            {
                if((charcount > 0u) && (rxbuffer[charcount - 1u] == '\r'))
                {
                    charcount--;
                }
                if(charcount > MAX_JSON_SIZE)
                {
                    errorStatus |= RX_ERROR_OVERFLOW;
                }
                rxbuffer[charcount] = '\0';
                receivingmsg = false;
                Isnewcommand = true;
            }
            else if(data == '\0')
            {
                errorStatus |= RX_ERROR_INVALID_CHARACTER;
            }
            else if(charcount < MAX_CMD_SIZE - 1u)
            {
                rxbuffer[charcount++] = (char)data;
            }
            else
            {
                /* Always drain the FIFO and recover at the next newline. */
                errorStatus |= RX_ERROR_OVERFLOW;
            }
        }
    }while((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u);

}

static bool json_valid_id(const cJSON *id)
{
    return cJSON_IsNumber(id) && isfinite(id->valuedouble) &&
           (id->valuedouble >= 0.0) && (id->valuedouble <= 9007199254740991.0) &&
           (floor(id->valuedouble) == id->valuedouble);
}

static void json_send_response(const cJSON *request, const char *error)
{
    const cJSON *type = cJSON_GetObjectItemCaseSensitive(request, "type");
    const cJSON *command = cJSON_GetObjectItemCaseSensitive(request, "command");
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(request, "id");
    cJSON *response = cJSON_CreateObject();
    char *text = NULL;
    char idText[32];

    if(json_valid_id(id))
    {
        /* cJSON's default number formatting can round large integer IDs. */
        (void)snprintf(idText, sizeof(idText), "%.0f", id->valuedouble);
    }

    if(response == NULL)
    {
        goto allocation_failed;
    }
    if((cJSON_IsString(type) ?
        cJSON_AddStringToObject(response, "type", type->valuestring) :
        cJSON_AddNullToObject(response, "type")) == NULL ||
       (cJSON_IsString(command) ?
        cJSON_AddStringToObject(response, "command", command->valuestring) :
        cJSON_AddNullToObject(response, "command")) == NULL ||
       (json_valid_id(id) ?
         cJSON_AddRawToObject(response, "id", idText) :
        cJSON_AddNullToObject(response, "id")) == NULL ||
       cJSON_AddStringToObject(response, "status", error == NULL ? "ok" : "error") == NULL)
    {
        goto allocation_failed;
    }
    if(error != NULL)
    {
        if(cJSON_AddStringToObject(response, "error", error) == NULL)
        {
            goto allocation_failed;
        }
    }
    text = cJSON_PrintUnformatted(response);
    if(text == NULL)
    {
        goto allocation_failed;
    }
    if(error == NULL)
    {
        /* Stream the array to avoid one heap allocation per detected slave. */
        text[strlen(text) - 1u] = '\0';
        UART_PutString(text);
        UART_PutString(",\"addresses\":[");
        for(uint8_t i = 0u; i < savedAddressCount; i++)
        {
            char address[4];
            (void)snprintf(address, sizeof(address), "%u", (unsigned int)savedAddresses8bit[i]);
            if(i > 0u)
            {
                UART_PutString(",");
            }
            UART_PutString(address);
        }
        UART_PutString("]}\r\n");
    }
    else
    {
        UART_PutString(text);
        UART_PutString("\r\n");
    }
    cJSON_free(text);
    cJSON_Delete(response);
    return;

allocation_failed:
    cJSON_Delete(response);
    UART_PutString("{\"type\":null,\"command\":null,\"id\":null,"
                   "\"status\":\"error\",\"error\":\"out_of_memory\"}\r\n");
}

static void json_process_command(const char *commandLine, uint16_t rxError)
{
    cJSON *request = NULL;
    const char *error = NULL;

    if(rxError != 0u)
    {
        error = (rxError & RX_ERROR_OVERFLOW) != 0u ? "command_too_long" :
            ((rxError & RX_ERROR_INVALID_CHARACTER) != 0u ? "invalid_character" : "uart_receive_error");
    }
    else
    {
        request = cJSON_ParseWithOpts(commandLine, NULL, true);
        if(request == NULL)
        {
            error = "invalid_json";
        }
        else if(!cJSON_IsObject(request))
        {
            error = "invalid_request";
        }
        else
        {
            const cJSON *type = cJSON_GetObjectItemCaseSensitive(request, "type");
            const cJSON *command = cJSON_GetObjectItemCaseSensitive(request, "command");
            const cJSON *id = cJSON_GetObjectItemCaseSensitive(request, "id");
            if(!json_valid_id(id))
            {
                error = "invalid_id";
            }
            else if(!cJSON_IsString(type) || !cJSON_IsString(command))
            {
                error = "invalid_request";
            }
            else if((strcmp(type->valuestring, "configuration") != 0) ||
                    (strcmp(command->valuestring, "scan_i2c") != 0))
            {
                error = "unsupported_command";
            }
            else if(i2c_scan_and_save_addresses() != I2C_MSTR_NO_ERROR)
            {
                error = "i2c_scan_failed";
            }
        }
    }
    json_send_response(request, error);
    cJSON_Delete(request);
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
    (void)i2c_scan_and_save_addresses();
    
    SW_Running_LED_Write(1);
	for(cycles_count = 0;;cycles_count++)//forever loop
    {
        if(Isnewcommand)
        {
            char commandLine[MAX_CMD_SIZE];
            uint16_t rxError = uart_take_command(commandLine, sizeof(commandLine));
            json_process_command(commandLine, rxError);
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




   
  
