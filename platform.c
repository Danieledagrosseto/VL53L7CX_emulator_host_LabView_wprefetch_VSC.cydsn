/******************************************************************************

  Breakout specific
  
*******************************************************************************/

#include "project.h"
#include "platform.h"
#include <stdio.h>



/* Transfer statuses */
#define TRANSFER_CMPLT      (0x00u)
#define TRANSFER_ERROR      (0xFFu)



/****** Platform specific functions and defines  START *****/
/****** to be filled by user                           *****/
/****** start                                          *****/


uint8_t VL53L7CX_WaitMs(
		VL53L7CX_Platform *p_platform,
		uint32_t TimeMs)
{
	CyDelayUs(TimeMs*1000);
	return 0;
}

uint8_t VL53L7CX_WaitUs(
		VL53L7CX_Platform *p_platform,
		uint32_t TimeUs)
{
	CyDelayUs(TimeUs);
	return 0;
}
static uint8_t vl53l7cx_write_bytes(uint8_t dev, uint8 *bytes, uint32_t cnt)
{
    
    uint8_t status = TRANSFER_ERROR;
    (void) I2C_MasterClearStatus();
    
    // Start I2C write and check status 
     status = I2C_MasterWriteBuf((uint8_t)dev>>(uint8_t)1,
                                            bytes,
									        cnt,
									        I2C_MODE_COMPLETE_XFER);
    if(I2C_MSTR_NO_ERROR == status)
    {
        // If I2C write started without errors, 
        // wait until I2C Master completes write transfer 
        while (0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT))
        {
        }
        
        // Report transfer status 
        if (0u == (I2C_MasterStatus() & I2C_MSTAT_ERR_XFER))
        {
            // Check if all bytes were written 
            if (I2C_MasterGetWriteBufSize() == cnt)
            {
                status = TRANSFER_CMPLT;
            }
        }
    }

    //return (status = VL53L4CD_WaitUs(WAIT_BREAKOUT_READY_uSEC));
    return (status);
}

static uint8_t vl53l7cx_read_bytes(uint8_t dev, uint8_t *data, uint32_t cnt)
{
    

    
     uint8_t status = TRANSFER_ERROR;    
    (void) I2C_MasterClearStatus();
    if(I2C_MSTR_NO_ERROR ==  I2C_MasterReadBuf((uint8_t)dev>>(uint8_t)1,
							data,
							cnt,
							I2C_MODE_COMPLETE_XFER))
    {
        // If I2C read started without errors, 
        // wait until master complete read transfer 
        while (0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT))
        { 
        }
        
        // Display transfer status 
        if (0u == (I2C_MSTAT_ERR_XFER & I2C_MasterStatus()))
        {
            // Check packet structure is correct 
            if ((I2C_MasterGetReadBufSize() == cnt) )
            {
               status = TRANSFER_CMPLT;                
            }
        }
    }
    //return (status = VL53L4CD_WaitUs(WAIT_BREAKOUT_READY_uSEC));
    return (status);
    
}

/****** Platform specific functions and defines  START *****/
/****** to be filled by user                           *****/
/****** end                                          *****/


/****** I2C bridge functions, variables and defines START   *****/
/****** user should leave unchanged                         *****/
/****** start                                               *****/

#define I2CBUF_SIZE 32u

/*  WAIT_BREAKOUT_READY constants */

#define PIPELINE 1

#if !PIPELINE
    #define WAIT_BREAKOUT_READY_uSEC 9
    #define WAIT_BREAKOUT_READY_uSEC_READ 9
    #define WAIT_ADJ 1
    #define FIXED_WAIT 200
#else
    #define WAIT_BREAKOUT_READY_uSEC 7
    #define WAIT_BREAKOUT_READY_uSEC_READ 7
    #define WAIT_ADJ 1
    #define FIXED_WAIT 200
#endif

#define WAIT_EEPROM_READY_MSEC 25

