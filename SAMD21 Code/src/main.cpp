#include "main.h"
#include <NDP.h>
#include <NDP_utils.h>
#include <Arduino.h>
#include "TinyML_init.h"
#include "NDP_init.h"
#include "NDP_loadModel.h"
#include "SAMD21_init.h"
#include "SAMD21_lowpower.h"
#include "LowPower.h"
#include "hostcmd.h"

Uart ext(&sercom3, 7, 6, SERCOM_RX_PAD_3, UART_TX_PAD_2); // UART for communication with host mcu

static volatile classifier_match_t s_match;
static void ndp_isr(void) { s_match = (classifier_match_t)NDP.poll(); }



void SERCOM3_Handler()
{
    ext.IrqHandler();
}

void service_ndp() {

  hostcmd_send_classifier_match(s_match);
  
  switch (s_match) {

  case GLASS_BREAK:
    digitalWrite(LED_BLUE, HIGH);
    delay(250);
    digitalWrite(LED_BLUE, LOW);
    break;

  case Z_OPENSET:
    digitalWrite(LED_GREEN, HIGH);
    delay(250);
    digitalWrite(LED_GREEN, LOW);
    break;

  default:
    break;
  }
}


void setup(void) {
  // Initialize the SAMD21 host processor
  SAMD21_init(0);

  ext.begin(115200);
  //pinPeripheral(6, PIO_SERCOM_ALT);                       //The 7th pin of TinyML board is setup for Serial2 debug. Please not 6 is Arduino pin mapping
  //pinPeripheral(7, PIO_SERCOM_ALT);                     // Since there is nothing expected from serial2 this pin is commented out

  NDP_init("ei_model.bin", 0);

  attachInterrupt(NDP_INT, ndp_isr, HIGH);
  
  // Prevent the internal flash memory from powering down in sleep mode
  NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
  // Select STANDBY for sleep mode
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
   usb_serial_disable(); // See note above about USB communications
}

void loop() {

  // Put various peripheral modules into low power mode before entering standby.
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_RED, LOW);
  adc_disable();        // See SAMD21_lowpower.h
 
  systick_disable();
  // Complete any memory operations and enter standby mode until an interrupt
  // is recieved from the NDP101
  __DSB();
  __WFI();
  
  // Arrive here after waking and having called ndp_isr().  Re-enable various
  // peripheral modules before servicing the NDP classifier data.
  systick_enable();
 // usb_serial_enable();
  adc_enable();
  // process the classifier data from the NDP
  service_ndp();
}
