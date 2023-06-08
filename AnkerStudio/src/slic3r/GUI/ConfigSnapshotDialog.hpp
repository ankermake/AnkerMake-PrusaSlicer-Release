#ifndef slic3r_GUI_ConfigSnapshotDialog_hpp_
#define slic3r_GUI_ConfigSnapshotDialog_hpp_

#include "GUI.hpp"
#include "GUI_Utils.hpp"

#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/html/htmlwin.h>

namespace Slic3r { 
namespace GUI {

namespace Config {
	class SnapshotDB;
}

class ConfigSnapshotDialog : public DPIDialog
{
public:
    ConfigSnapshotDialog(const Config::SnapshotDB &snapshot_db, const wxString &id);
    const std::string& snapshot_to_activate() const { return m_snapshot_to_activate; }

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;

private:
    void onLinkClicked(wxHtmlLinkEvent &event);
    void onCloseDialog(wxEvent &);

    // If set, it contains a snapshot ID to be restored after the dialog closes.
    std::string m_snapshot_to_activate;

    wxHtmlWindow* html;
};

} // namespace GUI
} // namespace Slic3r

#endif /* slic3r_GUI_ConfigSnapshotDialog_hpp_ */
