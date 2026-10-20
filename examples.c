#include <device.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "platform.h"
#include "vl53l7cx_api.h"
#include "vl53l7cx_plugin_xtalk.h"
#include "vl53l7cx_plugin_motion_indicator.h"
#include "vl53l7cx_plugin_detection_thresholds.h"
#include "examples.h"

/* Example 0: Change the I2C address of a connected sensor. */
int example0(void)
{

	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	VL53L7CX_Configuration 					dev;
	uint8_t 				status;
	uint8_t                 newAddress;
	
    
    printf("Example0 Start: change I2C address\r\n");
    printf("Sensor Reset\r\n");
    /* Reset sensor toggling PINs (see platform, not in API) */
	VL53L7CX_Reset_Sensor(&(dev.platform));
	printf("Choose current sensor address before connecting.\r\n");
	/*********************************/
	/*      Customer platform        */
	/*********************************/

	/* Select current sensor address to connect */
    dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using current sensor 8-bit I2C address 0x%02X\r\n", dev.platform.address);

	printf("Choose new address to assign to this sensor.\r\n");
	newAddress = prompt_sensor_address_8bit(0x51u);
	if(newAddress == dev.platform.address)
	{
		printf("New address is identical to current address. No change performed.\r\n");
		return 0;
	}

	/*(Optional) Change I2C address */
	status = vl53l7cx_set_i2c_address(&dev, newAddress);
    
    if(status)
	{
		printf("VL53L7cx change address failed\r\n");
		return status;
	}
	dev.platform.address = newAddress;
	printf("VL53L7cx changed address to 0x%02X\r\n", dev.platform.address);
    return 0;
}

/***********************************/
/*   example1: VL53L7CX ULD basic example    */
/***********************************/
/*
* This example is the most basic. It initializes the VL53L7CX ULD, and starts
* a ranging to capture 10 frames.
*
* By default, ULD is configured to have the following settings :
* - Resolution 4x4
* - Ranging period 1Hz
*
* In this example, we also suppose that the number of target per zone is
* set to 1 , and all output are enabled (see file platform.h).
*/


/* Example 1: Basic VL53L7CX ULD ranging example. */
/* Initializes the sensor, starts ranging, and prints one measurement frame. */
int example1(void)
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
    printf("Example1 Start: Standard acquisition..\r\n");
    printf("Sensor Reset\r\n");
    /* Reset sensor toggling PINs (see platform, not in API) */
	VL53L7CX_Reset_Sensor(&(Dev.platform));
    
	printf("Choose sensor address before connecting.\r\n");  
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	

	/* (Optional) Set a new I2C address if the wanted address is different
	* from the default one (filled with 0x20 for this example).
	*/
	//status = vl53l7cx_set_i2c_address(&Dev, 0x20);


	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/
    
   

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
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);
	

	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l7cx_start_ranging(&Dev);
    if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}
	loop = 0;
	while(loop < 1)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
 
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
        
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);

			/* As the sensor is set in 4x4 mode by default, we have a total 
			 * of 16 zones to print. For this example, only the data of first zone are 
			 * print */
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i]);
			}
			printf("\r\n");
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
    
    printf("End of ULD demo\r\n");       
    
	return status;
}


/***********************************/
/*   VL53L7CX ULD get/set params example  */
/***********************************/
/*
* This example shows the possibility of VL53L7CX to get/set params. It
* initializes the VL53L7CX ULD, set a configuration, and starts
* a ranging to capture 10 frames.
*
* In this example, we also suppose that the number of target per zone is
* set to 1 , and all output are enabled (see file platform.h).
*/


