#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

enum Button { NONE = 0, UP = 1, DOWN = 2, SET = 3 };

class Peripherals {
    public:
        static void init();
        static bool isFanSwitchOn();
        static bool isSolderSwitchOn();
        static bool isFanOnSeat();
        static bool isFanSensorOk();
        static bool isSolderSensorOk();
        static void setAirFlowVelocity(uint8_t velocity);
        static void setFanPower(uint8_t power_percentage);
        static void setSolderPower(uint8_t power_percentage);
        static uint16_t getAirFlowAjustment();
        static uint16_t getSolderAdc();
        static uint16_t getFanAdc();
        static uint16_t getSolderTemp();
        static uint16_t getFanTemp();
        static Button getButton();
};

#endif /* PERIPHERALS_H_ */
