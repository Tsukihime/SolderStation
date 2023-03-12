#include <avr/interrupt.h>
#include "stdlib.h"

#include "peripherals.h"
#include "config.h"
#include "Scheduler.h"
#include "adc.hpp"
#include "calibrator.h"
#include "lcd.h"
#include "utils.h"

const uint16_t PFC_delay[101] = {10000,
    8840, 8531, 8310, 8132, 7980, 7846, 7724, 7612, 7508, 7411,
    7319, 7231, 7147, 7067, 6990, 6915, 6842, 6772, 6704, 6637,
    6572, 6508, 6445, 6384, 6324, 6264, 6206, 6149, 6092, 6036,
    5980, 5926, 5871, 5818, 5765, 5712, 5659, 5607, 5556, 5504,
    5453, 5402, 5351, 5301, 5251, 5200, 5150, 5100, 5050, 5000,
    4950, 4900, 4850, 4800, 4749, 4699, 4649, 4598, 4547, 4496,
    4444, 4393, 4341, 4288, 4235, 4182, 4129, 4074, 4020, 3964,
    3908, 3851, 3794, 3736, 3676, 3616, 3555, 3492, 3428, 3363,
    3296, 3228, 3158, 3085, 3010, 2933, 2853, 2769, 2681, 2589,
    2492, 2388, 2276, 2154, 2020, 1868, 1690, 1469, 1160, 18,
};

inline void timer1_start(uint16_t delay_us) {
    TCNT1 = 0;
    ICR1 = delay_us;
    TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10); // prescaler 1/8 = 1us per increment
}

inline void timer1_stop() {
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
}

const uint8_t ON_OFF_DELAY = 3;
const uint8_t POWER_STEPS = 100; // number of power levels = 100%

uint8_t fan_power_percentage = 0;
uint8_t solder_power_percentage = 0;

bool fan_pin_need_set = false;
ISR(TIMER1_COMPA_vect) {
    timer1_stop();
    if(fan_pin_need_set) {
        FanHeaterPin::Set();
        fan_pin_need_set = false;
        timer1_start(100);
    } else {
        FanHeaterPin::Clear();
    }
}

uint8_t fan_counter = 0;
ISR(INT1_vect) { // fan zero-crossing interrupt
    fan_counter = ON_OFF_DELAY;

    if (fan_power_percentage == 0) {
        FanHeaterPin::Clear();
        return;
    }
    // Phase-fired control (PFC), also called phase cutting or "phase angle control"
    fan_pin_need_set = true;
    timer1_start(PFC_delay[fan_power_percentage]);
}

uint8_t solder_counter = 0;
ISR(INT0_vect) { // solder zero-crossing interrupt
    solder_counter = ON_OFF_DELAY;

    // Bresenham's line algorithm + Pulse skipping modulation (PSM)
    static int8_t error = 0;
    error -= solder_power_percentage;
    if(error < 0) {
        error += POWER_STEPS;
        SolderHeaterPin::Set();
        Scheduler::setTimer([]{ SolderHeaterPin::Clear(); }, 2);
    }
}

ISR(TIMER0_OVF_vect) {
    TCNT0 += 255 - (125 - 1); // 8 000 000 / 64 / 125 - 1 = 1 ms
    Scheduler::TimerISR();
    Lcd::draw();
}

const uint8_t ADC_ACCUM_SIZE = 52; // fanPwmPeriodTicks 256 / AdcDivider 64 * AdcConversionTime 13 ticks = 52
uint16_t adc_accum = 0;

ISR(ADC_vect) {
    static uint8_t adc_count = 0;

    adc_accum += ADC;
    if(++adc_count <  ADC_ACCUM_SIZE) return;

    ADCSRA &= ~((1 << ADFR) | (1 << ADIE)); // stop
    adc_count = 0; 
}

bool fanSwitchOn = false;
bool solderSwitchOn = false;

void updateSwitches() {
    fanSwitchOn = fan_counter > 0;
    if(fanSwitchOn) {
        fan_counter--;
    }

    solderSwitchOn = solder_counter > 0;
    if(solderSwitchOn) {
        solder_counter--;
    }
}

bool Peripherals::isFanSwitchOn() {
    return fanSwitchOn;
}

bool Peripherals::isSolderSwitchOn() {
    return solderSwitchOn;
}

bool Peripherals::isFanOnSeat() {
    return !FanSeatSwitchPin::IsSet();
}

bool Peripherals::isFanSensorOk() {
    return getFanAdc() != 1023;
}

