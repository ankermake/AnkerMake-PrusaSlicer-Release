#include <wx/graphics.h>
#include "AnkerRoundBaseDialog.hpp"



void AnkerRoundBaseDialog::OnSize(wxSizeEvent& event)
{
	// Handle the size event here
	wxSize newSize = event.GetSize();
	static bool isProcessing = false;
	if (!isProcessing)
	{
		isProcessing = true;

		wxGraphicsPath path = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
		path.AddRoundedRectangle(0, 0, newSize.GetWidth(), newSize.GetHeight(), 8);
		SetShape(path);
		isProcessing = false;
	}

	event.Skip();
}

AnkerRoundBaseDialog::AnkerRoundBaseDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	InitUi();
	InitEvent();
}

AnkerRoundBaseDialog::~AnkerRoundBaseDialog()
{

}

void AnkerRoundBaseDialog::InitUi()
{
	SetWindowStyleFlag(GetWindowStyleFlag() | wxFRAME_SHAPED);
	SetBackgroundColour(DEFAULT_ROUND_BG_COLOR);
}

void AnkerRoundBaseDialog::InitEvent()
{
	Connect(wxEVT_SIZE, wxSizeEventHandler(AnkerRoundBaseDialog::OnSize));
}
