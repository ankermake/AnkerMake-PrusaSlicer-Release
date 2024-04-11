#include "basetype.hpp"
#include "libslic3r/Utils.hpp"

void PrintLog(const std::string& str)
{
#ifdef _WIN32
	OutputDebugStringA((str + std::string("\n")).c_str());
#elif __APPLE__
	std::cout << str;
#endif // _WIN32
	ANKER_LOG_INFO << str;
}

void mySleep(UINT64 ms)
{
#ifdef _WIN32
	Sleep(ms);
#elif __APPLE__
	usleep(ms * 1000);
#endif // _WIN32

}