#include "AnkerMerterialBoxBtn.hpp"
#include <wx/dcbuffer.h>
#include "libslic3r/Utils.hpp"
#include "AnkerGUIConfig.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_MATERIAL_BOX_CLICKED, wxCommandEvent);

AnkerMerterialBoxBtn::AnkerMerterialBoxBtn()
{

}


BEGIN_EVENT_TABLE(AnkerMerterialBoxBtn, wxControl)
EVT_PAINT(AnkerMerterialBoxBtn::OnPaint)
EVT_ENTER_WINDOW(AnkerMerterialBoxBtn::OnEnter)
EVT_LEAVE_WINDOW(AnkerMerterialBoxBtn::OnLeave)
EVT_LEFT_DOWN(AnkerMerterialBoxBtn::OnPressed)
EVT_LEFT_DCLICK(AnkerMerterialBoxBtn::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerMerterialBoxBtn, wxControl)

AnkerMerterialBoxBtn::AnkerMerterialBoxBtn(wxWindow* parent,
	wxColour bgColor,
	wxColour borderColor,	
	wxWindowID winid /*= wxID_ANY*/,
	const wxPoint& pos /*= wxDefaultPosition*/,
	const wxSize& size /*= wxDefaultSize*/)
{
	wxControl::Create(parent, winid, pos, size, wxBORDER_NONE, wxDefaultValidator);
	initUi(bgColor, borderColor);
}

AnkerMerterialBoxBtn::~AnkerMerterialBoxBtn()
{

}


void AnkerMerterialBoxBtn::setUnWorkStatus(bool isWork)
{
	m_isAbleWork = isWork;
	Update();
}

void AnkerMerterialBoxBtn::setMaterialName(const wxString& name)
{
	m_materialName = name;
	Update();
}


MaterialBoxStatus AnkerMerterialBoxBtn::getBtnStatus()
{
	return m_currentStatus;
}

void AnkerMerterialBoxBtn::setBtnStatus(const MaterialBoxStatus& status)
{	
	if (m_currentStatus == BOX_OFFLINE)
		return;

	m_currentStatus = status;
	Refresh();
}


//void AnkerMerterialBoxBtn::setDefaultStatus(const MaterialBoxStatus& status)
//{
//	m_currentStatus = status;
//	Update();
//}

void AnkerMerterialBoxBtn::setMaterialColor(const wxColour& color)
{
	m_materialColor = color;
	Update();
}

void AnkerMerterialBoxBtn::OnPressed(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE|| !m_isAbleWork)
		return;

	m_currentStatus = BOX_SELECT;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MATERIAL_BOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Update();
}

void AnkerMerterialBoxBtn::OnDClick(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || !m_isAbleWork)
		return;

	m_currentStatus = BOX_SELECT;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MATERIAL_BOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Update();
}

void AnkerMerterialBoxBtn::OnEnter(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_SELECT || !m_isAbleWork)
		return;
	
	m_currentStatus = BOX_HOVER;

	Refresh();
}

void AnkerMerterialBoxBtn::OnLeave(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_SELECT || !m_isAbleWork)
		return;

	m_currentStatus = BOX_NOR;

	Refresh();

}

void AnkerMerterialBoxBtn::initUi(const wxColor& bgColor,const wxColor& borderColor)
{
	m_materialName = "PLA+";
	m_materialColor = bgColor;	
	m_borderColor = borderColor;	
	m_currentStatus = BOX_NOR;	

	Update();
}

void AnkerMerterialBoxBtn::OnPaint(wxPaintEvent& event)
{
	//wxColor parantColor = GetParent()->GetBackgroundColour();
		
	wxColor spacingColor = wxColor(41, 42, 45);//parent color
	wxColor bgColor = m_materialColor;
	wxPaintDC dc(this);	

	dc.SetBrush(wxBrush(spacingColor));
	dc.SetPen(wxPen(spacingColor));
	dc.DrawRectangle(GetClientRect());
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (gc)
	{		

		if (m_currentStatus == BOX_NOR)
		{			
			if (m_materialName == "?")
			{
				wxColor borderColor = wxColor(93, 93, 96);
				wxColor bgColor = wxColor(52, 53, 56);
				wxColor textColor = wxColor(133, 134, 136);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 16);
				gc->DrawPath(contentPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 15);
				gc->DrawPath(borderPath);

				dc.SetBrush(wxBrush(textColor));
				dc.SetTextForeground(textColor);
				dc.SetPen(wxPen(textColor));
				dc.DrawText(m_materialName, 17, 14);
			}
			else if (m_materialName == "N/A")
			{
				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, 7, 7, naImage.GetWidth(), naImage.GetHeight());
			}
			else 
			{			
				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 15);			
				gc->DrawPath(contentPath);
			}
		}
		else if (m_currentStatus == BOX_HOVER)
		{
			if (m_materialName == "?")
			{
				wxColor borderColor = wxColor(93, 93, 96);
				wxColor bgColor = wxColor(52, 53, 56);
				wxColor textColor = wxColor(133, 134, 136);

				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(spacingColor);
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 19);
				gc->DrawPath(contentPath);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath borderExPath = gc->CreatePath();
				borderExPath.AddCircle(20, 21, 15);
				gc->DrawPath(borderExPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 14);
				gc->DrawPath(borderPath);

				dc.SetBrush(wxBrush(textColor));
				dc.SetTextForeground(textColor);
				dc.SetPen(wxPen(textColor));
				dc.DrawText(m_materialName, 17, 14);
			}
			else if (m_materialName == "N/A")
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 18);
				gc->DrawPath(borderPath);

				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, 7, 7, naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 18);
				gc->DrawPath(borderPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 15);
				gc->DrawPath(contentPath);
			}			
		}
		else if (m_currentStatus == BOX_SELECT)
		{
			if (m_materialName == "?")
			{
				wxColor borderColor = wxColor(93, 93, 96);
				wxColor bgColor = wxColor(52, 53, 56);
				wxColor textColor = wxColor(133, 134, 136);

				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(spacingColor);
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 19);
				gc->DrawPath(contentPath);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath borderExPath = gc->CreatePath();
				borderExPath.AddCircle(20, 21, 15);
				gc->DrawPath(borderExPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 14);
				gc->DrawPath(borderPath);

				dc.SetBrush(wxBrush(textColor));
				dc.SetTextForeground(textColor);
				dc.SetPen(wxPen(textColor));
				dc.DrawText(m_materialName, 17, 14);
			}
			else if (m_materialName == "N/A")
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 18);
				gc->DrawPath(borderPath);

				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, 7, 7, naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(20, 21, 20);
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(20, 21, 18);
				gc->DrawPath(borderPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(20, 21, 15);
				gc->DrawPath(contentPath);
			}
		}
		else if (m_currentStatus == BOX_OFFLINE)
		{
			wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
			naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
			gc->DrawBitmap(naImage, 7, 7, naImage.GetWidth(), naImage.GetHeight());
		}
		else
		{

		}		
		
		dc.SetBrush(wxBrush(m_borderColor));
		dc.SetTextForeground(m_borderColor);
		dc.SetPen(wxPen(m_borderColor));	
		dc.SetFont(ANKER_FONT_NO_1);		
		wxSize textSize = dc.GetTextExtent(m_materialName);
		int textWidth = textSize.GetWidth();
		int widgetWidth = GetRect().GetWidth();
		int materialPointX = (widgetWidth - textWidth) / 2;
		dc.DrawText(m_materialName, materialPointX, 42);

		if(!m_materialName.IsEmpty())
			dc.DrawText(m_materialName, materialPointX, 42);
		delete gc;
	}	
}