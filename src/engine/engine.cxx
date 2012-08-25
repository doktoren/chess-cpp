#include "engine.hxx"

Engine::Engine(CPU_CommunicationModule *comm) :
  comm(comm)
// : Board2(true) won't work
{
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Engine constructor called. (comm = " << comm << ")\n";
}

Engine::~Engine() {
  if (PRINT_CONSTRUCTOR_DESTRUCTOR_CALLS)
    cerr << "Engine destructor called.\n";
}
