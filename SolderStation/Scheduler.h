#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

const uint8_t MAX_TIMERS_COUNT = 8;
const uint8_t MAX_TASK_QUEUE_SIZE = 8;

typedef void(*TaskPointer)();

class Scheduler {
    private:
        typedef struct {
            uint16_t counter;
            uint16_t period;
            TaskPointer task;
        } Timer;

        static Timer timers[MAX_TIMERS_COUNT];
        static uint8_t timersCount;
        static TaskPointer taskQueue[MAX_TASK_QUEUE_SIZE];
        static uint8_t taskQueueSize;

    public:
        static void processTasks();
        static void run();
        static bool setTimer(TaskPointer task, uint16_t period_ticks, bool periodic = false);
        static bool setTask(TaskPointer task);

        static inline void TimerISR() {
            for(uint8_t i = 0; i < timersCount; i++) {
                timers[i].counter--;
                if(timers[i].counter != 0) {                    
                    continue;
                }

                setTask(timers[i].task);
                
                if(timers[i].period != 0) { // reset timer
                    timers[i].counter = timers[i].period;
                } else {                    // delete timer
                    timersCount--;
                    timers[i] = timers[timersCount];
                    i--;
                }
            }
        }
};

#endif /* SCHEDULER_H_ */
