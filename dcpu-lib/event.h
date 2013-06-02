#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

#include "main.h"

typedef int64_t eventtime;
typedef struct Event Event;

typedef struct Event {
    void (*ontrigger)(void*);
    void* data;
    eventtime time;
    Event* nextevent;
} Event;

/* The first event in the eventchain is a dummy event.
 * ontrigger points to NULL and time is the current time.
 */

//Create a new eventchain
//Returns a pointer to the new eventchain
Event* newChain();

//Add a new event to an eventchain, time units after the current time.
//Returns a pointer to the added event, returns NULL if out of memory
Event* addEvent(Event* eventchain, eventtime time, void (*ontrigger)(void*), void* data);

//Remove an event from the eventchain and free it
//Returns an integer indicating success or failure
int removeEvent(Event* eventchain, Event* event);

//Destroy an eventchain by freeing all events
void destroyChain(Event* eventchain);

//Run all events up to a certain time
//Returns the number of events run
int runEvents(Event* eventchain, eventtime time);

//Count the number of events in the eventchain
//Returns the number of events
int countEvents(Event* eventchain);

#endif // EVENT_H_INCLUDED