uint8_t VL53L7CX_RdByte(
		VL53L7CX_Platform *p_platform,
		uint16_t RegisterAddress,
		uint8_t *p_value)
{
	uint8_t status = 255;
	
	/* Need to be implemented by customer. This function returns 0 if OK */
    const uint8_t bytestoreadfromregister = 1u;
	uint8_t writeBuffer[3];
	writeBuffer[0] = bytestoreadfromregister;
	writeBuffer[1] = (uint8_t)(RegisterAddress >> 8);
	writeBuffer[2] = (uint8_t)(RegisterAddress & 0xFFu);
    status = vl53l7cx_write_bytes(p_platform->address, writeBuffer, 3);
    
	//VL53L7CX_WaitUs(p_platform,WAIT_BREAKOUT_READY_uSEC*4);
    
    VL53L7CX_WaitUs(p_platform,(WAIT_BREAKOUT_READY_uSEC_READ*WAIT_ADJ)*4+FIXED_WAIT);    
    uint8_t data[1];
	status = vl53l7cx_read_bytes(p_platform->address, data, 1);  
    if(status == TRANSFER_CMPLT)
	{
		*p_value = data[0];
	}
    VL53L7CX_WaitUs(p_platform,100);
	return status;	
}


enum writestatuses{ BRIDGE, CONFIG, CHANGEADDRESS,CHANGEDADDRESS};

uint8_t VL53L7CX_WrByte(
		VL53L7CX_Platform *p_platform,
		uint16_t RegisterAddress,
		uint8_t value)
{
	uint8_t status = 255;
    
    static enum writestatuses writestatus = BRIDGE;  

    switch (writestatus){
        case BRIDGE: if(RegisterAddress == 0x7fff & value == 0x00)
                    writestatus = CONFIG;
        break;
        case CONFIG: 
                    if(RegisterAddress == 0x4)
                        writestatus = CHANGEADDRESS;
                    else
                        writestatus = BRIDGE;
        break;
        case CHANGEADDRESS: if(RegisterAddress == 0x7fff & value == 0x02)
                    writestatus = CHANGEDADDRESS;
        break;
        case CHANGEDADDRESS: writestatus = BRIDGE;
                    
    }
	
    const uint8_t bytestoreadfromregister = 0u;
	uint8_t data[4];
        
	data[0] = bytestoreadfromregister;
	data[1] = (uint8_t)(RegisterAddress >> 8);
	data[2] = (uint8_t)(RegisterAddress & 0xFFu);
	data[3] = value;

	status = vl53l7cx_write_bytes(p_platform->address, data, 4);   
    
    
    //Manage change address
    if( writestatus == CHANGEDADDRESS){
        VL53L7CX_WaitMs(p_platform, WAIT_EEPROM_READY_MSEC);
        writestatus = BRIDGE;
    }
    else
        VL53L7CX_WaitUs(p_platform,(WAIT_BREAKOUT_READY_uSEC*WAIT_ADJ)*5+FIXED_WAIT);
        
	return status;
}

uint8_t VL53L7CX_WrMulti(
		VL53L7CX_Platform *p_platform,
		uint16_t RegisterAddress,
		uint8_t *p_values,
		uint32_t size)
{
	uint8_t status = TRANSFER_CMPLT;
	uint32_t offset = 0u;
	uint8_t data[I2CBUF_SIZE];
	const uint32_t max_payload_per_xfer = (uint32_t)I2CBUF_SIZE - 4u;

	uint32_t chunk_count = 0;
	while(offset < size)
	{
		uint32_t chunk_size = size - offset;
		uint16_t current_register = (uint16_t)(RegisterAddress + offset);

		if(chunk_size > max_payload_per_xfer)
		{
			chunk_size = max_payload_per_xfer;
		}

		data[0] = 0u;
		data[1] = (uint8_t)(current_register >> 8);
		data[2] = (uint8_t)(current_register & 0xFFu);

		for(uint32_t i = 0u; i < chunk_size; i++)
		{
			data[3u + i] = p_values[offset + i];
		}
        chunk_count++;
		status = vl53l7cx_write_bytes(p_platform->address, data, chunk_size + 3u);
		if(status != TRANSFER_CMPLT)
		{
            printf("Wmulti fails after %d chunk_counts with status = %u\r\n",chunk_count ,status);
			return status;
		}

		VL53L7CX_WaitUs(p_platform, (WAIT_BREAKOUT_READY_uSEC *WAIT_ADJ)* (chunk_size+1)+FIXED_WAIT);
        
        //VL53L7CX_WaitUs(p_platform, (WAIT_BREAKOUT_READY_uSEC)*80);//pipelining test
       
		offset += chunk_size;
        
	}

	return status;
}