/* Example 2: Get and set VL53L7CX ranging parameters. */
/* Configures resolution, frequency, target order, and reports acquisition data. */
int example2(void)
{

	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	uint8_t 				status, loop, isAlive, isReady, i;
	uint32_t 				integration_time_ms;
	VL53L7CX_Configuration 	Dev;			/* Sensor configuration */
	VL53L7CX_ResultsData 	Results;		/* Results data from VL53L7CX */
	uint8_t                 streamcounts[10];
	uint32_t                acquisition_durations_us[10];
	uint8_t                 target_statuses[10][VL53L7CX_RESOLUTION_8X8];
	int16_t                 distances_mm[10][VL53L7CX_RESOLUTION_8X8];

	
	/*********************************/
	/*      Customer platform        */
	/*********************************/

    printf("Example2 Start: resetting sensor..\r\n");
    
    VL53L7CX_Reset_Sensor(&(Dev.platform));
    
	/* Fill the platform structure with customer's implementation. For this
	* example, only the I2C address is used.
	*/
	printf("Choose sensor address before connecting.\r\n");  
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
   	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	/* (Optional) Reset sensor toggling PINs (see platform, not in API) */
	//VL53L7CX_Reset_Sensor(&(Dev.platform));

	/* (Optional) Set a new I2C address if the wanted address is different
	* from the default one (filled with 0x20 for this example).
	*/
	//status = vl53l7cx_set_i2c_address(&Dev, 0x20);
	

	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L7CX sensor connected */
    
	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected status: %u\r\n", status);
        
		return status;
	}
    printf("VL53L7CX detected at 8 bit address 0x%02X\r\n",Dev.platform.address );
    
	/* (Mandatory) Init VL53L7CX sensor */
    Test_Timer_msec_WriteCounter(Test_Timer_msec_INIT_PERIOD );    
	status = vl53l7cx_init(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}
	printf("vl53l7cx_init time: %u us\r\n", (uint32_t)(Test_Timer_msec_INIT_PERIOD - Test_Timer_msec_ReadCounter())*10);
	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);
	
    
	/*********************************/
	/*        Set some params        */
	/*********************************/

	/* Set resolution in 8x8. WARNING : As others settings depend to this
	 * one, it must be the first to use.
	 */
    //CyDelay(20000);
	status = vl53l7cx_set_resolution(&Dev, VL53L7CX_RESOLUTION_8X8);
	if(status)
	{
		printf("vl53l7cx_set_resolution failed, status %u\r\n", status);
		return status;
	}

	/* Set ranging frequency .
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l7cx_set_ranging_frequency_hz(&Dev, 10);
	if(status)
	{
		printf("vl53l7cx_set_ranging_frequency_hz failed, status %u\r\n", status);
		return status;
	}

	/* Set target order to closest */
	status = vl53l7cx_set_target_order(&Dev, VL53L7CX_TARGET_ORDER_CLOSEST);
	if(status)
	{
		printf("vl53l7cx_set_target_order failed, status %u\r\n", status);
		return status;
	}

    
	/* Get current integration time */
	status = vl53l7cx_get_integration_time_ms(&Dev, &integration_time_ms);
	if(status)
	{
		printf("vl53l7cx_get_integration_time_ms failed, status %u\r\n", status);
		return status;
	}
	printf("Current integration time is : %d ms\r\n", (int)integration_time_ms);
    integration_time_ms = 20;
    status = vl53l7cx_set_integration_time_ms(&Dev,integration_time_ms);
    if(status)
	{
		printf("vl53l7cx_set_integration_time_ms failed, status %u\r\n", status);
		return status;
	}
    printf("New integration time is : %d ms\r\n", (int)integration_time_ms);
	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l7cx_start_ranging(&Dev);
    printf("Example2 ranging...\r\n");
	loop = 0;
	while(loop < 10)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
        Test_Timer_msec_WriteCounter(Test_Timer_msec_INIT_PERIOD );    
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
        while(!isReady && !status){
            status = vl53l7cx_check_data_ready(&Dev, &isReady);
        }
        if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);

			streamcounts[loop] = Dev.streamcount;
            acquisition_durations_us[loop] =
                (uint32_t)(Test_Timer_msec_INIT_PERIOD - Test_Timer_msec_ReadCounter()) * 10u;
			for(i = 0; i < 64; i++)
			{
				target_statuses[loop][i] =
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i];
				distances_mm[loop][i] =
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i];
			}
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		//VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);

	for(loop = 0; loop < 10; loop++)
	{
		printf("Print data no : %u\r\n", streamcounts[loop]);
        printf("acq duration: %lu us\r\n", (unsigned long)acquisition_durations_us[loop]);
		for(i = 0; i < 64; i++)
		{
			printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
				i,
				target_statuses[loop][i],
				distances_mm[loop][i]);
		}
		printf("\r\n");
	}
    
    //putoff sensor I2C
    //LPn_OUT_Write(0);  
    
	printf("End of ULD demo\r\n");
	return status;
}
/***********************************/
/*    VL53L7CX ULD ranging mode    */
/***********************************/
/*
* This example shows the differences between ranging modes of VL53L7CX
* (mode continuous and autonomous). For both modes, it initializes the VL53L7CX
* ULD, set the mode, and starts a ranging to capture 10 frames.
*
* In this example, we also suppose that the number of target per zone is
* set to 1 , and all output are enabled (see file platform.h).
*/

