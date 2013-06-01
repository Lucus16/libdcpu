#include "event.h"

Event* addEvent(Event* eventchain, eventtime time, void (*ontrigger)(void*), void* data) {
    Event* event = malloc(sizeof(event));
    if (event == NULL) {
        return NULL; //error: out of memory
    }
    event->ontrigger = ontrigger;
    event->data = data;
    event->time = time;
    Event* ne = eventchain;
    while (true) {
        if (ne->nextevent == NULL || ne->nextevent->time >= time) {
            event->nextevent = ne->nextevent;
            ne->nextevent = event;
            return event;
        }
        ne = ne->nextevent;
    }
    return event;
}

int removeEvent(Event* eventchain, Event* event) {
    Event* ne = eventchain;
    while (ne != NULL) {
        if (ne->nextevent == event) {
            ne->nextevent = event->nextevent;
            free(event);
            return 0;
        }
        ne = ne->nextevent;
    }
    return 1;
}

void destroyEvents(Event* eventchain) {
    Event* ne = eventchain;
    Event* del;
    while (ne != NULL) {
        del = ne;
        ne = ne->nextevent;
        free(del);
    }
}

int runEvents(Event* eventchain, eventtime time) {
    Event* ne = eventchain;
    Event* del;
    while (ne->time <= time) {
        ne->trigger(ne->data);
        del = ne
        ne = ne->nextevent;
        free(del);
    }
}
