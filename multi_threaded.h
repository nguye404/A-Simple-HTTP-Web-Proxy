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

/*
 * Calls the user defined function to continue process
 * When user process completes, deallocates Thread_Args, closes the sock and ends the thread
 * @params void pointer to type struct Thread_Args containing the connected sock and an Event type
 * @return NULL after detatching the thread, which closes the thread
*/

static void*
thread_main(void* args) {
  Thread_Args* thread_args = (struct Thread_Args*) args;
  Event e = thread_args->event;
  int* sock = (int*)thread_args->client_sock;
  e.param = sock;

  e.fn(e.param);

  delete thread_args;

  close(*sock);
  pthread_detach(pthread_self());
  return NULL;
}

/*
 * Creates a sock connected to the local sock, calls pthread_create to create a new thread then call
 *     thread_main which runs the user's function passed in by Event
 * Allocated memory is handled in thread_main function
 * @params Event type which contains address to a function call and the parameters passed into the
 *     function
 * @return void, each thread created by pthread_create will detatch itself and exit
*/
// STUDY: is there a need for a return type?
// STUDY: can we use pointers and not get null exceptions?
static void
run_thread(Event& e) {
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
      thread_args->event.fn = e.fn;
      // Create client thread
      pthread_t threadID;
      int status = pthread_create(&threadID, NULL, thread_main, (void*) thread_args);
      if (status != 0) {
          close(client_sock);
        }
    }
}
#endif // __MULTI_THREAD_H__
