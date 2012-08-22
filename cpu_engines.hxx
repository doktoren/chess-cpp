#ifndef _CPU_ENGINES_
#define _CPU_ENGINES_

#define NUM_SEARCH_VERSIONS 3
#define NUM_EVAL_VERSIONS 3

#include "cpu_communication_module.hxx"

#include "cpu_evaluation_1.hxx"
#include "cpu_evaluation_2.hxx"
#include "cpu_evaluation_3.hxx"
#include "cpu_search_1.hxx"
#include "cpu_search_2.hxx"
#include "cpu_search_3.hxx"

template <class TYPE>
class Gah {
public:
  Gah() : a(42) {}
  TYPE a;
};

template<> class Gah<string>;

Engine *load_cpu(Engine *cpu, int search_version, int evaluation_version, CPU_CommunicationModule *comm);

#define EvalSearch(Eval, Search) class Eval_ ## Eval ## _Search_ ## Search : \
public Eval_ ## Eval , public Search_ ## Search {\
public:\
  Eval_ ## Eval ## _Search_ ## Search (CPU_CommunicationModule *comm) :\
    Engine(comm) {}\
}

EvalSearch(1,1);
EvalSearch(1,2);
EvalSearch(1,3);
EvalSearch(2,1);
EvalSearch(2,2);
EvalSearch(2,3);
EvalSearch(3,1);
EvalSearch(3,2);
EvalSearch(3,3);

#endif
