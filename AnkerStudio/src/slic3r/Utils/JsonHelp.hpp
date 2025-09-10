//#pragma once
#ifndef _JSONHELP_H_
#define _JSONHELP_H_

#include <string>
#include <jansson.h>


namespace Slic3r {
class JsonHelp {
public:
	static bool GetDataB(json_t* jsonObj, const char* key, bool defaultValue = false);
	static int GetDataI(json_t* jsonObj, const char* key, int defaultValue = 0);
	static const char* GetDataS(json_t* jsonObj, const char* key, std::string defaultValue = "");
};
}
#endif
