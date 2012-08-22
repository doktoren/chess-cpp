#include <iostream>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>

#include "xboard_listener.hxx"
#include "cpu_communication_module.hxx"

// Specified in .hxx:
//BEGIN
void *shared_mem;
void init_shared_mem(int size);
void* get_shared_mem(int size);

MessageQueue *mq;
CPU_CommunicationModule *comm;
//END

pid_t child_thread;
int shmid;


void init_shared_mem(int size) {
  // Create the shared memory segment
  shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
  if (shmid == -1) {
    cerr << "Could not create shared memory ID\n";
    exit(1);
  }
  
  // Attach shared memory
  shared_mem = shmat(shmid, 0, 0);
  if (shared_mem == (char *)-1) {
    cerr << "Child (engine) could not attach shared memory pointer.\n";
    exit(1);
  }

  *((int *)shared_mem) = (int)shared_mem + 8;
  ((int *)shared_mem)[1] = size - 8;
}

void* get_shared_mem(int size) {
  assert(size <= ((int *)shared_mem)[1]);

  char *result = (char *)(*((int *)shared_mem));
  *((int *)shared_mem) += size;
  ((int *)shared_mem)[1] += size;

  return (void *)result;
}

void detach_shared_memory() {
  if (shmdt(shared_mem) == -1) {
    cerr << "Child (engine) could not detach from shared memory segment.\n";
    cerr << "    (address = " << shared_mem << ")\n";
    cerr << "    (errno = " << errno << ")\n";
    exit(1);
  }
}

void im_parant__kill_me() {
  // Wait for child to die.
  cerr << "Waiting for child to die...\n";
  int _status;
  wait(&_status);


  // detach from shared memory
  if (shmdt(shared_mem) == -1) {
    cerr << "Parent could not detach from shared memory segment.\n";
    cerr << "    (address = " << shared_mem << ")\n";
    cerr << "    (errno = " << errno << ")\n";
    exit(1);
  }

  // destroy shared memory
  int result = shmctl(shmid, IPC_RMID, 0);
  if (result == -1) {
    cerr << "Could not destroy shared memory segment!!!\n";
    exit(1);
  }
}

bool is_interrupt_message(string s);

bool spawn_listener() {
  // This function may only be called once
  static bool not_called_before = true;
  assert(not_called_before);
  not_called_before = false;


  // cin is not proberly set to being unbuffered by this!
  cout.setf(ios::unitbuf);
  //cout.rdbuf()->setbuf(NULL, 0);
  cin.setf(ios::unitbuf);
  //cin.rdbuf()->setbuf(NULL, 0);
  signal(SIGTERM, SIG_IGN);
  //signal(SIGINT, SIG_IGN);

  setbuf(stdin, NULL);

  init_shared_mem(0x10000); // 64 KB

  mq = new MessageQueue();
  comm = new CPU_CommunicationModule();

  /*
    // Initialize shared memory variables.
    received_message = (bool *)shared_mem;
    message = (char *)((int)shared_mem+4);
  */

  // Spawn the thread
  child_thread = fork();
  if (child_thread == (pid_t)-1) {
    cerr << "Could not spawn new thread!\n";
    exit(1);
  }

  if (/*!*/child_thread) {
    /*
    // Attach shared memory
    shared_mem = shmat(shmid, 0, 0);
    if (shared_mem == (char *)-1) {
      cerr << "Child (engine) could not attach shared memory pointer.\n";
      exit(1);
    }
    */

    // child process returns
    return true;
  }
  /*
  // Attach shared memory
  shared_mem = shmat(shmid, 0, 0);
  if (shared_mem == (char *)-1) {
    cerr << "Parent (engine) could not attach shared memory pointer.\n";
    exit(1);
  }
  */

  // Store incoming messages in file incoming.txt

  ofstream out("incoming.txt");
  out.setf(ios::unitbuf);

  string s;
  do {
    cin.sync();
    s = "";
    getline(cin, s);

    if (s != "") {
      out << s << "\n";
      if (!is_interrupt_message(s)) {
	//cerr << "mq->push(" << s << ")\n";
	if (!mq->push(s.c_str())) {
	  cerr << "Error: spawn_listener: !mq->push(" << s << ")\n";
	  assert(0);
	}
      }
    }
    
    /*
    while (*received_message) {
      // sleep for 40 ms
      usleep(40);
    }
    */

  } while (s != "quit");

  delete mq;
  delete comm;

  detach_shared_memory();
  //im_parant__kill_me();

  return false;
}

bool is_interrupt_message(string s) {
  if (s == "?") {
    // Interrupt search
    comm->move_now = true;
    return true;
  }
  return false;
}
