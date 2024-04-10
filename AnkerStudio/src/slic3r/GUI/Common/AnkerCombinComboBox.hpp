#ifndef ANKER_BASE_HPP
#define ANKER_BASE_HPP

#include "AnkerGUIConfig.hpp"
#include <wx/string.h>

#ifdef CUSTOM_CONTROL
// _AnkerL:_L(X) , translate
#define _AnkerL(X) wxString(X)
#else
#include "../I18N.hpp"
#define _AnkerL(X) _L(X)

#endif

#ifdef __APPLE__
#define _AnkerFont(size) wxFont(size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "SF Pro Display")
#define _AnkerFontParams(size, family, style, weight, underlined) wxFont(size, family, style, weight, underlined, "SF Pro Display")
#elif _WIN32
#define _AnkerFont(size) wxFont(size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Microsoft YaHei")
#define _AnkerFontParams(size, family, style, weight, underlined) wxFont(size, family, style, weight, underlined, "Microsoft YaHei")
#endif // __APPLE__



class AnkerBase
{
public:
	AnkerBase();
	~AnkerBase();

	static wxString	AnkerResourceIconPath;

private:

};



#endif // !ANKER_BASE_HPP
