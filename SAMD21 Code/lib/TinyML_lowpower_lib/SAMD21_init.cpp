/*
 * Copyright (c) 2021 Syntiant Corp.  All rights reserved.
 * Contact at http://www.syntiant.com
 *
 * This software is available to you under a choice of one of two licenses.
 * You may choose to be licensed under the terms of the GNU General Public
 * License (GPL) Version 2, available from the file LICENSE in the main
 * directory of this source tree, or the OpenIB.org BSD license below.  Any
 * code involving Linux software will require selection of the GNU General
 * Public License (GPL) Version 2.
 *
 * OPENIB.ORG BSD LICENSE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <NDP.h>
#include <wiring_private.h>
#include "NDP_loadModel.h"
#include "SAMD21_init.h"


void SAMD21_init(byte waitBeforeSerialPort){
 // Serial.begin(115200);
  
 
  //Serial.println("");
  //Serial.println("                    Starting Syntiant TinyML");
  // ********** Setting SAMD21 pins for specific usage and avoiding floating nodes to save power *******

  pinMode(PORSTB, OUTPUT);                                //Reset pin for NDP
  pinMode(TINYML_CS, OUTPUT);                             //Chip select for NDP's SPI interface
  pinMode(FLASH_CS, OUTPUT);
  pinMode(ENABLE_PDM, OUTPUT);                            //Pin which controls microphone usage or sensor usage
  pinMode(NDP_INT, INPUT);                         //NDP interrupt pin
  digitalWrite(ENABLE_PDM, HIGH);                         // Disable PDM clock with external buffer to avoid contention is model is sensor related
  pinPeripheral(OSC32K_OUT_PIN, PIO_AC_CLK);              // Route OSC32K to PA11 pin for NDP clock
}