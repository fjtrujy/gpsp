#define LIBCO_C
#include "libco.h"

#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>

/* Since cothread_t is a void pointer it must contain an address. We can't return a reference to a local variable
 * because it would go out of scope, so we create a static variable instead so we can return a reference to it.
 */
static s32 active_thread_id = -1;
extern void *_gp;

cothread_t co_active()
{
  active_thread_id = GetThreadId();
  return &active_thread_id;
}

cothread_t co_create(unsigned int size, void (*entrypoint)(void))
{
  /* Similar scenario as with active_thread_id except there will only be one active_thread_id while there could be many
   * new threads each with their own handle, so we create them on the heap instead and delete them manually when they're
   * no longer needed in co_delete().
   */
  cothread_t handle = malloc(sizeof(cothread_t));
  ee_thread_t thread;

  // u8 threadStack[size/8] __attribute__ ((aligned(16)));
  void *threadStack = (void *)malloc(size);

  if ( threadStack== NULL)
	{
		printf("libco: ERROR: creating threadStack\n");
		return(-1);
	}

	thread.stack_size		= size;
	thread.gp_reg			= &_gp;
	thread.func				= (void *)entrypoint;
	thread.stack			= threadStack;
	thread.option			= 0;
  thread.initial_priority = 1;

  s32 new_thread_id = CreateThread(&thread);
	if (new_thread_id < 0)
		printf("libco: ERROR: creating thread\n");
	
  StartThread(new_thread_id, NULL);
  *(u32 *)handle = new_thread_id;
  return handle;
}

void co_delete(cothread_t handle)
{
  TerminateThread(*(u32 *)handle);
	DeleteThread(*(u32 *)handle);
  free(handle);
}

void co_switch(cothread_t handle)
{
  WakeupThread(*(u32 *)handle);
  /* Sleep the currently active thread so the new thread can start */
  SleepThread();
}
