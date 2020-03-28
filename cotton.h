#include <functional>
#include <cstring>
#include <cstdio>
#include <cstdlib>
namespace cotton
{
	//main user interfaces:
	void init_runtime();
	void start_finish();
	void async(std::function<void()> && lambda); //d
	void end_finish();
	void finalize_runtime();
}