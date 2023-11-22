#ifndef ANKER_OTA_NOTES_BOX_HPP
#define ANKER_OTA_NOTES_BOX_HPP

#include "AnkerDialog.hpp"
#include <wx/tokenzr.h>

enum OtaType
{
	OtaType_Normal,
	OtaType_Forced,
};

class AnkerOtaNotesNormalPanel : public AnkerDialogCancelOkPanel
{
public:
	AnkerOtaNotesNormalPanel(wxWindow* parent, const wxString& title = "", const wxString& version = "", const wxString& context = "", const wxSize& size = wxDefaultSize);
    ~AnkerOtaNotesNormalPanel();

private:
};

class AnkerOtaNotesForcedPanel : public AnkerDialogOkPanel
{
public:
	AnkerOtaNotesForcedPanel(wxWindow* parent, const wxString& title = "", const wxString& version = "",  const wxString& context = "", const wxSize& size = wxDefaultSize);
	~AnkerOtaNotesForcedPanel();
};


class AnkerOtaNotesDialog : public AnkerDialog
{
public:
	AnkerOtaNotesDialog(wxWindow* parent,
		wxWindowID id,
		const wxString& title,
		const wxString& version,
		const wxString& context,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE,
		const wxString& name = wxASCII_STR(wxFrameNameStr));
	~AnkerOtaNotesDialog();

	virtual void InitDialogPanel(int dialogType = 0);
	virtual void setBackgroundColour(const wxColour& color = "#333438");
	virtual int ShowAnkerModal(int dialogType = 0);

private:
	wxString m_version;
};



#endif // !ANKER_OTA_NOTES_BOX_HPP
