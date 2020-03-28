/*	issues:
	1.) no checks for malloc and pthread_get/set_specific, pthread_mutex lock/unlock, 
	2.) which files to include and where?
	3.) Any other degugging counter required?
	4.) How to manage multiple files together?
	5.) what should be there in makefile?
*/

#include "cotton.h"
#include "cotton-runtime.h"
#include <pthread.h>
#include <stdlib.h>
#include <functional>
#include "dequeue.h"
#include "hermes-wps.h"

namespace cotton {

//debugging stuffs

//mutex lock to protect shared global variable finish_counter.
pthread_mutex_t finish_lock;

//global varaible telling whether the status of runtime as on or off.
volatile bool shutdown = false;

//shared global variable which keeps count of asynch calls/ created tasks which have not been handled yet.
volatile int finish_counter = 0;

//no of worker threads availabe, default value is 1. This is received as environment variable COTTON_WORKERS from user.
int pool_size = 1; 

//array of per thread dequeue. Initialized aat runtime by receiving pool size from environment variable COTTON_WORKERS.
dequeue * dequeues;

//pthread - speciifc data. Stores the unique id of the worker. This is a per thread varaibel.
pthread_key_t thread_id;

//call to initialize runtime. This call creates and starts all the worker threads.
void init_runtime() 
{
	//for debugging purposes.
	int ret;

	//setting up pool size by taking input from environment variable COTTON_WORKERS
	char * pool_size_ptr = (char *)getenv("COTTON_WORKERS");
	if (pool_size_ptr!=NULL)
	{
		pool_size = atoi(pool_size_ptr);
	}
	//debudding to check whether correct pool size is set or not.

	//hermes-wps call
	#ifdef hermes_wps
		wps_init(pool_size);
	#endif	


	//initializing dequeue array.
	//dequeues is the array which contain references to per thread dequeue. and alse perthread pthread variable used in pthread_create and pthread_join.
	dequeues = new dequeue[pool_size];	

	//creating the thread-specific data thread_id, which is used to get the id (from 0 to n) of a worker.
	ret = pthread_key_create(&thread_id,NULL);
	#ifdef ENABLE_CHECKS
		if (ret!=0)
			printf("ERROR: in creating thread-specific data thread_id");
	#endif

	//setting thread_id of the main thread as 0.
	void * ptr;
	if ((ptr = pthread_getspecific(thread_id))==NULL)
	{
		int * ptr1 = (int *)malloc(sizeof(int));
		*ptr1 = 0;
		pthread_setspecific(thread_id,(void *) ptr1);
	}


	//initializinf the lock variable for finish_counter.
	ret = pthread_mutex_init(&finish_lock, NULL)!=0;
	#ifdef ENABLE_CHECKS
		if (ret!=0)
			printf("ERROR: finish_lock init failed\n");
	#endif
	
	//creating all the pthreads for workers.
	int size = thread_pool_size(); 
	int i;
	for (i=1;i<size;i++)
	{
		int *ii =(int *)malloc(sizeof(int));
		*ii = i;
		ret = pthread_create(&dequeues[i].p_thread,NULL,worker_routine,(void*)ii);	

		#ifdef ENABLE_CHECKS
			if (ret!=0)
				printf("ERROR: in call to pthread_create");
		#endif
	}
	
}

//marks start of finish scope. Initilize finish_counter to 0.
void start_finish() 
{
	finish_counter=0;
}

//asyncc call. Takes c++11 lambda function as input ans sores it as a task in thread specific dequeue.
//increaments finish_counter. Pushes new task to runtime(in calling thread's dequeue).
void async(std::function<void()> && lambda) 
{
	task * p;
	p = new std::function<void()>(lambda);
	lock_finish();
	finish_counter++;
	unlock_finish();
	push_task_to_runtime(p);
	return;
}

// marks end of a finish scope. Makes master/main thread work as a worker until all tasks are done (until finish_counter=0).
void end_finish() 
{
	while (finish_counter!=0)
	{
		find_and_execute_task();
	}
}

// Shutting down of runtime. Marks shutdown as true and waits for all threads to join.
void finalize_runtime() 
{
	shutdown = true;
	int size = thread_pool_size();
	int i;
	
	for (i=1;i<size;i++)
	{
		pthread_join(dequeues[i].p_thread,NULL);
	}

	long total_tasks=0, total_steals=0, total_downs=0;
	for(int i=0; i<size; i++) {
		total_tasks += dequeues[i].task_counter;
		total_steals += dequeues[i].steal_counter;
		total_downs += dequeues[i].down_counter;
	}
	printf("Total tasks = %ld, total steals = %ld, total down count = %ld\n",total_tasks, total_steals,total_downs);
	//debugging purpose

	delete dequeues;
	#ifdef hermes_wps
		wps_shutdown(pool_size);
	#endif		
}

// returns size of thread pool, given by user in environment variable COTTON_WORKER.
int thread_pool_size() 
{
	return pool_size;
}

// entry point of each worker thread. Thread specific id is saved then this thread keeps pulling tasks out of runtime and executing them.
void * worker_routine(void * id1)  
{
	//debugging purposes
	
	//setting up pthread-specific data thread_id.
	int * id = (int *)id1;
	void * ptr;
	if ((ptr = pthread_getspecific(thread_id))==NULL)
	{
		int * ptr1 = (int *)malloc(sizeof(int));
		*ptr1 = *id;
		pthread_setspecific(thread_id,(void *) ptr1);
	}


	while(!shutdown)	
	{
		find_and_execute_task();
	}
	
	//debugging purposes

	return (void *)1;
}

//pops a task out of runtime datastructures, execute the task, free the task, decrement finish_counter.
void find_and_execute_task() 
{
	task * task1 = pop_task_from_runtime();
	if (task1 != NULL)
	{
		execute(task1);
		delete task1;
		lock_finish();
		finish_counter--;;
		unlock_finish();
	}
}

//takes mutex lock on finish_counter.
void lock_finish() 
{
	pthread_mutex_lock(&finish_lock);
}

//releases mutex lock on finish counter.
void unlock_finish() 
{
	pthread_mutex_unlock(&finish_lock);
}

//execute the input task (which is a saved c++11 lambda function). 
void execute(task * t) 
{
	(*t)();
	 
	//debugging purpose
}

//returns a task from runtime, either by popping from calling thread's own dequeue or stealing from some other thread's dequeue.
//stealing is done only if the dequeue of current thread is empty.
task * pop_task_from_runtime() 
{
	task * toret;
	int s = thread_pool_size(); 
	int tid = *(int *)pthread_getspecific(thread_id);
	if ((toret = dequeues[tid].pop()) == NULL)
	{
		dequeues[tid].down_counter++;

		//hermer-wps call.
		#ifdef hermes_wps
			wps_relay_immediacy(tid);
		#endif
		

		int r;
		while (true)
		{	
			//printf("%d\n",finish_counter);
			if(finish_counter<=0)
				break;
			r = rand()%s;
			if (r == tid)
			{
				if ((toret=dequeues[tid].pop())!=NULL)
					break;
			}
			else
			{
				if ((toret = dequeues[r].steal()) != NULL)
				{
					dequeues[tid].steal_counter++;
					//hermer-wps call.
					#ifdef hermes_wps 
						wps_thief_procrastination(tid,r);
					#endif 

					//debugging purpose
					break;
				}
			}
		}
	}
	return toret;
}

//pushes the input task on calling thread's dequeue.
void push_task_to_runtime(task * t) 
{
	int tid = *(int *)pthread_getspecific(thread_id);
	dequeues[tid].push(t);
	dequeues[tid].task_counter++;

	//debugging purpose
}
}

