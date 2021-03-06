/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2018, This. Is. IoT. - https://thisisiot.io
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \addtogroup cc13xx-cc26xx-platform
 * @{
 *
 * \file
 *        Setup the Keystone platform with the
 *        Contiki environment.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 *        Evan Ross <evan@thisisiot.io>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "dev/button-hal.h"
#include "dev/gpio-hal.h"
#include "dev/serial-line.h"
#include "dev/leds.h"
#include "net/mac/framer/frame802154.h"
#include "lib/random.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <NoRTOS.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/TRNG.h>
#include <ti/drivers/UART.h>
#include "PDMCC26XX_contiki.h"
/*---------------------------------------------------------------------------*/
#include "board-peripherals.h"
#include "uart0-arch.h"
#include "trng-arch.h"
/*---------------------------------------------------------------------------*/
#include "rf/rf.h"
#include "rf/ble-beacond.h"
#include "rf/ieee-addr.h"
#include "rf/dot-15-4g.h"
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "IoT.Keystone"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/*
 * Board-specific initialization function. This function is defined in
 * the <BOARD>_fxns.c file.
 */
extern void Board_initHook(void);
/*---------------------------------------------------------------------------*/
/* Singleton instance handles for device drivers SPI, I2C, I2S */
SPI_Handle hSpiInternal;
SPI_Handle hSpiSensor;
I2C_Handle hI2cSensor;

/*---------------------------------------------------------------------------*/
/* List of all sensors in the Keystone platform to bind with
 * the sensor module 
 */

/* Note: BME280 sensor driver is poorly designed:
 *   - Initialization is triggered at sensors process start and we can't
 *     detect failures there.
 *   - Value function triggers full readout but only returns one value.
 * Use its bme280.h API directly.
 *
 * Motion sensor init is done by application to catch init failures.
 */
SENSORS(/* &bme280_sensor, */ &opt_3001_sensor, &audio_sensor /* &motion_sensor */);

/*---------------------------------------------------------------------------*/

/*
 * \brief  Fade a specified LED.
 */
static void
fade(PIN_Id pin)
{
  volatile uint32_t i;
  uint32_t k;
  uint32_t j;
  uint32_t pivot = 800;
  uint32_t pivot_half = pivot / 2;

  for(k = 0; k < pivot; ++k) {
    j = (k > pivot_half) ? pivot - k : k;

    PINCC26XX_setOutputValue(pin, 1);
    for(i = 0; i < j; ++i) {
      __asm__ __volatile__ ("nop");
    }
    PINCC26XX_setOutputValue(pin, 0);
    for(i = 0; i < pivot_half - j; ++i) {
      __asm__ __volatile__ ("nop");
    }
  }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Configure RF params for the radio driver.
 */
static void
set_rf_params(void)
{
  uint8_t ext_addr[8];
  uint16_t short_addr;

  memset(ext_addr, 0x0, sizeof(ext_addr));

  ieee_addr_cpy_to(ext_addr, sizeof(ext_addr));

  /* Short address is the last two bytes of the MAC address */
  short_addr = (((uint16_t)ext_addr[7] << 0) |
                ((uint16_t)ext_addr[6] << 8));

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, sizeof(ext_addr));
}
/*---------------------------------------------------------------------------*/

/*
 * Open the SPI instance as a master to manage all traffic on the specified bus.
 */
static void
spi_open(uint8_t index, SPI_Handle* hInst, uint32_t bitRate, SPI_FrameFormat format)
{
    SPI_Params spi_params;

    /* Ensure the provided SPI index is valid. */
    if (index >= SPI_CONF_CONTROLLER_COUNT) {
        return;
    }
    if (hInst == NULL) {
        return;
    }

    SPI_Params_init(&spi_params);

    spi_params.transferMode = SPI_MODE_BLOCKING;
    spi_params.mode = SPI_MASTER;
    spi_params.bitRate = bitRate;
    spi_params.dataSize = 8;
    spi_params.frameFormat = format;

    /*
    * Try to open the SPI driver. Accessing the SPI driver also ensures
    * atomic access to the SPI interface.
    */
    *hInst = SPI_open(index, &spi_params);
}

/*---------------------------------------------------------------------------*/

/*
 * Open the single I2C instance as a master to manage all traffic.
 */
static void
i2c_open()
{
    I2C_Params i2c_params;
    I2C_Params_init(&i2c_params);

    i2c_params.transferMode = I2C_MODE_BLOCKING;
    i2c_params.bitRate = I2C_400kHz;

    hI2cSensor = I2C_open(Board_I2C0, &i2c_params);
}

/*---------------------------------------------------------------------------*/

void
platform_init_stage_one(void)
{
  DRIVERLIB_ASSERT_CURR_RELEASE();

  /* Enable flash cache */
  VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
  /* Configure round robin arbitration and prefetching */
  VIMSConfigure(VIMS_BASE, true, true);

  Power_init();

  /* BoardGpioInitTable declared in Board.h */
  if(PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
    /*
     * Something is seriously wrong if PIN initialization of the Board GPIO
     * table fails.
     */
    for(;;) { /* hang */ }
  }

  /* Perform board-specific initialization */
  Board_initHook();

  /* Contiki drivers init */
  gpio_hal_init();
  leds_init();

  fade(Board_PIN_LED0);

  /* TI Drivers init */
#if TI_UART_CONF_ENABLE
  UART_init();
#endif
#if TI_I2C_CONF_ENABLE
  I2C_init();
  i2c_open();
  if (hI2cSensor == NULL) {
      /*
      * Something is seriously wrong if I2C initialization fails.
      */
      for (;;) { /* hang */ }
  }
#endif
#if TI_SPI_CONF_ENABLE
  SPI_init();

  /* SX1262 is a MODE 0 device. */
  spi_open(0, &hSpiInternal, 4000000, SPI_POL0_PHA0); /* max speed SX1262=16MHz*/

    /*!NOTICE!
    *  ICM-20948 requires SPI MODE 3 (POL=1, PHA=1) to function.
    *  BME-280 supports either MODE 0 or MODE 3.
    *  Therefore, we can set the bus to mode 3.
    */
  spi_open(1, &hSpiSensor, 4000000, SPI_POL1_PHA1); /* max speed ICM20948=7MHz */
  if (hSpiInternal == NULL || hSpiSensor == NULL) {
      /*
      * Something is seriously wrong if SPI initialization fails.
      */
      for (;;) { /* hang */ }
  }
#endif
#if TI_NVS_CONF_ENABLE
  NVS_init();
#endif
  /* Init PDM driver and I2S hardware as per TI driver procedure.
   * Audio sensor driver is responsible for opening an instance. 
   */
  PDMCC26XX_Contiki_init((PDMCC26XX_Handle)&PDMCC26XX_config[0]);

  TRNG_init();

  fade(Board_PIN_LED1);

  /* NoRTOS must be called last */
  NoRTOS_start();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  serial_line_init();

#if TI_UART_CONF_UART0_ENABLE
  uart0_init();
#endif

#if BUILD_WITH_SHELL
  uart0_set_callback(serial_line_input_byte);
#endif

  /* Use TRNG to seed PRNG. If TRNG fails, use a hard-coded seed. */
  unsigned short seed = 0;
  if(!trng_rand((uint8_t *)&seed, sizeof(seed), TRNG_WAIT_FOREVER)) {
    /* Default to some hard-coded seed. */
    seed = 0x1234;
    LOG_WARN("TRNG random seeding failed.\n");
  }
  random_init(seed);

  /* Populate linkaddr_node_addr */
  ieee_addr_cpy_to(linkaddr_node_addr.u8, LINKADDR_SIZE);

#if PLATFORM_HAS_BUTTON
  button_hal_init();
#endif

  fade(Board_PIN_LED0);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
#if RF_CONF_BLE_BEACON_ENABLE
  rf_ble_beacond_init();
#endif

  radio_value_t chan = 0;
  radio_value_t pan = 0;

  set_rf_params();

  LOG_DBG("With DriverLib v%u.%u\n", DRIVERLIB_RELEASE_GROUP,
          DRIVERLIB_RELEASE_BUILD);
  LOG_DBG("IEEE 802.15.4: %s, Sub-1 GHz: %s, BLE: %s\n",
          ChipInfo_SupportsIEEE_802_15_4() ? "Yes" : "No",
          ChipInfo_SupportsPROPRIETARY() ? "Yes" : "No",
          ChipInfo_SupportsBLE() ? "Yes" : "No");

#if (RF_MODE == RF_MODE_SUB_1_GHZ)
  LOG_INFO("Operating frequency on Sub-1 GHz band " DOT_15_4G_BAND_NAME " \n");
#elif (RF_MODE == RF_MODE_2_4_GHZ)
  LOG_INFO("Operating frequency on 2.4 GHz\n");
#endif

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  LOG_INFO("RF: Channel %d\n", chan);

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &pan) == RADIO_RESULT_OK) {
    LOG_INFO("PANID 0x%04X\n", pan);
  }
  LOG_INFO("Node ID: %d\n", node_id);

  process_start(&sensors_process, NULL);

  fade(Board_PIN_LED1);
}
/*---------------------------------------------------------------------------*/
void
platform_idle(void)
{
  /* Drop to some low power mode */
  Power_idleFunc();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
