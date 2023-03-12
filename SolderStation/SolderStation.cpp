// Fuses High:D9, Low:24) // 8MHz + BodEn + BodLevel 4V

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>

extern "C" {
    #include "pid/pid.h"
}

const float K_P = 2.50;
const float K_I = 0.01;
const float K_D = 15.00;

struct PID_DATA fanPidData;

#include "utils.h"
#include "config.h"
#include "Scheduler.h"
#include "AVRPin.hpp"
#include "calibrator.h"
#include "lcd.h"
#include "peripherals.h"

#ifdef SOFTUART
    #include "softuart.hpp"
#endif

enum Mode {SOLDER, FAN, FAN_CALIBRATION, SOLDER_CALIBRATION};
enum FanMode {OFF, SLEEP, COOLING, ON};

Mode mode = Mode::SOLDER;
FanMode fanMode = FanMode::OFF;

uint16_t solderSetupTemp = TEMPERATURE_MIN;
uint16_t fanSetupTemp = TEMPERATURE_MIN;
uint16_t calibratorColdTemp;
uint16_t calibratorFanTemp;
uint16_t calibratorSolderTemp;

uint16_t pwr = 0;

void processFan() {
    static bool pid_init = true;
    static uint8_t cooling_timeout = 0;
    uint16_t currentTemp = Peripherals::getFanTemp();
    bool heaterOn = Peripherals::isFanSwitchOn() && !Peripherals::isFanOnSeat();
    
    // AirFlow
    static bool coolingRequirement = false;
    if(currentTemp > (FAN_THRESHOLD_TEMP + FAN_HYSTERESIS_TEMP)) {
        coolingRequirement = true;
        cooling_timeout = FAN_COOLING_TIMEOUT * 10; // sec * 10 * 100ms per call
    }
    if(currentTemp <= FAN_THRESHOLD_TEMP) {
        if(cooling_timeout == 0) {
            coolingRequirement = false;
        } else {
            cooling_timeout--;
        }
    }
    
    if (heaterOn) {
        fanMode = FanMode::ON;
        uint8_t velocity = map(Peripherals::getAirFlowAjustment(), 0, 1023, FAN_AIR_FLOW_MIN, FAN_AIR_FLOW_MAX);
        Peripherals::setAirFlowVelocity(velocity);
    } else if(coolingRequirement) {
        fanMode = FanMode::COOLING;
        Peripherals::setAirFlowVelocity(FAN_AIR_FLOW_MAX);
    } else {
        Peripherals::setAirFlowVelocity(0);
        fanMode = Peripherals::isFanSwitchOn() ? FanMode::SLEEP : FanMode::OFF;
    }

    // Heat
    if(!heaterOn) {
        Peripherals::setFanPower(0);
        pid_init = true;
        return;
    }

    if (pid_init) {
        pid_init = false;
        pid_Init(K_P * SCALING_FACTOR,  K_I * SCALING_FACTOR, K_D * SCALING_FACTOR, &fanPidData);
    }
    
    int16_t inputValue = pid_Controller(fanSetupTemp, currentTemp, &fanPidData);
    uint8_t power = clamp(inputValue, 0, 100);
    pwr = power;
    Peripherals::setFanPower(power);
}

void processSolder() {
    if(!Peripherals::isSolderSwitchOn()) {
        Peripherals::setSolderPower(0);
        return;
    }

    uint16_t currentTemp = Peripherals::getSolderTemp();        
    if(currentTemp > solderSetupTemp) {
       Peripherals::setSolderPower(0);
    } else {
       Peripherals::setSolderPower(100);
    }
}

uint16_t lastChangeTimeout = 0;
uint8_t recentlyChangedTimeout = 0;

bool isChangeMode() {
    return lastChangeTimeout != 0;
}

bool isRecentlyChanged() {
    return recentlyChangedTimeout != 0;
}

void changeModeOn() {
    lastChangeTimeout = CHANGE_MODE_DELAY;
    recentlyChangedTimeout = 2;
}

void processTimeouts() {
    if (lastChangeTimeout != 0) {
        lastChangeTimeout--;
    }

    if (recentlyChangedTimeout != 0) {
        recentlyChangedTimeout--;
    }
}

void processSwitches() {
    static bool fanSwitchOld = false;
    static bool solderSwitchOld = false;
    static bool fanSeat = false;

    bool fan_changed = Peripherals::isFanSwitchOn() != fanSwitchOld ||
                       Peripherals::isFanOnSeat() != fanSeat;

    bool solder_changed = Peripherals::isSolderSwitchOn() != solderSwitchOld;

    if(fan_changed && Peripherals::isFanSwitchOn()) {
        mode = Mode::FAN;
    }

    if(solder_changed && Peripherals::isSolderSwitchOn()) {
        mode = Mode::SOLDER;
    }

    fanSwitchOld = Peripherals::isFanSwitchOn();
    solderSwitchOld = Peripherals::isSolderSwitchOn();
    fanSeat = Peripherals::isFanOnSeat();
}

uint16_t& getCurrentModeValue() {
    static uint16_t none;
    switch(mode) {
        case FAN:
            return fanSetupTemp;

        case SOLDER:
            return solderSetupTemp;

        case FAN_CALIBRATION:
            return calibratorFanTemp;

        case SOLDER_CALIBRATION:
            return calibratorSolderTemp;

        default: 
            return none;
    }
}

void changeButtonsClick(bool isUp) {
    changeModeOn();
    int8_t delta = isUp ? +1 : -1;
    uint16_t &value = getCurrentModeValue();
    value = clamp(value + delta, TEMPERATURE_MIN, TEMPERATURE_MAX);
}

