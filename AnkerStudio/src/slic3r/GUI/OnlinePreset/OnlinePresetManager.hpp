#ifndef slic3r_OnlinePresetManager_hpp_
#define slic3r_OnlinePresetManager_hpp_

#include <wx/wx.h>
#include "OnlinePresetInfo.hpp"

namespace Slic3r {
namespace GUI {
	class OnlinePresetManager
	{
	public:
		//OnlinePresetManager();

		// Check whether there is an updateable file locally.
		// Includes version checking and file checking.
		bool CheckUpdateable();

		// Displays an update prompt popup and returns whether the user chooses to update.
		bool ChooseUpdate();

		// Perform updates.
		// Including file backup, file copy, and execution result check.
		void ExecuteUpdate();

		// Including CheckUpdateable , ChooseUpdate , ExecuteUpdate
		void CheckAndUpdate();


	private:
		OnlinePresetInfo info;
	};

} // GUI
} // Slic3r

#endif /* slic3r_2DBed_hpp_ */
