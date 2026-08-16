/* Pinout of this board (see Platine2 schematic). The generic example that ships with
 * HBWired is HBW-1W-T10_config_example.h - this file replaces it for this hardware. */

// ATmega328P-A with 16 MHz resonator, RS485 via MAX487E on Platine1.

EEPROMClass* EepromPtr = &EEPROM;  // use internal EEPROM

/* The board wires UART0 straight to the MAX487E and uses D2 as transmit enable,
 * so the SoftwareSerial variant (which would need D2 for TX) does not apply here.
 * Comment this out only for bench testing with a Nano, then RS485 moves to
 * SoftwareSerial and UART0 becomes the debug output. */
#define USE_HARDWARE_SERIAL

/* Undefine "HBW_DEBUG" in 'HBWired.h' to remove unneeded code. "HBW_DEBUG" also works as
 * master switch, as hbwdebug() or hbwdebughex() used in channels will point to empty functions. */

// Pins
#ifdef USE_HARDWARE_SERIAL
  #define RS485_TXEN 2  // D2  - Transmit-Enable (schematic net "TXEN")
  #define BUTTON A6     // A6  - Button fuer Factory-Reset etc. (schematic net "Button")
  #define LED 13        // D13 - Signal-LED
  #define IDENTIFY_LED 12  // D12/PB4 - Identify-LED (schematic net "ID_LED"), auskommentieren wenn nicht bestueckt

  #define ONEWIRE_PIN	10 // D10 - Onewire Bus (schematic net "1WIRE", 4k7 pullup on board)

#else
  // bench setup on an Arduino Nano, RS485 via SoftwareSerial, UART0 = debug
  #define RS485_RXD 4
  #define RS485_TXD 2
  #define RS485_TXEN 3
  #define BUTTON 5
  #define LED 13
  #define IDENTIFY_LED 12

  #define ONEWIRE_PIN	10

  #include "FreeRam.h"
  #include <HBWSoftwareSerial.h>
  HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
#endif  //USE_HARDWARE_SERIAL
