#ifndef EXAMPLES_H
#define EXAMPLES_H

#include <stdint.h>
#include "vl53l7cx_api.h"

uint8_t prompt_sensor_address_8bit(uint8_t defaultAddress8bit);
uint8_t vl53l7cx_init_timed(VL53L7CX_Configuration *p_dev);

int example0(void);
int example1(void);
int example2(void);
int example3(void);
int example4(void);
int example5(void);
int example6(void);
int example7(void);
int example8(void);
int example9(void);
int example10(void);
int example11(void);
int example12(void);

#endif
