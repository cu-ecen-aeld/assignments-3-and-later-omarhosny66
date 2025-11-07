#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    // delay before attempting the lock
    struct thread_data* thread = (struct thread_data*)thread_param;

    // wait before attempting the lock
    usleep(thread->wait_to_obtain_ms * 1000);

    int rc = pthread_mutex_lock(thread->mutex);
    if (rc != 0) {
        thread->thread_complete_success = false;
        ERROR_LOG("pthread_mutex_lock failed with %d\n", rc);
        return thread;
    }

    // wait the specified period before mutex release
    usleep(thread->wait_to_release_ms * 1000);

    rc = pthread_mutex_unlock(thread->mutex); 
    if (rc != 0) {
        thread->thread_complete_success = false;
        ERROR_LOG("pthread_mutex_unlock failed with %d\n", rc);
        return thread;
    }

    thread->thread_complete_success = true;
    return thread;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *created_thread = malloc(sizeof(struct thread_data));
    if (!created_thread) {
        ERROR_LOG("failed to allocate memory for the thread\n");
        return false;
    }

    created_thread->mutex = mutex;
    created_thread->wait_to_obtain_ms = wait_to_obtain_ms;
    created_thread->wait_to_release_ms = wait_to_release_ms;
    created_thread->thread_complete_success = false;

    int rc = pthread_create(thread, NULL, threadfunc, created_thread);
    if (rc != 0) {
        ERROR_LOG("failed to free the thread memory\n");
        free(created_thread);
        return false;
    }

    return true;
}

