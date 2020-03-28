#include <cstdio>
#include <cstdlib>
#include <cassert>
#define wps_chcek
#define wps_debug
#define hermes_wps
#define max_tempo 3

typedef struct wps_dll_node
{
	int wid; // worker's id // 4 bytes
	struct wps_dll_node * prev; //4bytes
	struct wps_dll_node * next; //4bytes
	int tempo; // 4 bytes
	char arr[48]; //52 bytes padding to avoid false sharing
}wps_dll_node;

void wps_relay_immediacy(int w);
void wps_init(int n);
void wps_thief_procrastination(int w, int v);
void wps_shutdown(int n);

