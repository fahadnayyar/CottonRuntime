#include "hermes-wps.h"
#include <stdlib.h>
#include <pthread.h>
#include <functional>

pthread_mutex_t wps_global_lock;
struct wps_dll_node * wps_structure_array;
int up_counter;
int down_counter;
int WORKERS_NO;
//helper functions def:
void wps_down(int w, int v);
void wps_up(int w);
int min(int x, int y);
int max(int x, int y);


void wps_init(int n) // n is no. of workers available
{
	int ret;
	//initializinf the global lock variable.
	ret = pthread_mutex_init(&wps_global_lock, NULL)!=0;
	#ifdef wps_chcek
		if (ret!=0)
			printf("ERROR: wps_global_lock init failed\n");
	#endif
	
	

	#ifdef wps_debug
		up_counter = 0;
		down_counter = 0;
	#endif 

	//assert((wps_structure_array = (wps_dll_node*) malloc(sizeof(wps_dll_node) * n)));
	assert((wps_structure_array = (wps_dll_node*) malloc(sizeof(wps_dll_node) * n)));
	int i;
	for (i=0;i<n;i++)
	{
		wps_structure_array[i].wid = i;
		wps_structure_array[i].prev = NULL;
		wps_structure_array[i].next = NULL;
		wps_structure_array[i].tempo = max_tempo;
	}
	
	WORKERS_NO = n;
}
void wps_shutdown(int n) //n is the no. of workers.
{
	free( wps_structure_array);
	#ifdef wps_debug
		printf("No. of wps_up called: %d\n",up_counter);
		printf("No. of wps_down called: %d\n",down_counter);
	#endif
}

void wps_relay_immediacy(int w)
{
	
	pthread_mutex_lock(&wps_global_lock);
	struct wps_dll_node * w0 = &wps_structure_array[w];
	struct wps_dll_node * w00 = w0->next;
	while (w00 != NULL)
	{
		wps_up(w00->wid); //doubt
		w00 = w00->next;
		printf("%d\n",w);
		
	}
	if (w0->prev!=NULL)
	{
		w0->prev->next = w0->next;
	}
	if (w0->next!=NULL)
	{
		w0->next->prev = w0->prev;
	}
	w0->next = NULL;
	w0->prev = NULL;
	pthread_mutex_unlock(&wps_global_lock);
}

void wps_thief_procrastination(int w, int v) // v is victim, w is thief.
{
	
	pthread_mutex_lock(&wps_global_lock);
	wps_down(w,v);
	struct wps_dll_node * v0 = &wps_structure_array[v];
	struct wps_dll_node * w0 = &wps_structure_array[w];
	if (v0->next != NULL)
	{
		w0->next = v0->next;
		v0->next->prev = w0;
		//v0->prev = w0->prev; // bug in paper
	}
	v0->next = w0;
	w0->prev = v0;
	pthread_mutex_unlock(&wps_global_lock);

}

void wps_down(int w, int v)
{
	#ifdef wps_debug
		down_counter++; 
	#endif
	wps_structure_array[w].tempo = min(wps_structure_array[v].tempo - 1,1);
}
void wps_up(int w)
{
	#ifdef wps_debug
		up_counter++; 
	#endif
	wps_structure_array[w].tempo = max(wps_structure_array[w].tempo+1,max_tempo);
}
int min(int x, int y)
{
	if (x>y)
		return y;
	else
		return x;
}
int max(int x, int y)
{
	if (x>y)
		return x;
	else
		return y;

}
