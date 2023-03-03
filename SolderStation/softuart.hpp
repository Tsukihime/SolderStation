#ifndef SOFTUART_H_
#define SOFTUART_H_

#include <avr/sfr_defs.h>
#include "util/atomic.h"

#define SOFTUART_PORT PORTB
#define SOFTUART_DDR DDRB
#define SOFTUART_PIN PINB5

// uart speed = F_CPU / 16

class Softuart { 
    public:
        static void sendChar(char ch) {
            uint8_t bitcount = 8;
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                asm volatile (
                "CBI %[port], %[pin]" "\n\t" // PORTx &= ~(1 << PINx) // startbit
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"

      "Loop%=:" "SBRS %[ch], 0" "\n\t"       // if (byte & 1)
                "RJMP Else%=" "\n\t"

                "NOP" "\n\t"
                "SBI %[port], %[pin]" "\n\t" // then PORTx |= (1 << PINx)
                "RJMP Endif%=" "\n\t"        // goto Endif:

      "Else%=:" "CBI %[port], %[pin]" "\n\t" // else PORTx &= ~(1 << PINx)
                "NOP" "\n\t"
                "NOP" "\n\t"

     "Endif%=:" "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"

                "LSR %[ch]" "\n\t"           // byte >>= 1
                "SUBI %[bitcount], 1" "\n\t" // bitcount--
                "BRNE Loop%=" "\n\t"         // if(bitcount != 0) goto Loop:

                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "NOP" "\n\t"
                "SBI %[port], %[pin]" "\n\t" // PORTx |= (1 << PINx) // stopbit

                :: [ch] "r" (ch),
                   [bitcount] "r" (bitcount),
                   [port] "I" (_SFR_IO_ADDR(SOFTUART_PORT)),
                   [pin] "I" (SOFTUART_PIN)
                );
            }
            extern void __builtin_avr_delay_cycles(unsigned long);
            __builtin_avr_delay_cycles(16);
        }

        static void sendString(const char* str) {
            while(*str) {
                sendChar(*str++);
            }
        }

        static void init() {
            SOFTUART_DDR |= (1 << SOFTUART_PIN);
            SOFTUART_PORT |= (1 << SOFTUART_PIN);
        }
};

#endif /* SOFTUART_H_ */
