#include "event.h"

Event* newChain() {
    Event* event = malloc(sizeof(event));
    if (event == NULL) {
        return NULL;
    }
    event->ontrigger = NULL;
    event->data = NULL;
    event->time = 0;
    event->nextevent = NULL;
    return event;
}

Event* addEvent(Event* eventchain, eventtime time, void (*ontrigger)(void*), void* data) {
    Event* event = malloc(sizeof(event));
    if (event == NULL) {
        return NULL; //error: out of memory
    }
    event->ontrigger = ontrigger;
    event->data = data;
    event->time = time + eventchain->time;
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

void destroyChain(Event* eventchain) {
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
    int total = 0;
    while (ne->time <= time) {
        total++;
        ne->ontrigger(ne->data);
        del = ne;
        ne = ne->nextevent;
        free(del);
    }
    eventchain->time = time;
    return total;
}
