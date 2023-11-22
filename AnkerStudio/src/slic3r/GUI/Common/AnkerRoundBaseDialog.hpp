#ifndef ANKER_ROUND_BASE_DIALOG_HPP
#define ANKER_ROUND_BASE_DIALOG_HPP
#include <wx/wx.h>

const wxColour DEFAULT_ROUND_BG_COLOR(33, 34, 38);

class AnkerRoundBaseDialog: public wxDialog
{
public:
	AnkerRoundBaseDialog(wxWindow* parent);
	~AnkerRoundBaseDialog();


	void InitUi();
	void InitEvent();
	void OnSize(wxSizeEvent& event);

private:
};
#endif // !ANKER_ROUND_BASE_DIALOG_HPP