/* Example 3: Compare autonomous and continuous ranging modes. */
int example3(void)
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

	/* Fill the platform structure with customer's implementation. For this
	* example, only the I2C address is used.
	*/
    
	printf("Example3 Start: mode continuous and autonomous\r\n");
    printf("Sensor Reset\r\n");
    /* Reset sensor toggling PINs (see platform, not in API) */
	VL53L7CX_Reset_Sensor(&(Dev.platform));
    
	/* Fill the platform structure with customer's implementation. For this
	* example, only the I2C address is used.
	*/
	printf("Choose sensor address before connecting.\r\n");  
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	

	/* (Optional) Set a new I2C address if the wanted address is different
	* from the default one (filled with 0x20 for this example).
	*/
	//status = vl53l7cx_set_i2c_address(&Dev, 0x20);

	
	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	/* (Mandatory) Init VL53L7CX sensor */
    
	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);

    
	/*********************************/
	/*  Set ranging mode autonomous  */
	/*********************************/

	status = vl53l7cx_set_ranging_mode(&Dev, VL53L7CX_RANGING_MODE_AUTONOMOUS);
	if(status)
	{
		printf("vl53l7cx_set_ranging_mode failed, status %u\r\n", status);
		return status;
	}

	/* Using autonomous mode, the integration time can be updated (not possible
	 * using continuous) */
	status = vl53l7cx_set_integration_time_ms(&Dev, 20);

	/* Start a ranging session */
   	status = vl53l7cx_start_ranging(&Dev);
   	printf("Start ranging autonomous\r\n");

   	loop = 0;
   	while(loop < 1)
   	{
   		status = vl53l7cx_check_data_ready(&Dev, &isReady);
   		if(isReady)
   		{
   			vl53l7cx_get_ranging_data(&Dev, &Results);

   			/* As the sensor is set in 4x4 mode by default, we have a total
			 * of 16 zones to print. For this example, only the data of first zone are
			 * print */
			printf("Print data no : %3u\r\n", Dev.streamcount);
   			for(i = 0; i < 16; i++)
   			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i]);
   			}
			printf("\r\n");
   			loop++;
   		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
   		VL53L7CX_WaitMs(&(Dev.platform), 10);
   	}

   	status = vl53l7cx_stop_ranging(&Dev);
   	printf("Stop ranging autonomous\r\n");

	
	/*********************************/
	/* Set ranging mode continuous   */
	/*********************************/

	/* In continuous mode, the integration time cannot be programmed
	 * (automatically set to maximum value) */

	status = vl53l7cx_set_ranging_mode(&Dev, VL53L7CX_RANGING_MODE_CONTINUOUS);
	if(status)
	{
		printf("vl53l7cx_set_ranging_mode failed, status %u\r\n", status);
		return status;
	}

	/* Trying to update value below will have no impact on integration time */
	//status = vl53l7cx_set_integration_time_ms(&Dev, 20);

	/* Start a ranging session */
   	status = vl53l7cx_start_ranging(&Dev);
   	printf("Start ranging continuous\r\n");

   	loop = 0;
   	while(loop < 1)
   	{
   		status = vl53l7cx_check_data_ready(&Dev, &isReady);
   		if(isReady)
   		{
   			vl53l7cx_get_ranging_data(&Dev, &Results);

   			/* As the sensor is set in 4x4 mode by default, we have a total
			 * of 16 zones to print */
			printf("Print data no : %3u\r\n", Dev.streamcount);
   			for(i = 0; i < 16; i++)
   			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i]);
   			}
			printf("\r\n");
   			loop++;
   		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
   		VL53L7CX_WaitMs(&(Dev.platform), 5);
   	}

   	status = vl53l7cx_stop_ranging(&Dev);
   	printf("Stop ranging continuous\r\n");

   	printf("End of ULD demo\r\n");
    //putoff sensor I2C
    LPn_OUT_Write(0);  
   	return status;
}


