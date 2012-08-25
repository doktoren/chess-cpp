#include "cpu_communication_module.hxx"

#include <assert.h>
#include <string.h>

#include "../streams.hxx"

void CPU_CommunicationModule::print(ostream& os) {
  os << "Cpu communication module:\n"
     << "  - cpu_is_thinking = " << cpu_is_thinking << '\n'
     << "  - cpu_color = " << cpu_color << '\n'
     << "  - ping_respond = " << ping_respond << '\n'
     << "  - move_now = " << move_now << '\n';
#ifdef XBOARD
  os << "  - use_fixed_depth = " << use_fixed_depth << '\n'
     << "  - fixed_time_per_move = " << fixed_time_per_move << '\n'
     << "  - seconds_per_move = " << seconds_per_move << '\n'
     << "  - setting_level_used = " << setting_level_used << '\n'
     << "  - mps, base, inc = " << mps << ' ' << base << ' ' << inc << '\n'
     << "  - cpu_time_in_centiseconds = " << cpu_time_in_centiseconds << '\n';
#endif
  os << "END\n";
}


void* CPU_CommunicationModule::operator new(size_t size) {
  return get_shared_mem(size);
}

void CPU_CommunicationModule::operator delete(void *p, size_t size) {
  cerr << "Calling CPU_CommunicationModule::operator delete.\n";
}


//##################################


MessageQueue::MessageQueue() {
  first = last = 0;
  mem = (char *)get_shared_mem(MAX_UNREAD_MESSAGES*MAX_MESSAGE_SIZE);
}

bool MessageQueue::empty() {
  return first == last;
}

void MessageQueue::pop(char* destination) {
  if (empty()) {
    cerr << "Error: MessageQueue::pop(), but empty!\n";
    assert(0);
  }
  strcpy(destination, &(mem[first * MAX_MESSAGE_SIZE]));
  first = (first+1) % MAX_UNREAD_MESSAGES;
}

bool MessageQueue::push(const char *message) {
  bool too_long = strlen(message) >= MAX_MESSAGE_SIZE;
  if (too_long) {// == not allowed because of terminator
    cerr << "The message below is too long. It is reduced to a \"#\"\n"
	 << message << "\n";
  }

  if ((last+1)%MAX_UNREAD_MESSAGES == first) {
    cerr << "Error: MessageQueue filled: first = " << first
	 << ", last = " << last << ", MAX_UNREAD_MESSAGES = " << MAX_UNREAD_MESSAGES << "\n";
    exit(1);
  }

  //cerr << "pushing " << message << "\n";

  if (too_long) {  
    mem[last * MAX_MESSAGE_SIZE] = '#';
    mem[last * MAX_MESSAGE_SIZE + 1] = 0;
  } else {
    mem[last * MAX_MESSAGE_SIZE] = 0; // whatever
    strcpy(&(mem[last * MAX_MESSAGE_SIZE]), message);
  }

  last = (last+1) % MAX_UNREAD_MESSAGES;

  //cerr << "pushing " << top() << "\n";
  return true;
}

void* MessageQueue::operator new(size_t size) {
  return get_shared_mem(size);
}

void MessageQueue::operator delete(void *p, size_t size) {
  cerr << "Calling MessageQueue::operator delete.\n";
}
