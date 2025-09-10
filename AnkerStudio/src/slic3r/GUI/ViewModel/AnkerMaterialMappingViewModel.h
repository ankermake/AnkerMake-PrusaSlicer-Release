#pragma once
#include <map>
#include "AnkerNetDefines.h"
using namespace AnkerNet;
namespace Slic3r {
	namespace GUI {

		enum FileSelectMode
		{
			FSM_NONE,
			FSM_COMPUTER,
			FSM_STORAGE,
			FSM_USB,
			FSM_SLICE
		};

		typedef struct tagGcodeFilementInfo {
			wxString strMaterialName;
			std::int64_t iFilamentId;
			wxColor filamentColor;
			std::int64_t iCoLorId;
			int iNozzelInx;

			bool operator<(const tagGcodeFilementInfo& rhs) const {
				return iNozzelInx < rhs.iNozzelInx;
			}

		}GcodeFilementInfo;

		typedef struct tagDeviceFilementInfo {
			wxString strMaterialName;
			std::int64_t iFilamentId;
			wxColor filamentColor;
			wxString strColor;
			std::int64_t iCoLorId;
			int iNozzelInx;
			bool bIsEdit = false;
			float fApproximateDegree = 0.0f;
			NozzleStatus nozzleStatus;

		}DeviceFilementInfo;


		class AnkerMaterialMappingViewModel 
		{
		public:
			void clear() 
			{
				if (m_previewImage.IsOk()) 
				{
					m_previewImage.Clear();
				}
				m_filamentCost = "";
				m_gcodeFilePath = "";
				m_PrintTime = "";
				m_SelectedFilamentInxVec.clear();
				m_vrCardInfoMap.clear();
				m_FileSelectMode = FSM_NONE;
				m_curFilamentMap.clear();
			}

		public:
			std::map<GcodeFilementInfo, std::vector<DeviceFilementInfo>> m_curFilamentMap;
			std::vector<int> m_SelectedFilamentInxVec;
			wxImage m_previewImage;
			wxString m_filamentCost;
			wxString m_PrintTime;
			std::string m_gcodeFilePath;
			VrCardInfoMap m_vrCardInfoMap;
			FileSelectMode m_FileSelectMode;

		};

		class AnkerMaterialMapItemViewModel 
		{
		public:
			wxString m_filamentNameVM;
			wxColor m_gcodeFilamentColorVM;
			std::vector<DeviceFilementInfo> m_deviceFilamentInfoVM;
			int m_MapedInx;
			wxString m_mappedFilamentName;

			void clear()
			{
				m_gcodeFilamentColorVM = wxColour(0, 0, 0);
				m_MapedInx = -1;
				m_filamentNameVM = "";
				m_mappedFilamentName = "";
			}

			AnkerMaterialMapItemViewModel()
			{
				clear();
			}
		};
	}
}
