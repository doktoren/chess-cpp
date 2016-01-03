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

class Eval_1_Search_1 : public Eval_1, public Search_1 { public: Eval_1_Search_1(CPU_CommunicationModule *comm): Engine(comm) {}};
class Eval_1_Search_2 : public Eval_1, public Search_2 { public: Eval_1_Search_2(CPU_CommunicationModule *comm): Engine(comm) {}};
class Eval_1_Search_3 : public Eval_1, public Search_3 { public: Eval_1_Search_3(CPU_CommunicationModule *comm): Engine(comm), Search_3(comm) {}};
class Eval_2_Search_1 : public Eval_2, public Search_1 { public: Eval_2_Search_1(CPU_CommunicationModule *comm): Engine(comm) {}};
class Eval_2_Search_2 : public Eval_2, public Search_2 { public: Eval_2_Search_2(CPU_CommunicationModule *comm): Engine(comm) {}};
class Eval_2_Search_3 : public Eval_2, public Search_3 { public: Eval_2_Search_3(CPU_CommunicationModule *comm): Engine(comm), Search_3(comm) {}};
class Eval_3_Search_1 : public Eval_3, public Search_1 { public: Eval_3_Search_1(CPU_CommunicationModule *comm): Engine(comm), Eval_3(comm) {}};
class Eval_3_Search_2 : public Eval_3, public Search_2 { public: Eval_3_Search_2(CPU_CommunicationModule *comm): Engine(comm), Eval_3(comm) {}};
class Eval_3_Search_3 : public Eval_3, public Search_3 { public: Eval_3_Search_3(CPU_CommunicationModule *comm): Engine(comm), Eval_3(comm), Search_3(comm) {}};

#endif
