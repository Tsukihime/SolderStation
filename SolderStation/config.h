#ifndef CONFIG_H_
#define CONFIG_H_

#include "AVRPin.hpp"

//#define SOFTUART 1

using FanLedPin = Pc5;
using SolderLedPin = Pc0;
using FanPWMPin = Pb3;
using FanSeatSwitchPin = Pc3;

using LCDAnode1Pin = Pd4;
using LCDAnode2Pin = Pd5;
using LCDAnode3Pin = Pd6;
using LCDBPin = Pd7;

using FanHeaterPin = Pd0;
using SolderHeaterPin = Pd1;

const uint8_t FAN_THRESHOLD_TEMP = 50;
const uint8_t FAN_HYSTERESIS_TEMP = 15;
const uint8_t FAN_COOLING_TIMEOUT = 10; // second
const uint8_t FAN_TEMP_ADC_CH = 7;
const uint8_t FAN_AIR_ADC_CH = 2;
const uint8_t SOLDER_TEMP_ADC_CH = 6;
const uint8_t BUTTONS_ADC_CH = 4;

const uint8_t FAN_AIR_FLOW_MAX = 255; // pwm
const uint8_t FAN_AIR_FLOW_MIN = 128; // pwm
const uint8_t LONG_PRESS_DELAY = 100; // 1 second

const uint16_t TEMPERATURE_MIN = 100;
const uint16_t TEMPERATURE_MAX = 500;

const uint16_t LCD_BLINK_DELAY = 500; // 500 ms
const uint16_t CHANGE_MODE_DELAY = 300; // 3000 ms

#endif /* CONFIG_H_ */
