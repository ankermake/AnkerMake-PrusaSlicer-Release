#ifndef ANKER_FONT_HPP
#define ANKER_FONT_HPP


#include <wx/stattext.h>
#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Singleton.hpp"

class AnkerGlobalFont
{
public:
    wxFont sysFont(int size, bool bold);
    wxFont sysFont(int size, int weight);
	void initSysFont();

public:

	wxFont Font_Head_14;
	wxFont Font_Head_12;


	wxFont Font_Body_14;
	wxFont Font_Body_12;
	wxFont Font_Body_10;
};

typedef Singleton<AnkerGlobalFont> AnkerFontSingleton;

#define Head_14  AnkerFontSingleton::getInstance().Font_Head_14
#define Head_12  AnkerFontSingleton::getInstance().Font_Head_12

#define Body_14  AnkerFontSingleton::getInstance().Font_Body_14
#define Body_12  AnkerFontSingleton::getInstance().Font_Body_12
#define Body_10  AnkerFontSingleton::getInstance().Font_Body_10

#endif