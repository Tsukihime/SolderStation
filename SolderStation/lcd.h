#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>

class Lcd {
    public:
        static void draw();
        static void setValue(uint16_t value);
        static void setOFF();
        static void setSleep();
        static void setSensorError();
        static void setDashes();
        static void setBlink(bool doBlink = true);
};

#endif /* LCD_H_ */
