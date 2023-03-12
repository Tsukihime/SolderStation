#ifndef CALIBRATOR_H_
#define CALIBRATOR_H_

#include <avr/eeprom.h>

class Calibrator {
    private:
        typedef struct {
            uint16_t magic;
            struct {
                uint16_t coldAdc;
                uint16_t coldTemp;
                uint16_t hotAdc;
                uint16_t hotTemp;
                uint16_t setupTemp;
            } fan;
            struct {
                uint16_t coldAdc;
                uint16_t coldTemp;
                uint16_t hotAdc;
                uint16_t hotTemp;
                uint16_t setupTemp;
            } solder;
        } CalibrationData;

        static CalibrationData data;
        static CalibrationData EEMEM eepromDataAddr;
        static void save();
        
    public:
        static void init();
        static uint16_t convertSolderTemp(uint16_t adcValue);
        static uint16_t convertFanTemp(uint16_t adcValue);

        static void setColdFanCalibration(uint16_t temp);
        static void setHotFanCalibration(uint16_t temp);
        static uint16_t getColdFanCalibrationTemp();

        static void setColdSolderCalibration(uint16_t temp);
        static void setHotSolderCalibration(uint16_t temp);
        static uint16_t getColdSolderCalibrationTemp();

        static void getSetupTemp(uint16_t &fanTemp, uint16_t &solderTemp);
        static void setSetupTemp(uint16_t fanTemp, uint16_t solderTemp);
};

#endif /* CALIBRATOR_H_ */