void buttonSetClick() {
    switch(mode) {
        case SOLDER_CALIBRATION:
            if(calibratorSolderTemp >= HOT_CALIBRATION_TEMP) {
                Calibrator::setHotSolderCalibration(calibratorSolderTemp);
            }

            if(calibratorSolderTemp <= COLD_CALIBRATION_TEMP) {
                Calibrator::setColdSolderCalibration(calibratorSolderTemp);
            }

            mode = Mode::SOLDER;
        break;

        case FAN_CALIBRATION:
            if(calibratorFanTemp >= HOT_CALIBRATION_TEMP) {
                Calibrator::setHotFanCalibration(calibratorFanTemp);
            }

            if(calibratorFanTemp <= COLD_CALIBRATION_TEMP) {
                Calibrator::setColdFanCalibration(calibratorFanTemp);
            }

            mode = Mode::FAN;
        break;

        case FAN:
            mode = Mode::SOLDER;
        break;

        case SOLDER:
            mode = Mode::FAN;
        break;

        default: ;
    }
}

void buttonSetHold() {
    switch(mode) {
        case FAN:
            mode = Mode::FAN_CALIBRATION;
            calibratorFanTemp = Peripherals::getFanTemp();
        break;

        case SOLDER:
            mode = Mode::SOLDER_CALIBRATION;
            calibratorSolderTemp = Peripherals::getSolderTemp();
        break;

        default: ;
    }
}

void processButtons() {
    static Button oldButton;
    static bool firstRun = true;
    static uint8_t delay = 0;
    static bool oldHold = false;
    Button button = Peripherals::getButton();

    if (firstRun) { // eliminates a click when turned on with the button held down
        oldButton = button;
        firstRun = false;
    }
    
    bool btnDown = button != oldButton;
    if(btnDown) {
        delay = 0;
    }

    bool btnHold = delay >= LONG_PRESS_DELAY;
    bool btnFirstHold = !oldHold && btnHold;
    if (!btnDown && !btnHold) {
        delay++;
    }

    if((button == UP || button == DOWN) && (btnDown || btnHold)) {
        changeButtonsClick(button == UP);
    }

    bool btnSetUp = (oldButton == SET) && btnDown && !oldHold;
    if(btnSetUp) {
        buttonSetClick();
    }

    if((button == SET) && btnFirstHold) {
        buttonSetHold();
    }
    
    oldHold = btnHold;
    oldButton = button;
}

void processLEDs() {
    FanLedPin::Set(mode == Mode::FAN ||
                   mode == Mode::FAN_CALIBRATION
    );

    SolderLedPin::Set(mode == Mode::SOLDER ||
                      mode == Mode::SOLDER_CALIBRATION
    );

    Lcd::setBlink(false);

    bool changeMode = isChangeMode() ||
                      mode == Mode::FAN_CALIBRATION ||
                      mode == Mode::SOLDER_CALIBRATION;

    if(mode == Mode::FAN && !changeMode) {
        switch(fanMode) {
            case COOLING:
                Lcd::setOFF();
                Lcd::setBlink();
            break;

            case SLEEP:
                Lcd::setSleep();
            break;

            case OFF:
                Lcd::setDashes();
                Lcd::setBlink();
            break;

            case ON:
                if(Peripherals::isFanSensorOk()) {
                    Lcd::setValue(Peripherals::getFanTemp());
                } else {
                    Lcd::setSensorError();
                    Lcd::setBlink();
                }
        }
    }
    
    if (mode == Mode::SOLDER && !changeMode) {
        if(Peripherals::isSolderSwitchOn()) {
            if(Peripherals::isSolderSensorOk()) {
                Lcd::setValue(Peripherals::getSolderTemp());
            } else {
                Lcd::setSensorError();
                Lcd::setBlink();
            }
        } else {
            Lcd::setDashes();
            Lcd::setBlink();
        }
    }

    if(changeMode) {
        uint16_t &value = getCurrentModeValue();
        Lcd::setValue(value);
        Lcd::setBlink(!isRecentlyChanged());
    }
}

void saveSettings() {
    static bool oldChange = false;
    if(!isChangeMode() && oldChange) {
        uint16_t fanSaved, solderSaved;
        Calibrator::getSetupTemp(fanSaved, solderSaved);

        if((fanSetupTemp != fanSaved) || (solderSetupTemp != solderSaved)) {
            Calibrator::setSetupTemp(fanSetupTemp, solderSetupTemp);
        }
    }
    oldChange = isChangeMode();
}

#ifdef SOFTUART
void printValue(uint16_t val) {
    uint8_t buffer[5];
    bin2bcd5(val, buffer);
    bcd2ascii(buffer);
    Softuart::sendChar(buffer[2]);
    Softuart::sendChar(buffer[3]);
    Softuart::sendChar(buffer[4]);
}

void printDbg(uint16_t a, uint16_t b, uint16_t c) {
    printValue(a);
    Softuart::sendChar(',');
    printValue(b);
    Softuart::sendChar(',');
    printValue(c);
    Softuart::sendString("\r\n");
}
#endif

void loop100ms() {
    wdt_reset();
    processFan();
    processSolder();
    processSwitches();
    processLEDs();
    saveSettings();
#ifdef SOFTUART
    printDbg(fanSetupTemp, Peripherals::getFanTemp(), pwr * 2);
#endif
}

void loop10ms() {
    processButtons();
    processTimeouts();
}

int main(void) {
    wdt_enable(WDTO_120MS);
    Peripherals::init();
    Calibrator::init();
    Calibrator::getSetupTemp(fanSetupTemp, solderSetupTemp);
    wdt_reset(); // Calibrator::init(); take long time ~80ms
    sei();

#ifdef SOFTUART
    Softuart::init();
#endif

    Scheduler::setTimer(loop10ms, 10, true);
    Scheduler::setTimer(loop100ms, 100, true);
    Scheduler::run();
}
