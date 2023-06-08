#include "AnkerLoginDialog.hpp"
#include <wx/artprov.h>

BEGIN_EVENT_TABLE(AnkerLoginDialog, wxControl)
EVT_PAINT(AnkerLoginDialog::OnPaint)
EVT_ENTER_WINDOW(AnkerLoginDialog::OnEnter)
EVT_LEAVE_WINDOW(AnkerLoginDialog::OnLeave)
EVT_LEFT_DOWN(AnkerLoginDialog::OnPressed)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerLoginDialog, wxControl)

AnkerLoginDialog::AnkerLoginDialog(wxWindow* parent,
									wxWindowID id,
									const wxPoint& pos /*= wxDefaultPosition*/,
									const wxSize& size /*= wxDefaultSize*/,
									long style /*= wxBORDER_NONE*/,
									const wxValidator& validator /*= wxDefaultValidator*/)
{
	Create(parent, id, pos, size, style, validator);
	SetMinSize(wxSize(57, 22));

	m_Name = wxString("Sign In");	
	m_Avatar = wxString("plater");
	m_isLogin = false;
}

AnkerLoginDialog::AnkerLoginDialog()	
{
	
}

bool AnkerLoginDialog::getLoginStatus()
{
	return m_isLogin;
}

void AnkerLoginDialog::setLoginStatus(bool loginStatus)
{
	m_isLogin = loginStatus;
}

void AnkerLoginDialog::reset()
{
	setName(wxString("Sign In"));	
	setAvatar(wxString("plater"));
}

void AnkerLoginDialog::setAvatar(const wxString& strAvatar)
{
	m_Avatar = strAvatar;
	Refresh();
	Update();
}

void AnkerLoginDialog::setName(const wxString& strName)
{
	m_Name = strName;

	if (m_Name.size() > 10)
		SetMinSize(wxSize(83, 22));

	Refresh();
	Update();
}

bool AnkerLoginDialog::Create(wxWindow* parent,
							wxWindowID id, 
							const wxPoint& pos /*= wxDefaultPosition*/,
							const wxSize& size /*= wxDefaultSize*/,
							long style /*= wxSUNKEN_BORDER*/, 
							const wxValidator& validator /*= wxDefaultValidator*/)
{
	if (!wxControl::Create(parent, id, pos, size, style, validator))
	{
		return false;
	}

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	return true;
}

void AnkerLoginDialog::OnEnter(wxMouseEvent& event)
{
	SetCursor(wxCursor(wxCURSOR_HAND));
}

void AnkerLoginDialog::OnLeave(wxMouseEvent& event)
{
}

void AnkerLoginDialog::OnPressed(wxMouseEvent& event)
{	
	Refresh();
	Update();
}

void AnkerLoginDialog::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	auto dialogRect = this->GetRect();

// 	wxBrush brush(wxColor("#1E1E1E"));
// 	wxPen pen(wxColor("#4C4C4C"));
// 	dc.SetBrush(brush);
// 	dc.SetPen(pen);
// 	dc.DrawRoundedRectangle(0, 0, GetSize().x, GetSize().y, 4);

	//draw pic
	{
		//auto rect = this->GetClientRect();
		//wxBitmap bitmap(m_Avatar, wxBITMAP_TYPE_ANY);
		//dc.DrawBitmap(bitmap, wxPoint(rect.x + 4,rect.y + 3), false);
	}

	//draw name
	{
		auto rect = this->GetClientRect();
		auto tempName = m_Name;

// 		wxFont font(4, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
// 		dc.SetFont(font);
		wxBrush brush(wxColor("#FFFFFF"));
		wxPen pen(wxColor("#FFFFFF"));
		dc.SetBrush(brush);
		dc.SetPen(pen);

		wxPoint textPoint;
		wxSize nameSize;
		if (tempName.size() > 10)
		{
			tempName = tempName.SubString(0, 9);
			tempName = tempName + "...";
			nameSize = dc.GetTextExtent(tempName);
			textPoint = wxPoint(rect.width - 4 - nameSize.GetWidth(), rect.y + 5);
		}
		else
		{
			nameSize = dc.GetTextExtent(tempName);
			textPoint = wxPoint(rect.width - 11 - nameSize.GetWidth(), rect.y + 5);
		}
		
		dc.DrawText(m_Name, textPoint);
	}
}


BEGIN_EVENT_TABLE(AnkerLoginPanle, wxPanel)
//EVT_PAINT(AnkerLoginPanle::OnPaint)
EVT_LEFT_DOWN(AnkerLoginPanle::OnClickEvent)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerLoginPanle, wxPanel)
AnkerLoginPanle::AnkerLoginPanle(wxWindow* parent, wxWindowID id /*= wxID_ANY*/)
	:wxPanel(parent, id, wxDefaultPosition, wxSize(120,35))
{
	initUi();
}

AnkerLoginPanle::AnkerLoginPanle()
{

}

void AnkerLoginPanle::initUi()
{
	this->SetBackgroundColour(wxColor("#1D1E1E"));	
	wxBoxSizer* pMainVSizer = new wxBoxSizer(wxHORIZONTAL);

	wxString filename = "plater";

	m_avatarBtn = new wxBitmapButton(this, wxID_ANY, wxBitmap());
	m_name = new wxButton(this, wxID_ANY, "Sign In");
	m_avatarBtn->SetPosition(wxPoint(3,5));
	m_name->SetPosition(wxPoint(35,5));
	
	pMainVSizer->Add(m_avatarBtn);
	pMainVSizer->AddSpacer(10);
	pMainVSizer->Add(m_name);

	m_avatarBtn->Bind(wxEVT_BUTTON, [this](wxEvent& event) {
		wxCommandEvent evt(wxEVT_BUTTON, GetId());
		evt.SetEventObject(this);
		GetEventHandler()->ProcessEvent(evt);
		});

	m_name->Bind(wxEVT_BUTTON, [this](wxEvent& event) {
		wxCommandEvent evt(wxEVT_BUTTON, GetId());
		evt.SetEventObject(this);
		GetEventHandler()->ProcessEvent(evt);
		});

	SetSizer(pMainVSizer);	
	
}

void AnkerLoginPanle::OnClickEvent(wxMouseEvent& event)
{
	wxCommandEvent evt(wxEVT_BUTTON, GetId());
	evt.SetEventObject(this);
	GetEventHandler()->ProcessEvent(evt);
}
