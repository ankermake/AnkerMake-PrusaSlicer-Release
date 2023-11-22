#include "AnkerSplitCtrl.hpp"


AnkerSplitCtrl::AnkerSplitCtrl(wxWindow* parent)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
{
	initUI();
}

AnkerSplitCtrl::~AnkerSplitCtrl()
{
}

void AnkerSplitCtrl::initUI()
{
    SetBackgroundColour(wxColour(56, 57, 60));
    SetMaxSize(wxSize(100000, 1));
    SetMinSize(wxSize(1, 1));
}
