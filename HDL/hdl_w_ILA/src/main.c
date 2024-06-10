/***************************************************************************//**
 *   @file   adaq8092_fmc.c
 *   @brief  ADAQ8092_FMC Application
 *   @author Antoniu Miclaus (antoniu.miclaus@analog.com)
********************************************************************************
 * Copyright 2022(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "xil_cache.h"
#include "no_os_spi.h"
#include "no_os_error.h"
#include "no_os_print_log.h"
#include "no_os_delay.h"
#include "xilinx_spi.h"

#include "ad5592r.h"
#include "axi_adc_core.h"

#define SPI_DEVICE_ID				0


#define AXI_ADC_DATA_CHANNEL_0		0x0042C
#define AXI_ADC_DATA_CHANNEL_1		0x0046C
#define AXI_ADC_DATA_CHANNEL_2		0x004AC
#define AXI_ADC_DATA_CHANNEL_3		0x004EC

#define RX_CORE_BASEADDR            0x44a00000
/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/

/***************************************************************************//**
* @brief main
*******************************************************************************/
int main(void)
{

	uint16_t value,value1,value2,value3;
    int ret, status;


	struct xil_spi_init_param xil_spi_init = {
		.flags = 0,
		.type = SPI_PS
	};

	 struct ad5592r_dev my_ad5592;

	struct ad5592r_init_param default_init_param = {
	 		/* SPI */
	 		.spi_init = {
	 			.device_id = SPI_DEVICE_ID,
	 			.max_speed_hz = 1000000u,
	 			.chip_select = 0,
	 			.mode = NO_OS_SPI_MODE_2,
	 			.platform_ops = &xil_spi_ops,
	 			.extra = &xil_spi_init
	 		}
	 	};
	 ad5592r_init(&my_ad5592,&default_init_param);

		/* ADC Core */
	struct axi_adc_init axi_pwm_custom_param = {
			.name = "axi_pwm_custom_core",
			.num_channels = 4,
			.base = RX_CORE_BASEADDR
		};
		struct axi_adc_init *axi_pwm_custom_core;

		ret = axi_adc_init(&axi_pwm_custom_core,  &axi_pwm_custom_param);
		if (ret) {
			pr_err("axi_adc_init() error: %s\n", axi_pwm_custom_core->name);
			return ret;
		}

while(1)
	 {
		 ad5592r_read_adc(&my_ad5592,0,&value);
		 ad5592r_read_adc(&my_ad5592,1,&value1);
		 ad5592r_read_adc(&my_ad5592,2,&value2);
		 ad5592r_read_adc(&my_ad5592,3,&value3);

		 status = axi_adc_write(axi_pwm_custom_core, AXI_ADC_DATA_CHANNEL_0, value & 0x0fff);
		 	if (status)
		 		return status;
	     status = axi_adc_write(axi_pwm_custom_core, AXI_ADC_DATA_CHANNEL_1, value1 & 0x0fff);
			 if (status)
			 	return status;
		 status = axi_adc_write(axi_pwm_custom_core, AXI_ADC_DATA_CHANNEL_2, value2 & 0x0fff);
			  if (status)
				 return status;
		 status = axi_adc_write(axi_pwm_custom_core, AXI_ADC_DATA_CHANNEL_3, value3 & 0x0fff);
				 	if (status)
				 		return status;


		 printf("%d,%d,%d,%d\n",value & 0x0fff,value1 & 0x0fff, value2 & 0x0fff, value3 & 0x0fff);

	 }

	return 0;
}
