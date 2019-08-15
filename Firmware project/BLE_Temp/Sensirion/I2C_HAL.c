//==============================================================================
//    S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
//==============================================================================
// Project   :  SHT2x Sample Code (V1.2)
// File      :  I2C_HAL.c
// Author    :  MST
// Controller:  NEC V850/SG3 (uPD70F3740)
// Compiler  :  IAR compiler for V850 (3.50A)
// Brief     :  I2C Hardware abstraction layer
//==============================================================================

//---------- Includes ----------------------------------------------------------
#include "I2C_HAL.h"
#include "nrf_delay.h"

//==============================================================================
void I2c_Init(void)
//==============================================================================
{
  nrf_gpio_cfg_output(SDA_Pin); nrf_gpio_pin_clear(SDA_Pin);                // Set port as output for configuration
  nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);                 // Set port as output for configuration

  //#SDA_CONF=LOW;           // Set SDA level as low for output mode
  //SCL_CONF=LOW;           // Set SCL level as low for output mode

  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);               // I2C-bus idle mode SDA released (input)
  nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);               // I2C-bus idle mode SCL released (input)
}

//==============================================================================
void I2c_StartCondition(void)
//==============================================================================
{
  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL); 
  nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL); 
  nrf_gpio_cfg_output(SDA_Pin); nrf_gpio_pin_clear(SDA_Pin);
  DelayMicroSeconds(10);  // hold time start condition (t_HD;STA)
  nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);
  DelayMicroSeconds(10);
}

//==============================================================================
void I2c_StopCondition(void)
//==============================================================================
{
  nrf_gpio_cfg_output(SDA_Pin); nrf_gpio_pin_clear(SDA_Pin);
  nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin); 
  nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);
  DelayMicroSeconds(10);  // set-up time stop condition (t_SU;STO)
  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);
  DelayMicroSeconds(10);
}

//==============================================================================
u8t I2c_WriteByte (u8t txByte)
//==============================================================================
{
  u8t mask,error=0;
  for (mask=0x80; mask>0; mask>>=1)   //shift bit for masking (8 times)
  { 
		if ((mask & txByte) == 0)
		{
			nrf_gpio_cfg_output(SDA_Pin); nrf_gpio_pin_clear(SDA_Pin);//masking txByte, write bit to SDA-Line
		}
    else 
		{
			nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);
		}
    DelayMicroSeconds(1);             //data set-up time (t_SU;DAT)
    nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);                        //generate clock pulse on SCL
    DelayMicroSeconds(5);             //SCL high time (t_HIGH)
    nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);
    DelayMicroSeconds(1);             //data hold time(t_HD;DAT)
  }
  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);                           //release SDA-line
  nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);                           //clk #9 for ack
  DelayMicroSeconds(1);               //data set-up time (t_SU;DAT)
  if(nrf_gpio_pin_read(SDA_Pin) == 1)
	{
		error=ACK_ERROR; //check ack from i2c slave
	}
  nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);
  DelayMicroSeconds(20);              //wait time to see byte package on scope
  return error;                       //return error code
}

//==============================================================================
u8t I2c_ReadByte (etI2cAck ack)
//==============================================================================
{
  u8t mask,rxByte=0;
  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);                          //release SDA-line
  for (mask=0x80; mask>0; mask>>=1)   //shift bit for masking (8 times)
  { 
		nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);                        //start clock on SCL-line
    DelayMicroSeconds(1);             //data set-up time (t_SU;DAT)
    DelayMicroSeconds(3);             //SCL high time (t_HIGH)
    if (nrf_gpio_pin_read(SDA_Pin) == 1)
		{
			rxByte=(rxByte | mask); //read bit
		}
    nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);
    DelayMicroSeconds(1);             //data hold time(t_HD;DAT)
  }
	if (ack)
	{
		nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL); 
	}
	else
	{
		nrf_gpio_cfg_output(SDA_Pin); nrf_gpio_pin_clear(SDA_Pin);
	}                            //send acknowledge if necessary
  DelayMicroSeconds(1);               //data set-up time (t_SU;DAT)
  nrf_gpio_cfg_input(SCL_Pin, NRF_GPIO_PIN_NOPULL);                             //clk #9 for ack
  DelayMicroSeconds(5);               //SCL high time (t_HIGH)
  nrf_gpio_cfg_output(SCL_Pin); nrf_gpio_pin_clear(SCL_Pin);
  nrf_gpio_cfg_input(SDA_Pin, NRF_GPIO_PIN_NOPULL);                          //release SDA-line
  DelayMicroSeconds(20);              //wait time to see byte package on scope
  return rxByte;                      //return error code
}

//==============================================================================
void DelayMicroSeconds (u32t nbrOfUs)
//==============================================================================
{
	nrf_delay_us(nbrOfUs);
}



