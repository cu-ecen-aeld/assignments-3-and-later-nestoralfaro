#include "threading.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param) {
  // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
  // hint: use a cast like the one below to obtain thread arguments from your parameter
  // struct thread_data* thread_func_args = (struct thread_data *) thread_param;
  struct thread_data* thread_func_args = (struct thread_data*) thread_param;

  usleep(thread_func_args->wait_to_obtain_ms * 1000); // convert ms to microseconds
  int lock_result = pthread_mutex_lock(thread_func_args->mutex);
  if (lock_result != 0) {
    ERROR_LOG("Failed to obtain mutex in thread");
    thread_func_args->thread_complete_success = false;
    return thread_param;
  }

  DEBUG_LOG("Thread obtained mutex");

  usleep(thread_func_args->wait_to_release_ms * 1000); // convert ms to microseconds
  int unlock_result = pthread_mutex_unlock(thread_func_args->mutex);
  if (unlock_result != 0) {
    ERROR_LOG("Failed to release mutex in thread");
    thread_func_args->thread_complete_success = false;
    pthread_mutex_unlock(thread_func_args->mutex); // ensure mutex is released
    return thread_param;
  }

  DEBUG_LOG("Thread released mutex");
  thread_func_args->thread_complete_success = true;
  return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms) {
  /**
   * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
   * using threadfunc() as entry point.
   *
   * return true if successful.
   *
   * See implementation details in threading.h file comment block
   */
  struct thread_data *thread_args = malloc(sizeof(struct thread_data));
  if (thread_args == NULL) {
    ERROR_LOG("Failed to allocate memory for thread_args");
    return false;
  }

  thread_args->mutex = mutex;
  thread_args->wait_to_obtain_ms = wait_to_obtain_ms;
  thread_args->wait_to_release_ms = wait_to_release_ms;
  thread_args->thread_complete_success = false;

  int create_result = pthread_create(thread, NULL, threadfunc, (void *)thread_args);
  if (create_result != 0) {
    ERROR_LOG("Failed to creat thread");
    free(thread_args);
    return false;
  }
  // free(thread_args);
  return true;
}
