#include "AnkerFont.hpp"



#ifndef ANKER_GUI_CONFIG
#define ANKER_GUI_CONFIG

#define ANKER_RGB_INT 98, 211, 97
#define ANKER_DARK_RGB_INT 49, 105, 49

#define SCENE_BG_RGB_INT 24, 25, 27
#define SCENE_BG_RGB_FLOAT  0.094f, 0.098f, 0.106f

#define PANEL_BACK_RGB_INT 41, 42, 45
#define PANEL_TITLE_BACK_RGB_INT 58, 59, 63
#define PANEL_TITLE_BACK_DARK_RGB_INT 32, 33, 36
#define PANEL_BACK_LIGHT_RGB_INT 58, 59, 63
#define TEXT_LIGHT_RGB_INT 255, 255, 255 
#define TEXT_DARK_RGB_INT 169, 170, 171  //#a9aaab
#define TITLE_TEXT_DARK_RGB_INT 105, 106, 108  //#696a6c

// all self-defined wxwidget ids, please defined here
enum {
	ID_FILAMENT_EDIT = wxID_HIGHEST + 100,
	ID_FILAMENT_REMOVE,
	ID_PRESET_RENAME,
	ID_PRESET_DELETE,
	ID_FILAMENT_COLOUR_BTN,
	ID_FILAMENT_EDIT_BTN,
	ID_GCODE_IMPORT_DIALOG,
	ID_PRIVACY_CHOICES_ITEM
};

// comboBox size defined
#ifndef __APPLE__
#define ANKER_COMBOBOX_HEIGHT 30
#else
#define ANKER_COMBOBOX_HEIGHT 40
#endif


#define ANKER_BOLD_FONT_BIG		Head_14
#define ANKER_BOLD_FONT_NO_1	Head_14
#define ANKER_FONT_NO_1			Body_14
#define ANKER_BOLD_FONT_NO_2	Head_12
#define ANKER_FONT_NO_2			Body_12



/********************************************** common font macro define begin *********************************************/
#ifndef __APPLE__

// only for device temperature number font
#define ANKER_FONT_TEMPERATURE_UNIT wxFont(28, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))
#define ANKER_FONT_TEMPERATURE_NUM wxFont(25, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))
#define ANKER_FONT_TEMPERATURE_UNITEX wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"))

// only for device nav list
#define ANKER_FONT_DEVICE_NAV_ITEM wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"))

// ANKER_FONT custom size 
#define ANKER_BOLD_FONT_SIZE(fontSize) wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))
#define ANKER_FONT_SIZE(fontSize) wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"))

// ANKER_FONT custom font type 
#define ANKER_BOLD_FONT_SIZE_TYPE(fontSize, fonttype) wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT(fonttype))
#define ANKER_FONT_SIZE_TYPE(fontSize, fonttype) wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT(fonttype))


#else

// only for device temperature number font
#define ANKER_FONT_TEMPERATURE_UNIT wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))
#define ANKER_FONT_TEMPERATURE_NUM wxFont(21, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))
#define ANKER_FONT_TEMPERATURE_UNITEX wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("San Francisco"))

// only for device nav list
#define ANKER_FONT_DEVICE_NAV_ITEM wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("San Francisco"))

// ANKER_FONT custom size 
#define ANKER_BOLD_FONT_SIZE(fontSize) wxFont(fontSize + 4, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))
#define ANKER_FONT_SIZE(fontSize) wxFont(fontSize + 4, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("San Francisco"))

// ANKER_FONT custom font type 
#define ANKER_BOLD_FONT_SIZE_TYPE(fontSize, fonttype) wxFont(fontSize + 4, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT(fonttype))
#define ANKER_FONT_SIZE_TYPE(fontSize, fonttype) wxFont(fontSize + 4, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT(fonttype"))


#endif // !__APPLE__	
/********************************************** common font macro define end *********************************************/






#endif // ANKER_GUI_CONFIG