uint8_t VL53L7CX_RdMulti(
		VL53L7CX_Platform *p_platform,
		uint16_t RegisterAddress,
		uint8_t *p_values,
		uint32_t size)
{
	uint8_t status = TRANSFER_CMPLT;
	uint32_t offset = 0u;
	uint8_t writeBuffer[3];
	const uint32_t max_read_per_xfer = (uint32_t)I2CBUF_SIZE - 1u;
    //const uint32_t max_read_per_xfer = (uint32_t)256 - 1u;//debug
	while(offset < size)
	{
		uint32_t chunk_size = size - offset;
		uint16_t current_register = (uint16_t)(RegisterAddress + offset);

		if(chunk_size > max_read_per_xfer)
		{
			chunk_size = max_read_per_xfer;
		}

		writeBuffer[0] = (uint8_t)chunk_size;
		writeBuffer[1] = (uint8_t)(current_register >> 8);
		writeBuffer[2] = (uint8_t)(current_register & 0xFFu);

		status = vl53l7cx_write_bytes(p_platform->address, writeBuffer, 3u);
		if(status != TRANSFER_CMPLT)
		{
			return status;
		}
		//VL53L7CX_WaitUs(p_platform, (WAIT_BREAKOUT_READY_uSEC)*3);
        
        VL53L7CX_WaitUs(p_platform, (WAIT_BREAKOUT_READY_uSEC_READ * WAIT_ADJ)* (chunk_size+4)+ FIXED_WAIT);
		status = vl53l7cx_read_bytes(p_platform->address, &p_values[offset], chunk_size);
		if(status != TRANSFER_CMPLT)
		{
            printf("Rmulti CHUNK TRANSFER_CMPLT FALSE \r\n" );
			return status;
		}
	
        VL53L7CX_WaitUs(p_platform,100);

		offset += chunk_size;
	}

	return status;
}

uint8_t VL53L7CX_Reset_Sensor(
		VL53L7CX_Platform *p_platform)
{
	uint8_t status = 0;
	
	/* (Optional) Need to be implemented by customer. This function returns 0 if OK */
	/* WARNING: AVDD and VDDIO are internally connected in Breakout board */
    
	/* Set pin LPN to LOW */
	/* Set pin AVDD to LOW */
	/* Set pin VDDIO  to LOW */
    LPn_OUT_Write(0); 
	VL53L7CX_WaitMs(p_platform, 100);

	/* Set pin LPN of to HIGH */
	/* Set pin AVDD of to HIGH */
	/* Set pin VDDIO of  to HIGH */
    LPn_OUT_Write(1); 
	VL53L7CX_WaitMs(p_platform, 100);

	return status;
}

void VL53L7CX_SwapBuffer(
		uint8_t 		*buffer,
		uint16_t 	 	 size)
{
	uint32_t i, tmp;
	
	/* Example of possible implementation using <string.h> */
	for(i = 0; i < size; i = i + 4) 
	{
		tmp = (
		  buffer[i]<<24)
		|(buffer[i+1]<<16)
		|(buffer[i+2]<<8)
		|(buffer[i+3]);
		
		memcpy(&(buffer[i]), &tmp, 4);
	}
}	
/****** I2C bridge functions, variables and defines START   *****/
/****** user should leave unchanged                         *****/
/****** end                                                 *****/

