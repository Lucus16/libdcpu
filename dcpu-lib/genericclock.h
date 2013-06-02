#ifndef GENERICCLOCK_H_INCLUDED
#define GENERICCLOCK_H_INCLUDED

typedef struct Clock {
    word interruptMessage;
    word ticksSinceLast;
    int cyclesPerTick;
    Event* currentEvent;
} Clock;

Device* newClock();
void clockHandler(Device* device);
void clockTick(void* data);
void clockReset(Device* device);

#endif // GENERICCLOCK_H_INCLUDED
