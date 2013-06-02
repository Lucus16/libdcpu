#include "dcpu.h"
#include "genericclock.h"

Device* newClock() {
    Device* dev = malloc(sizeof(Device));
    dev->data = malloc(sizeof(Clock));
    dev->super.ID = 0x12d0b402ui;
    dev->interruptHandler = clockHandler;
    dev->reset = clockReset;
    dev->destroyData = NULL;
    clockReset(dev);
    return dev;
}

void clockHandler(Device* device) {
    Clock* clock = device->data;
    int prev;
    switch (device->dcpu->regA) {
        case 0:
            prev = clock->cyclesPerTick;
            clock->cyclesPerTick = 100000ll * device->dcpu->regB / 60; //TODO: Check if this works on 32-bit computers
            if (prev == 0 && clock->cyclesPerTick != 0) {
                clock->currentEvent = addEvent(device->dcpu->eventchain, clock->cyclesPerTick, clockTick, device);
            } else if (prev != 0 && clock->cyclesPerTick == 0) {
                removeEvent(device->dcpu->eventchain, clock->currentEvent);
                clock->currentEvent = NULL;
            }
            break;
        case 1:
            device->dcpu->regC = clock->ticksSinceLast;
            clock->ticksSinceLast = 0;
            break;
        case 2:
            clock->interruptMessage = device->dcpu->regB;
            break;
    }
}

void clockTick(void* data) {
    Device* device = data;
    Clock* clock = device->data;
    if (clock->cyclesPerTick != 0) {
        clock->ticksSinceLast++;
        clock->currentEvent = addEvent(device->dcpu->eventchain, clock->cyclesPerTick, clockTick, device);
        if (clock->interruptMessage != 0) {
            addInterrupt(device->dcpu, clock->interruptMessage);
        }
    }
}

void clockReset(Device* device) {
    Clock* clock = device->data;
    clock->cyclesPerTick = 0;
    clock->interruptMessage = 0;
    clock->ticksSinceLast = 0;
    if (device->dcpu != NULL) {
        removeEvent(device->dcpu->eventchain, clock->currentEvent);
    }
    clock->currentEvent = NULL;
}
