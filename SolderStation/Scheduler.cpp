#include "Scheduler.h"

#include <util/atomic.h>

Scheduler::Timer Scheduler::timers[MAX_TIMERS_COUNT];
uint8_t Scheduler::timersCount = 0;
TaskPointer Scheduler::taskQueue[MAX_TASK_QUEUE_SIZE];
uint8_t Scheduler::taskQueueSize = 0;

void Scheduler::processTasks() {
    TaskPointer currentTask;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if(taskQueueSize == 0) {
            return; // Idle();
        }

        currentTask = taskQueue[0];
        taskQueueSize--;
        for(uint8_t i = 0; i < taskQueueSize; i++) {
            taskQueue[i] = taskQueue[i + 1];
        }
    }
    currentTask();
}

void Scheduler::run() {
    //set_sleep_mode(SLEEP_MODE_IDLE);
    while(true) {
        processTasks();
        //sleep_mode();                    // Enter Sleep Mode To Save Power CPU Will Wake Up From Timer2 Interrupt
    }
}

bool Scheduler::setTimer(TaskPointer task, uint16_t period_ticks, bool periodic) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (timersCount == MAX_TIMERS_COUNT) {
            return false;
        }

        timers[timersCount].task = task;
        timers[timersCount].counter = period_ticks;
        timers[timersCount].period = periodic ? period_ticks : 0;
        timersCount++;
    }
    return true;
}

bool Scheduler::setTask(TaskPointer task) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if(taskQueueSize == MAX_TASK_QUEUE_SIZE) {
            return false;
        }

        taskQueue[taskQueueSize] = task;
        taskQueueSize++;
    }
    return true;
}
