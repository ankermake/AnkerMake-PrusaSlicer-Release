#ifndef ANKER_GUI_CONFIG
#define ANKER_GUI_CONFIG

#define ANKER_RGB_INT 98, 211, 97

#define SCENE_BG_RGB_INT 24, 25, 27
#define SCENE_BG_RGB_FLOAT  0.094f, 0.098f, 0.106f

#define PANEL_BACK_RGB_INT 41, 42, 45
#define PANEL_BACK_LIGHT_RGB_INT 58, 59, 63
#define TEXT_LIGHT_RGB_INT 255, 255, 255
#define TEXT_DARK_RGB_INT 183, 183, 183

// id value of user define 
#define ID_FILAMENT_EDIT  (wxID_HIGHEST + 100)
#define ID_FILAMENT_REMOVE (wxID_HIGHEST + 101)
#define ID_PRESET_RENAME  (wxID_HIGHEST + 102)
#define ID_PRESET_DELETE (wxID_HIGHEST + 103)
#define ID_FILAMENT_COLOUR_BTN (wxID_HIGHEST + 104)
#define ID_FILAMENT_EDIT_BTN (wxID_HIGHEST + 105)


// comboBox size defined
#ifndef __APPLE__
#define ANKER_COMBOBOX_HEIGHT 30
#else
#define ANKER_COMBOBOX_HEIGHT 40
#endif


/********************************************** common font macro define begin *********************************************/
#ifndef __APPLE__
// ANKER_BOLD_FONT_BIG is used for big title
#define ANKER_BOLD_FONT_BIG wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))

// ANKER_BOLD_FONT_NO_1 is used for title 
#define ANKER_BOLD_FONT_NO_1 wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))
// ANKER_FONT_NO_1 is used for comboBox text and content
#define ANKER_FONT_NO_1 wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"))

// ANKER_BOLD_FONT_NO_2 and ANKER_FONT_NO_2 is used for content text and unit
#define ANKER_BOLD_FONT_NO_2 wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Microsoft YaHei"))
#define ANKER_FONT_NO_2 wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Microsoft YaHei"))

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
// ANKER_BOLD_FONT_BIG is used for big title
#define ANKER_BOLD_FONT_BIG wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))

// ANKER_BOLD_FONT_NO_1 is used for title
#define ANKER_BOLD_FONT_NO_1 wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))
// ANKER_FONT_NO_1 is used for combox text and content
#define ANKER_FONT_NO_1 wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("San Francisco"))


// ANKER_BOLD_FONT_NO_2 and ANKER_FONT_NO_2 is used for content text and unit
#define ANKER_FONT_NO_2 wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("San Francisco"))
#define ANKER_BOLD_FONT_NO_2 wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("San Francisco"))

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