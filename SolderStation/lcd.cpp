#include "lcd.h"
#include "config.h"
#include "utils.h"

static const uint8_t didgits[] = {
  //__fagcbhde
    0b11011011, // 0
    0b00011000, // 1
    0b01101011, // 2
    0b01111010, // 3
    0b10111000, // 4
    0b11110010, // 5
    0b11110011, // 6
    0b01011000, // 7
    0b11111011, // 8
    0b11111010, // 9
};

enum Chars {
       // fagcbhde       
    A = 0b11111001, // A
    B = 0b10110011, // B
    C = 0b11000011, // C
    D = 0b00111011, // D
    E = 0b11100011, // E
    F = 0b11100001, // F
    L = 0b10000011, // L
    P = 0b11101001, // P
    Dash =  0b00100000, // -
    Dot =   0b00000100, // dot
    Space = 0b00000000  // [ ]
};

uint8_t chars[3];
bool blink = false;

void Lcd::draw() {
    LCDAnode1Pin::Clear();
    LCDAnode2Pin::Clear();
    LCDAnode3Pin::Clear();

    static uint16_t delay;
    if(delay >= LCD_BLINK_DELAY * 2 || !blink) {
        delay = 0;
    }
    delay++;

    if(delay > LCD_BLINK_DELAY) return; // turn off lcd

    static uint8_t charIndex = 0;
    char heat_led = ((FanHeaterPin::IsSet() | SolderHeaterPin::IsSet()) && charIndex == 2)
                    ? Chars::Dot
                    : Chars::Space;

    uint8_t ch = ~(chars[charIndex] | heat_led);

    #ifdef SOFTUART
        ch |= (PORTB & (1 << PORTB5)); // block pb5 pin change because its used by soft uart
    #endif

    /*
    Ignore the pb3 writing because:
    The general I/O port function is overridden by the Output Compare (OC2) from the waveform generator if
    either of the COM21:0 bits are set.
    */
    Portb::Write(ch); 
    LCDBPin::Set(ch & (1 << 3)); // set B pin if corresponding bit is set
    
    switch(charIndex) {
        case 0: LCDAnode1Pin::Set();
        break;
        case 1: LCDAnode2Pin::Set();
        break;
        case 2: LCDAnode3Pin::Set();
        break;
    }

    if(++charIndex > 2) {
        charIndex = 0;
    }
}

void Lcd::setOFF() {
    chars[0] = didgits[0];
    chars[1] = Chars::F;
    chars[2] = Chars::F;
}

void Lcd::setSleep() {
    chars[0] = didgits[5];
    chars[1] = Chars::L;
    chars[2] = Chars::P;
}

void Lcd::setSensorError() {
    chars[0] = didgits[5];
    chars[1] = Chars::Dash;
    chars[2] = Chars::E;
}

void Lcd::setDashes() {
    chars[0] = Chars::Dash;
    chars[1] = Chars::Dash;
    chars[2] = Chars::Dash;
}

void Lcd::setValue(uint16_t value) {
    uint8_t buffer[5];
    bin2bcd5(value, buffer);

    chars[0] = didgits[buffer[2]];
    chars[1] = didgits[buffer[3]];
    chars[2] = didgits[buffer[4]];
}

void Lcd::setBlink(bool doBlink) {
    blink = doBlink;
}
