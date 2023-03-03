#include "calibrator.h"
#include "peripherals.h"
#include "config.h"

Calibrator::CalibrationData Calibrator::data;
Calibrator::CalibrationData EEMEM Calibrator::eepromDataAddr;
const uint16_t MAGIC = 0xC0DE;

uint16_t remap(uint16_t value, uint16_t oldMin, uint16_t oldMax, uint16_t newMin, uint16_t newMax) {
    uint16_t oldRange = oldMax - oldMin;
    uint16_t newRange = newMax - newMin;
    int16_t result = ((static_cast<int32_t>(value) - oldMin) * newRange) / oldRange + newMin;

    return (result >= 0) ? result : 0;
}

void Calibrator::init() {
    eeprom_read_block(&data, &eepromDataAddr, sizeof(data));
    if(data.magic != MAGIC) { // set defaults
        data.magic = MAGIC;
        data.fan.coldTemp = 20;
        data.fan.coldAdc = 109;
        data.fan.hotTemp = 566;
        data.fan.hotAdc = 1023;
        data.solder.coldTemp = 20;
        data.solder.coldAdc = 109;
        data.solder.hotAdc = 1023;
        data.solder.hotTemp = 566;

        data.fan.setupTemp = TEMPERATURE_MIN;
        data.solder.setupTemp = TEMPERATURE_MIN;
        save();
    }
}

void Calibrator::save() {
    eeprom_update_block(&data, &eepromDataAddr, sizeof(data));
}

void Calibrator::setColdFanCalibration(uint16_t temp) {
    data.fan.coldTemp = temp;
    data.fan.coldAdc = Peripherals::getFanAdc();
    save();
}

void Calibrator::setFanCalibration(uint16_t temp) {
    data.fan.hotAdc = Peripherals::getFanAdc();
    data.fan.hotTemp = temp;
    save();
}

uint16_t Calibrator::getColdFanCalibrationTemp() {
    return data.fan.coldTemp;
}

void Calibrator::setColdSolderCalibration(uint16_t temp) {
    data.solder.coldTemp = temp;
    data.solder.coldAdc = Peripherals::getSolderAdc();
    save();
}

void Calibrator::setSolderCalibration(uint16_t temp) {
    data.solder.hotAdc = Peripherals::getSolderAdc();
    data.solder.hotTemp = temp;
    save();
}

uint16_t Calibrator::getColdSolderCalibrationTemp() {
    return data.solder.coldTemp;
}

uint16_t Calibrator::convertSolderTemp(uint16_t adcValue) {
    return remap(adcValue, data.solder.coldAdc, data.solder.hotAdc, data.solder.coldTemp, data.solder.hotTemp);
}

uint16_t Calibrator::convertFanTemp(uint16_t adcValue) {
    return remap(adcValue, data.fan.coldAdc, data.fan.hotAdc, data.fan.coldTemp, data.fan.hotTemp);
}

void Calibrator::getSetupTemp(uint16_t &fanTemp, uint16_t &solderTemp) {
    fanTemp = data.fan.setupTemp;
    solderTemp = data.solder.setupTemp;
}

void Calibrator::setSetupTemp(uint16_t fanTemp, uint16_t solderTemp) {
    data.fan.setupTemp = fanTemp;
    data.solder.setupTemp = solderTemp;
    save();
}
