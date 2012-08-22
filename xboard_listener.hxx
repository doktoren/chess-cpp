#ifndef _XBOARD_LISTENER_
#define _XBOARD_LISTENER_

#include <iostream>
#include <string>

#include "typedefs.hxx"
#include "streams.hxx"

// shared memory
extern void *shared_mem;
extern void init_shared_mem(int size);
extern void* get_shared_mem(int size);

class MessageQueue;
class CPU_CommunicationModule;

extern MessageQueue *mq;
extern CPU_CommunicationModule *comm;

// the thread that returns false must be killed
bool spawn_listener();
void detach_shared_memory();

void im_parant__kill_me();

#endif
