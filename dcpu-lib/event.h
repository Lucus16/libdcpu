#ifndef EVENT_H_INCLUDED
#define EVENT_H_INCLUDED

typedef int64_t eventtime;

typedef struct Event {
    void (*ontrigger)(void*);
    void* data;
    eventtime time;
    Event* nextevent;
} Event;

//Add a new event to an eventchain
//Returns a pointer to the added event
Event* addEvent(Event* eventchain, eventtime time, void (*ontrigger)(void*), void* data);

//Remove an event from the eventchain and free it
//Returns an integer indicating success or failure
int removeEvent(Event* eventchain, Event* event);

//Destroy an eventchain by freeing all events
void destroyEvents(Event* eventchain);

//Run all events up to a certain time
//Returns the number of events run
int runEvents(Event* eventchain, eventtime time);

#endif // EVENT_H_INCLUDED
