#ifndef __MULTI_THREAD_H__
#define __MULTI_THREAD_H__

#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>

typedef struct Event {
  void (*fn)(void*);
  void* param;
} Event;

typedef struct Thread_Args {
  int* client_sock;
  Event event;
} Thread_Args;

static void*
thread_main(void* args) {
  Thread_Args* thread_args = (struct Thread_Args*) args;
  Event event = thread_args->event;
  int* sock = (int*)thread_args->client_sock;
  event.param = sock;
  event.fn(event.param);

  delete thread_args;

  close(*sock);
  pthread_detach(pthread_self());
  return NULL;
}

// STUDY: is there a need for a return type?
// STUDY: can we use pointers and not get null exceptions?
static void
run_thread(Event& event) {
  int client_sock;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  while (true)
    {
      int* sock_ptr = (int*) e.param;
      if ((client_sock = accept(*sock_ptr, (struct sockaddr*) &client_addr, &client_addr_len)) < 0) {
          //perror("accept");
        }

      Thread_Args* thread_args = new Thread_Args;
      thread_args->event.fn = nullptr;
      // Create and initialize argument struct
      thread_args->client_sock = &client_sock;
      thread_args->event.fn = event.fn;
      // Create client thread
      pthread_t threadID;
      int status = pthread_create(&threadID, NULL, thread_main, (void*) thread_args);
      if (status != 0) {
          close(client_sock);
        }
    }
}
#endif // __MULTI_THREAD_H__
