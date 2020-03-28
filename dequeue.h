#include "cotton-runtime.h"

namespace cotton
{
	//compile time constant: size of dequeues.
	#define dequeue_capacity 8192

	class dequeue
	{
		//dequeue array.
		task * arr[dequeue_capacity];
		
		//head of dequeue.
		volatile int head=0;
		
		//tail of dequeue.
		volatile int tail=0;
		
		public:
			pthread_t p_thread;
		long task_counter=0;
		long steal_counter=0;
		long down_counter=0;
		

			//dequeue constructor, initializes the per dequeue lock.
			dequeue();
			
			//tells whether dequeue is full.
			bool is_full();

			//tells whether dequeue is empty.
			bool is_empty();

			//pushes task on dequeue head without any synchronization.
			void push(task * t); 

			//pops task from head of dequeue using per dequeue mutex lock for synchronization.
			//this function is called by only the owner worker thread.
			task * pop();

			//steals task from tail of dequeue using per dequeue mutex loxck for synchronization.
			//this function is called by other worker threads.
			task * steal();
	};	

}	
