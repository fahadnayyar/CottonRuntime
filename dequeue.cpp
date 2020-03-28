#include "dequeue.h"
#include "cotton-runtime.h"
#include "cotton-atomic.h"

namespace cotton
{

	//dequeue constructor, initializes the per dequeue lock.
	dequeue::dequeue()
	{
		// if (pthread_mutex_init(&dequeue_lock, NULL)!=0)
		// 	printf("dequeue_lock init failed\n");
	}

	//pushes task on dequeue head without any synchronization.
	void dequeue::push(task * t) 
	{
		int size = tail - head;
		if (size == dequeue_capacity) { /* deque looks full */
			/* may not grow the deque if some interleaving steal occur */
			// std::cout<<getenv("PMI_RANK") <<": Deque full for worker-"<<current_ws()->id << std::endl;
			// HASSERT("DEQUE full, increase deque's size " && 0);
			printf("ERROR: dequeue is full\n");
		}
		int n = (tail) % dequeue_capacity;
		arr[n] = t;
		tail++;
		//return true;
	 
		
	}

	//pops task from head of dequeue using per dequeue mutex lock for synchronization.
	//this function is called by only the owner worker thread.
	task * dequeue::pop()
	{
		hc_mfence();
		int tail1 = tail;
		tail1--;
		tail = tail1;
		hc_mfence();
		int head1 = head;

		int size = tail1 - head1;
		if (size < 0) {
			tail = head;
			return NULL;
		}
		task * t = (task *) arr[(tail1) % dequeue_capacity];

		if (size > 0) {
			return t;
		}

		/* now size == 1, I need to compete with the thieves */
		if (!hc_cas(&head, head1, head1 + 1)) {
			t = NULL;
		}

		/* now the deque is empty */
		tail = head;
		return t;
	}


	//steals task from tail of dequeue using per dequeue mutex loxck for synchronization.
	//this function is called by other worker threads.
	task * dequeue::steal()
	{
		int head1;
		/* Cannot read deq->data[head] here
		 * Can happen that head=tail=0, then the owner of the deq pushes
		 * a new task when stealer is here in the code, resulting in head=0, tail=1
		 * All other checks down-below will be valid, but the old value of the buffer head
		 * would be returned by the steal rather than the new pushed value.
		 */
		int tail1;

		head1 = head;
		hc_mfence();
		tail1 = tail;
		if ((tail1 - head1) <= 0) {
			return NULL;
		}

		task * t = (task *) arr[head1 % dequeue_capacity];
		/* compete with other thieves and possibly the owner (if the size == 1) */
		if (hc_cas(&head, head1, head1 + 1)) { /* competing */
			return t;
		}
		return NULL;




			
	}
}