bool Peripherals::isSolderSensorOk() {
    return getSolderAdc() != 1023;
}

void Peripherals::setFanPower(uint8_t power_percentage) {
    fan_power_percentage = power_percentage;
    if(fan_power_percentage == 0) {
        FanHeaterPin::Clear();
    }
}

void Peripherals::setSolderPower(uint8_t power_percentage) {
    solder_power_percentage = power_percentage;
    if(solder_power_percentage == 0) {
        SolderHeaterPin::Clear();
    }
}

void Peripherals::setAirFlowVelocity(uint8_t velocity) {
    OCR2 = 0xff - velocity;
}

uint16_t Peripherals::getAirFlowAjustment() {
    Adc::SetChannel(FAN_AIR_ADC_CH);
    return Adc::SingleConversion();
}

uint16_t Peripherals::getSolderAdc() {
    Adc::SetChannel(SOLDER_TEMP_ADC_CH);
    return Adc::SingleConversion();
}

uint16_t Peripherals::getFanAdc() {
    Adc::SetChannel(FAN_TEMP_ADC_CH);
    return Adc::SingleConversion();
}

uint16_t getAverageAdc(uint8_t channel) {
    Adc::SetChannel(channel);
    Adc::EnableInterrupt();
    adc_accum = 0;
    Adc::StartContinuousConversions();
    while(ADCSRA & (1 << ADFR));
    return adc_accum / ADC_ACCUM_SIZE;
}

uint16_t Peripherals::getSolderAverageAdc() {
    return getAverageAdc(SOLDER_TEMP_ADC_CH);
}

uint16_t Peripherals::getFanAverageAdc() {
    return getAverageAdc(FAN_TEMP_ADC_CH);
}

uint16_t Peripherals::getSolderTemp() {
    return Calibrator::convertSolderTemp(getSolderAverageAdc());
}

uint16_t Peripherals::getFanTemp() {
    return Calibrator::convertFanTemp(getFanAverageAdc());
}

Button Peripherals::getButton() {
    Adc::SetChannel(BUTTONS_ADC_CH);
    uint16_t value = Adc::SingleConversion();

    if(value < 500) {
        return Button::UP;
    }
    
    if (value < 700) {
        return Button::DOWN;
    }
    
    if (value < 900) {
        return Button::SET;
    }

    return Button::NONE;
}

void Peripherals::init() {
    Portd::Set(0); // turn Off the Pull-up
    Portd::DirWrite(0b11110011); // PD2, PD3 (INT0, INT1 pin) is now an input, other output

    MCUCR |= (1 << ISC11) | (1 << ISC10) | // The rising edge of INT1 generates an interrupt request
             (1 << ISC01) | (1 << ISC00);  // The rising edge of INT0 generates an interrupt request
 
    GICR |= (1 << INT0) | // Turns on INT0
            (1 << INT1);  // Turns on INT1

    Portb::Set(0);
    Portb::DirSet(0xff); // All output

    Pc1::SetDirRead(); // free pin as input
    Pc1::Set(); // turn On the Pull-up

    FanSeatSwitchPin::SetDirRead();
    FanSeatSwitchPin::Set(); // turn On the Pull-up

    FanLedPin::SetDirWrite();
    SolderLedPin::SetDirWrite();

   // AirFlow PWM
    TCCR2 = (1 << WGM21) | (1 << WGM20) | // Fast PWM mode
            (1 << COM21) | (1 << COM20) | // Set OC2 on Compare Match, clear OC2 at BOTTOM, (inverting mode)
           // (1 << CS22) | (1 << CS21) | (0 << CS20); // prescaler 1/256 = 122 Hz
            (0 << CS22) | (0 << CS21) | (1 << CS20); // No prescaling = 31 250 Hz
    OCR2 = 0xff; // zero in inverting mode

    // Scheduler Timer
    TCCR0 = (0 << CS02) | (1 << CS01) | (1 << CS00); // prescaler 1/64
    TIMSK = (1 << TOIE0); // Timer/Counter0 Overflow Interrupt Enable

    // Heater PFC timer
    // Normal port operation, OC1A/OC1B disconnected.
    TCCR1A = (0 << WGM11) | (0 << WGM10);  // CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | // CTC mode
             (0 << CS12) | (0 << CS11) | (0 << CS10); // prescaler off
    TIMSK |= (1 << OCIE1A); //  Output Compare A Match Interrupt Enable
    
    Adc::Init(0, Adc::Div64, Adc::Internal);
    Adc::Enable();
    Scheduler::setTimer(updateSwitches, 10, true);
}