/* Example 4: Switch the sensor between sleep and wake-up power modes. */
int example4(void)
{
	uint8_t status, isAlive, isReady, i;
	VL53L7CX_Configuration Dev;
	VL53L7CX_ResultsData Results;

	printf("Example4 Start: power mode switch\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n", VL53L7CX_API_REVISION);
	status = vl53l7cx_set_power_mode(&Dev, VL53L7CX_POWER_MODE_SLEEP);
	if(status)
	{
		printf("vl53l7cx_set_power_mode failed, status %u\r\n", status);
		return status;
	}

	printf("VL53L7CX is sleeping for 10s...\r\n");
	VL53L7CX_WaitMs(&(Dev.platform), 10000);

	status = vl53l7cx_set_power_mode(&Dev, VL53L7CX_POWER_MODE_WAKEUP);
	if(status)
	{
		printf("vl53l7cx_set_power_mode failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 5: Detect and print multiple targets per zone. */
int example5(void)
{
	uint8_t status, isAlive, isReady, i, j;
	VL53L7CX_Configuration Dev;
	VL53L7CX_ResultsData Results;

	printf("Example5 Start: multiple targets per zone\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	status = vl53l7cx_set_resolution(&Dev, VL53L7CX_RESOLUTION_8X8);
	if(status)
	{
		printf("vl53l7cx_set_resolution failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 64; i++)
			{
				printf("Zone %3u : nb=%2u target0=%4d mm ", i, Results.nb_target_detected[i], Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
				for(j = 1; j < VL53L7CX_NB_TARGET_PER_ZONE; j++)
				{
					printf("| %4d mm ", Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i + j]);
				}
				printf("\r\n");
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 6: Demonstrate I2C and RAM optimization settings. */
int example6(void)
{
	uint8_t status, isAlive;
	VL53L7CX_Configuration Dev;

	printf("Example6 Start: I2C and RAM optimization\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n", VL53L7CX_API_REVISION);
	printf("This example is kept as a lightweight check of the platform optimization settings.\r\n");
	return status;
}

/* Example 7: Calibrate and read the sensor cross-talk data. */
int example7(void)
{
	uint8_t status, isAlive, isReady, i;
	uint8_t xtalk_data[VL53L7CX_XTALK_BUFFER_SIZE];
	VL53L7CX_Configuration Dev;
	VL53L7CX_ResultsData Results;

	printf("Example7 Start: Xtalk calibration\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	printf("VL53L7CX ULD ready ! (Version : %s)\r\n", VL53L7CX_API_REVISION);
	status = vl53l7cx_calibrate_xtalk(&Dev, 3, 4, 600);
	if(status)
	{
		printf("vl53l7cx_calibrate_xtalk failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_get_caldata_xtalk(&Dev, xtalk_data);
	if(status)
	{
		printf("vl53l7cx_get_caldata_xtalk failed, status %u\r\n", status);
		return status;
	}

	printf("XTALK buffer bytes: ");
	for(i = 0; i < 8; i++)
	{
		printf("%02X ", xtalk_data[i]);
	}
	printf("\r\n");

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 8: Visualize the sensor cross-talk data and range results. */
int example8(void)
{
	uint8_t status, isAlive, isReady, i;
	uint8_t xtalk_data[VL53L7CX_XTALK_BUFFER_SIZE];
	VL53L7CX_Configuration Dev;
	VL53L7CX_ResultsData Results;

	printf("Example8 Start: visualize Xtalk data\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	status = vl53l7cx_get_caldata_xtalk(&Dev, xtalk_data);
	if(status)
	{
		printf("vl53l7cx_get_caldata_xtalk failed, status %u\r\n", status);
		return status;
	}

	printf("XTALK map preview:\r\n");
	for(i = 0; i < 16; i++)
	{
		printf("[%u] = %02X ", i, xtalk_data[i]);
	}
	printf("\r\n");

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}


int IntCount = 0;

static int WaitForL5Interrupt(VL53L7CX_Configuration *pDev)
{
	uint8_t isReady = 0u;
	if(pDev == NULL)
	{
		return 0;
	}

	if(vl53l7cx_check_data_ready(pDev, &isReady) == 0u && isReady)
	{
		IntCount++;
		return 1;
	}

	return 0;
}

/* Example 9: Configure detection thresholds and report threshold results. */
int example9(void)
{
	uint8_t status, isAlive, isReady, i;
	VL53L7CX_Configuration Dev;
	VL53L7CX_ResultsData Results;
	VL53L7CX_DetectionThresholds thresholds[VL53L7CX_NB_THRESHOLDS];

	printf("Example9 Start: detection thresholds\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	memset(&thresholds, 0, sizeof(thresholds));
	for(i = 0; i < 16; i++)
	{
		thresholds[i].zone_num = i;
		thresholds[i].measurement = VL53L7CX_DISTANCE_MM;
		thresholds[i].type = VL53L7CX_IN_WINDOW;
		thresholds[i].mathematic_operation = VL53L7CX_OPERATION_OR;
		thresholds[i].param_low_thresh = 200;
		thresholds[i].param_high_thresh = 400;
	}

	thresholds[15].zone_num = VL53L7CX_LAST_THRESHOLD | thresholds[15].zone_num;
	status = vl53l7cx_set_detection_thresholds(&Dev, thresholds);
	if(status)
	{
		printf("vl53l7cx_set_detection_thresholds failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_set_detection_thresholds_enable(&Dev, 1);
	if(status)
	{
		printf("vl53l7cx_set_detection_thresholds_enable failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		isReady = WaitForL5Interrupt(&Dev);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
					i,
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE * i],
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE * i]);
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 10: Configure and display the motion indicator. */
int example10(void)
{
	uint8_t status, isAlive, isReady, i;
	VL53L7CX_Configuration Dev;
	VL53L7CX_Motion_Configuration motion_config;
	VL53L7CX_ResultsData Results;

	printf("Example10 Start: motion indicator\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	status = vl53l7cx_motion_indicator_init(&Dev, &motion_config, VL53L7CX_RESOLUTION_4X4);
	if(status)
	{
		printf("Motion indicator init failed with status : %u\r\n", status);
		return status;
	}

	status = vl53l7cx_set_ranging_frequency_hz(&Dev, 2);
	if(status)
	{
		printf("vl53l7cx_set_ranging_frequency_hz failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			printf("Print data no : %3u\r\n", Dev.streamcount);
			for(i = 0; i < 16; i++)
			{
				printf("Zone : %3d, Motion power : %3lu\r\n",
					i,
					(unsigned long)Results.motion_indicator.motion[motion_config.map_id[i]]);
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 11: Combine the motion indicator with detection thresholds. */
int example11(void)
{
	uint8_t status, isAlive, isReady, i;
	VL53L7CX_Configuration Dev;
	VL53L7CX_Motion_Configuration motion_config;
	VL53L7CX_ResultsData Results;
	VL53L7CX_DetectionThresholds thresholds[VL53L7CX_NB_THRESHOLDS];

	printf("Example11 Start: motion indicator with thresholds\r\n");
	printf("Sensor Reset\r\n");
	VL53L7CX_Reset_Sensor(&(Dev.platform));
	printf("Choose sensor address before connecting.\r\n");
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected at requested address\r\n");
		return status;
	}

	status = vl53l7cx_init_timed(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}

	status = vl53l7cx_motion_indicator_init(&Dev, &motion_config, VL53L7CX_RESOLUTION_8X8);
	if(status)
	{
		printf("Motion indicator init failed with status : %u\r\n", status);
		return status;
	}

	memset(&thresholds, 0, sizeof(thresholds));
	for(i = 0; i < 64; i++)
	{
		thresholds[i].zone_num = i;
		thresholds[i].measurement = VL53L7CX_MOTION_INDICATOR;
		thresholds[i].type = VL53L7CX_GREATER_THAN_MAX_CHECKER;
		thresholds[i].mathematic_operation = VL53L7CX_OPERATION_NONE;
		thresholds[i].param_low_thresh = 44;
		thresholds[i].param_high_thresh = 44;
	}
	thresholds[63].zone_num = VL53L7CX_LAST_THRESHOLD | thresholds[63].zone_num;

	status = vl53l7cx_set_detection_thresholds(&Dev, thresholds);
	if(status)
	{
		printf("vl53l7cx_set_detection_thresholds failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_set_detection_thresholds_enable(&Dev, 1);
	if(status)
	{
		printf("vl53l7cx_set_detection_thresholds_enable failed, status %u\r\n", status);
		return status;
	}

	status = vl53l7cx_start_ranging(&Dev);
	if(status)
	{
		printf("vl53l7cx_start_ranging failed\r\n");
		return status;
	}

	while(1)
	{
		isReady = WaitForL5Interrupt(&Dev);
		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);
			for(i = 0; i < 64; i++)
			{
				if(Results.motion_indicator.motion[motion_config.map_id[i]] >= 44)
				{
					printf("Movement detected in this zone : %3d !\r\n", i);
				}
			}
			printf("\r\n");
			break;
		}
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	printf("End of ULD demo\r\n");
	return status;
}

/* Example 12: Get and set VL53L7CX ranging parameters and sends data to LabView. */
/* Configures resolution, frequency, target order, and reports acquisition data. */
#include "vl53l7cx_to_LabView.h"

static VL53L7CX_LV_Context labview;

static void LabView_WriteByte(uint8_t byte)
{
    UART_PutChar(byte);
}
int example12(void)
{

	/*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	uint8_t 				status, loop, isAlive, isReady, i, acquisition_status;
	uint8_t                 acquired_frames;
	uint32_t 				integration_time_ms;
	VL53L7CX_Configuration 	Dev;			/* Sensor configuration */
	VL53L7CX_ResultsData 	Results;		/* Results data from VL53L7CX */
	uint8_t                 streamcounts[10];
	uint32_t                acquisition_durations_us[10];
	uint8_t                 target_statuses[10][VL53L7CX_RESOLUTION_8X8];
	int16_t                 distances_mm[10][VL53L7CX_RESOLUTION_8X8];

	
	/*********************************/
	/*      Customer platform        */
	/*********************************/

    printf("Example12 Start: send ranging data to LabVIEW\r\n");
    
    VL53L7CX_Reset_Sensor(&(Dev.platform));
    
	/* Fill the platform structure with customer's implementation. For this
	* example, only the I2C address is used.
	*/
	printf("Choose sensor address before connecting.\r\n");  
	Dev.platform.address = prompt_sensor_address_8bit(VL53L7CX_DEFAULT_I2C_ADDRESS);
   	printf("Using sensor 8-bit I2C address 0x%02X\r\n", Dev.platform.address);

	/* (Optional) Reset sensor toggling PINs (see platform, not in API) */
	//VL53L7CX_Reset_Sensor(&(Dev.platform));

	/* (Optional) Set a new I2C address if the wanted address is different
	* from the default one (filled with 0x20 for this example).
	*/
	//status = vl53l7cx_set_i2c_address(&Dev, 0x20);
	

	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

	/* (Optional) Check if there is a VL53L7CX sensor connected */
    
	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		printf("VL53L7CX not detected status: %u\r\n", status);
        
		return status;
	}
    printf("VL53L7CX detected at 8 bit address 0x%02X\r\n",Dev.platform.address );
    
	/* (Mandatory) Init VL53L7CX sensor */
    Test_Timer_msec_WriteCounter(Test_Timer_msec_INIT_PERIOD );    
	status = vl53l7cx_init(&Dev);
	if(status)
	{
		printf("VL53L7CX ULD Loading failed\r\n");
		return status;
	}
	printf("vl53l7cx_init time: %u us\r\n", (uint32_t)(Test_Timer_msec_INIT_PERIOD - Test_Timer_msec_ReadCounter())*10);
	printf("VL53L7CX ULD ready ! (Version : %s)\r\n",
			VL53L7CX_API_REVISION);
	
    
	/*********************************/
	/*        Set some params        */
	/*********************************/

	/* Set resolution in 8x8. WARNING : As others settings depend to this
	 * one, it must be the first to use.
	 */
    //CyDelay(20000);
	status = vl53l7cx_set_resolution(&Dev, VL53L7CX_RESOLUTION_8X8);
	if(status)
	{
		printf("vl53l7cx_set_resolution failed, status %u\r\n", status);
		return status;
	}

	/* Set ranging frequency .
	 * Using 4x4, min frequency is 1Hz and max is 60Hz
	 * Using 8x8, min frequency is 1Hz and max is 15Hz
	 */
	status = vl53l7cx_set_ranging_frequency_hz(&Dev, 10);
	if(status)
	{
		printf("vl53l7cx_set_ranging_frequency_hz failed, status %u\r\n", status);
		return status;
	}

	/* Set target order to closest */
	status = vl53l7cx_set_target_order(&Dev, VL53L7CX_TARGET_ORDER_CLOSEST);
	if(status)
	{
		printf("vl53l7cx_set_target_order failed, status %u\r\n", status);
		return status;
	}

    
	/* Get current integration time */
	status = vl53l7cx_get_integration_time_ms(&Dev, &integration_time_ms);
	if(status)
	{
		printf("vl53l7cx_get_integration_time_ms failed, status %u\r\n", status);
		return status;
	}
	printf("Current integration time is : %d ms\r\n", (int)integration_time_ms);
    integration_time_ms = 100;
    status = vl53l7cx_set_integration_time_ms(&Dev,20);
    if(status)
	{
		printf("vl53l7cx_set_integration_time_ms failed, status %u\r\n", status);
		return status;
	}
    printf("New integration time is : %d ms\r\n", (int)integration_time_ms);
	
    
    status = VL53L7CX_LV_Init(&labview, LabView_WriteByte);
    if(status != VL53L7CX_LV_OK)
    {
        printf("VL53L7CX_LV_Init failed, status %u\r\n", status);
        return status;
    }
    
    /*********************************/
	/*         Ranging loop          */
	/*********************************/

    Sys_Timer_msec_WriteCounter(Sys_Timer_msec_INIT_PERIOD);
	status = vl53l7cx_start_ranging(&Dev);
    if(status)
    {
        printf("vl53l7cx_start_ranging failed, status %u\r\n", status);
        return status;
    }
	unsigned int nb_frames = 10;
    printf("Example12 ranging %u frames\r\n", nb_frames);
	loop = 0;
    acquisition_status = VL53L7CX_LV_OK;
	while(loop < nb_frames)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
        Test_Timer_msec_WriteCounter(Test_Timer_msec_INIT_PERIOD );    
        isReady = 0u;
		status = vl53l7cx_check_data_ready(&Dev, &isReady);
        while(!isReady && !status){
            status = vl53l7cx_check_data_ready(&Dev, &isReady);
        }
        if(status)
        {
            printf("vl53l7cx_check_data_ready failed, status %u\r\n", status);
            acquisition_status = status;
            break;
        }
        if(isReady)
		{
			status = vl53l7cx_get_ranging_data(&Dev, &Results);
            if(status)
            {
                printf("vl53l7cx_get_ranging_data failed, status %u\r\n", status);
                acquisition_status = status;
                break;
            }

            status = VL53L7CX_LV_SendResults(
                &labview, &Results,
                Sys_Timer_msec_INIT_PERIOD - Sys_Timer_msec_ReadCounter());
            if(status != VL53L7CX_LV_OK)
            {
                printf("VL53L7CX_LV_SendResults failed, status %u\r\n", status);
                acquisition_status = status;
                break;
            }

			streamcounts[loop] = Dev.streamcount;
            acquisition_durations_us[loop] =
                (uint32_t)(Test_Timer_msec_INIT_PERIOD - Test_Timer_msec_ReadCounter()) * 10u;
			for(i = 0; i < 64; i++)
			{
				target_statuses[loop][i] =
					Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i];
				distances_mm[loop][i] =
					Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i];
			}
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		//VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
    acquired_frames = loop;

	for(loop = 0; loop < acquired_frames; loop++)
	{
		printf("Print data no : %u\r\n", streamcounts[loop]);
        printf("acq duration: %lu us\r\n", (unsigned long)acquisition_durations_us[loop]);
		for(i = 0; i < 64; i++)
		{
			printf("Zone : %3d, Status : %3u, Distance : %4d mm\r\n",
				i,
				target_statuses[loop][i],
				distances_mm[loop][i]);
		}
		printf("\r\n");
	}
    
    //putoff sensor I2C
    //LPn_OUT_Write(0);  
    
	printf("End of ULD demo\r\n");
    if(acquisition_status != VL53L7CX_LV_OK)
    {
        return acquisition_status;
    }
	return status;
}
