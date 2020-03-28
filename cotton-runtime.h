#include <functional>
#include <cstring>
#include <cstdio>
#include <cstdlib>
	//macro for turning on/off the debug code.
	#define ENABLE_DEBUG
	
	//macro for turning on/off the error checking code.
	//#define ENABLE_CHECKS
namespace cotton
{
	
	//task is nothing but a std function, storing lambda.
	#define task std::function<void()>

	//main interface for users
	void init_runtime();
	void start_finish();
	void async(std::function<void()> && lambda); 
	void end_finish();
	void finalize_runtime();

	//private functions of runtume library.
	int thread_pool_size();
	void * worker_routine(void * id1);
	void find_and_execute_task();
	void lock_finish();
	void unlock_finish();
	void execute(task * t);
	task * pop_task_from_runtime();
	void push_task_to_runtime(task * t);

	

}

