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
 * \addtogroup keystone-platform
 * @{
 *
 * \defgroup Keystone-peripherals
 *
 * Defines related to configuring Keystone peripherals. 
 *
 * @{
 *
 * \file
 *        Header file with definitions related to Keystone boards.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 *        Evan Ross <evan@thisisiot.io>
 * \note
 *        This file should not be included directly
 */
/*---------------------------------------------------------------------------*/
#ifndef BOARD_CONF_H_
#define BOARD_CONF_H_
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"
/*---------------------------------------------------------------------------*/
/**
 * \name LED configurations for the dev/leds.h API.
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define PLATFORM_HAS_LEDS           1

#define LEDS_CONF_COUNT             2

#define LEDS_CONF_RED               0
#define LEDS_CONF_GREEN             1

#define LEDS_CONF_ALL               ((1 << LEDS_CONF_COUNT) - 1)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Button configurations for the dev/button-hal.h API.
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define PLATFORM_HAS_BUTTON           0
#define PLATFORM_SUPPORTS_BUTTON_HAL  0

#define BUTTON_HAL_ID_KEY_LEFT      0
#define BUTTON_HAL_ID_KEY_RIGHT     1
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Keystone board r1 has sensors.
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define BOARD_CONF_HAS_SENSORS      1
/** @} */
/*---------------------------------------------------------------------------*/
/**
* \name Keystone board r1 SPI1 is enabled as a sensor bus.
*
* @{
*/
#define TI_SPI_CONF_SPI1_ENABLE     1
/** @} */
/*---------------------------------------------------------------------------*/
/**
* \name Keystone board r1 sub-GHz band
*
*  Needs to be set to 868 for EU board variants.
* @{
*/
#define DOT_15_4G_CONF_FREQ_BAND_ID     DOT_15_4G_FREQ_BAND_915
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name The external flash SPI CS pin, defined in Board.h.
 *
 * Note that SPI SCK, MOSI and MISO does not need to be defined, as they are
 * implicitly defined via the Board_SPI0 controller.
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#if TI_SPI_CONF_SPI0_ENABLE
#define EXT_FLASH_SPI_CONTROLLER      Board_SPI0

#define EXT_FLASH_SPI_PIN_SCK         Board_SPI0_SCK
#define EXT_FLASH_SPI_PIN_MOSI        Board_SPI0_MOSI
#define EXT_FLASH_SPI_PIN_MISO        Board_SPI0_MISO
#define EXT_FLASH_SPI_PIN_CS          Board_SPI_FLASH_CS

#define EXT_FLASH_DEVICE_ID           0x14
#define EXT_FLASH_MID                 0xC2

#define EXT_FLASH_PROGRAM_PAGE_SIZE   256
#define EXT_FLASH_ERASE_SECTOR_SIZE   4096
#endif /* TI_SPI_CONF_SPI0_ENABLE */
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* BOARD_CONF_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
