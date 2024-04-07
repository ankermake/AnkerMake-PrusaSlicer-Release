#pragma once
#include <map>
#include <wx/string.h>
#include "../Utils/Singleton.hpp"

using namespace std;

class AnkerGlobalConfig {


public:
		map<int, wxString> getColorMap() { return m_colorIdToRgb; }
		map<int, wxString> getMaterrialMap() { return m_materialIdToName; }
		void SimulateData() 
		{
			std::int64_t  materialIdArr[] = { 10000010,10000011 ,10000012 ,10000013 ,10000014 ,10000015 };
			wxString materialNameArr[] = {"PLA","ABS","TPU","PLA","PLA" ,"PLA" };
			std::int64_t  colourIdArr[] = { 10010001,10010002 ,10010003 ,10010004 ,10010005 ,10010006 };
			wxString colourNameArr[] = { "#000000","#FFFFFF","#AAAAA8","#E60012","#00358E" ,"#EB6100" };

			for (int i = 0; i< sizeof(materialIdArr) /sizeof(std::int64_t);i++ )
			{
				m_materialIdToName.insert(make_pair(materialIdArr[i], materialNameArr[i]));
				m_colorIdToRgb.insert(make_pair(colourIdArr[i], colourNameArr[i]));
			}
		}

private:
	map<int, wxString> m_colorIdToRgb;
	map<int, wxString> m_materialIdToName;
};

typedef Singleton<AnkerGlobalConfig> AnkerConfigSingleton;