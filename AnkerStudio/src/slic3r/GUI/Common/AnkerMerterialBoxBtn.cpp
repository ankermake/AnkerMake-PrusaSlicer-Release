#include "AnkerMerterialBoxBtn.hpp"
#include <wx/dcbuffer.h>
#include "libslic3r/Utils.hpp"
#include "AnkerGUIConfig.hpp"
#include "../GUI_App.hpp"


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
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	initUi(bgColor, borderColor);
}

AnkerMerterialBoxBtn::~AnkerMerterialBoxBtn()
{

}

void AnkerMerterialBoxBtn::setMaterialName(const wxString& name)
{
	this->Freeze();
	m_alotMaterialName = name;
	SetToolTip(m_alotMaterialName);
	m_materialName = name;
	if(name.size() > 6)
		m_materialName = m_materialName.substr(0, 6) + "...";
	
	Refresh();
	this->Thaw();
}


MaterialBoxStatus AnkerMerterialBoxBtn::getBtnStatus()
{
	return m_currentStatus;
}

void AnkerMerterialBoxBtn::reSetBtnStatus(const MaterialBoxStatus& status)
{
	this->Freeze();
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_DISWORK)
		return;

	m_currentStatus = status;
	Refresh();
	this->Thaw();
}


void AnkerMerterialBoxBtn::setBtnStatus(const MaterialBoxStatus& status)
{		
	if (m_currentStatus == BOX_OFFLINE)
		return;

	m_currentStatus = status;
	Refresh();		
}

void AnkerMerterialBoxBtn::setMaterialColor(const wxColour& color)
{
	this->Freeze();
	m_materialColor = color;
	Refresh();
	this->Thaw();
}

void AnkerMerterialBoxBtn::OnPressed(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE|| m_currentStatus == BOX_DISWORK || 
		m_currentStatus == BOX_WORKING)
		return;

	m_currentStatus = BOX_SELECT;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MATERIAL_BOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Update();
}

void AnkerMerterialBoxBtn::OnDClick(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_DISWORK || 
		m_currentStatus == BOX_WORKING)
		return;

	m_currentStatus = BOX_SELECT;

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_MATERIAL_BOX_CLICKED);
	evt.SetEventObject(this);
	ProcessEvent(evt);

	Update();
}

void AnkerMerterialBoxBtn::OnEnter(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_SELECT ||
		m_currentStatus == BOX_DISWORK || m_currentStatus == BOX_WORKING)
		return;
	
	m_currentStatus = BOX_HOVER;

	Refresh();
}

void AnkerMerterialBoxBtn::OnLeave(wxMouseEvent& event)
{
	if (m_currentStatus == BOX_OFFLINE || m_currentStatus == BOX_SELECT ||
		m_currentStatus == BOX_DISWORK || m_currentStatus == BOX_WORKING)
		return;

	m_currentStatus = BOX_NOR;

	Refresh();

}

void AnkerMerterialBoxBtn::initUi(const wxColor& bgColor,const wxColor& borderColor)
{
	m_materialName = "?";
	m_materialColor = bgColor;	
	m_borderColor = borderColor;	
	m_currentStatus = BOX_NOR;	

	Update();
}

void AnkerMerterialBoxBtn::OnPaint(wxPaintEvent& event)
{			
	wxColor spacingColor = wxColor(41, 42, 45);//parent color
	wxColor bgColor = m_materialColor;
	//wxPaintDC dc(this);	

	wxBufferedPaintDC dc(this);
	PrepareDC(dc);

	dc.SetFont(ANKER_FONT_NO_1);
	dc.SetBrush(wxBrush(spacingColor));
	dc.SetPen(wxPen(spacingColor));
	
	auto widgetRect = GetClientRect();
	widgetRect.SetSize(AnkerSize(widgetRect.width+20,widgetRect.height+20));
	//dc.DrawRectangle(GetClientRect());
	dc.DrawRectangle(widgetRect);
	int rCenter = GetClientRect().width / 2;
	int naX = (GetClientRect().width-28) / 2;
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
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
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(16));
				gc->DrawPath(contentPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(borderPath);

				wxSize textSize = dc.GetTextExtent(m_materialName);
				int textWidth = textSize.GetWidth();
				int widgetWidth = GetRect().GetWidth();
				int materialPointX = (widgetWidth - textWidth) / 2;

				gc->SetBrush(wxBrush(textColor));
				gc->SetFont(ANKER_FONT_NO_1, textColor);
				gc->SetPen(wxPen(textColor));
				gc->DrawText(m_materialName, materialPointX, AnkerLength(14));
			}
			else if (m_materialName == "N/A")
			{
				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
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
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(spacingColor);
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(contentPath);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath borderExPath = gc->CreatePath();
				borderExPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(borderExPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(14));
				gc->DrawPath(borderPath);

				wxSize textSize = dc.GetTextExtent(m_materialName);
				int textWidth = textSize.GetWidth();
				int widgetWidth = GetRect().GetWidth();
				int materialPointX = (widgetWidth - textWidth) / 2;

				gc->SetBrush(wxBrush(textColor));
				gc->SetFont(ANKER_FONT_NO_1, textColor);
				gc->SetPen(wxPen(textColor));
				gc->DrawText(m_materialName, materialPointX, AnkerLength(14));
			}
			else if (m_materialName == "N/A")
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
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
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(spacingColor);
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(contentPath);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath borderExPath = gc->CreatePath();
				borderExPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(borderExPath);

				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(14));
				gc->DrawPath(borderPath);

				wxSize textSize = dc.GetTextExtent(m_materialName);
				int textWidth = textSize.GetWidth();
				int widgetWidth = GetRect().GetWidth();
				int materialPointX = (widgetWidth - textWidth) / 2;

				gc->SetBrush(wxBrush(textColor));
				gc->SetFont(ANKER_FONT_NO_1, textColor);
				gc->SetPen(wxPen(textColor));
				gc->DrawText(m_materialName, materialPointX, AnkerLength(14));

			}
			else if (m_materialName == "N/A")
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				gc->SetBrush(m_borderColor);
				gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(contentPath);
			}
		}
		else if (m_currentStatus == BOX_WORKING)
		{
			if (m_materialName == "?")
			{
				wxColor borderColor = wxColor(97, 212, 97);
				//wxColor borderColor = wxColor(93, 93, 96);
				wxColor bgColor = wxColor(52, 53, 56);
				wxColor textColor = wxColor(133, 134, 136);

				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(spacingColor);
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(contentPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderExPath = gc->CreatePath();
				borderExPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(14));
				gc->DrawPath(borderExPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(14));
				gc->DrawPath(borderPath);

				wxSize textSize = dc.GetTextExtent(m_materialName);
				int textWidth = textSize.GetWidth();
				int widgetWidth = GetRect().GetWidth();
				int materialPointX = (widgetWidth - textWidth) / 2;

				gc->SetBrush(wxBrush(textColor));
				gc->SetFont(ANKER_FONT_NO_1, textColor);
				gc->SetPen(wxPen(textColor));
				gc->DrawText(m_materialName, materialPointX, AnkerLength(14));
			}
			else if (m_materialName == "N/A")
			{
				wxColor borderColor = wxColor(97, 212, 97);
				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));
				//gc->SetBrush(m_borderColor);
				//gc->SetPen(wxPen(m_borderColor));
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				wxImage naImage(wxString::FromUTF8(Slic3r::var("NA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{
				wxColor borderColor = wxColor(97, 212, 97);
				gc->SetBrush(borderColor);
				gc->SetPen(wxPen(borderColor));				
				wxGraphicsPath spacingPath = gc->CreatePath();
				spacingPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(20));
				gc->DrawPath(spacingPath);

				gc->SetBrush(wxBrush(spacingColor));
				gc->SetPen(wxPen(spacingColor));				
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(18));
				gc->DrawPath(borderPath);

				gc->SetBrush(bgColor);
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(contentPath);
			}
		}
		else if (m_currentStatus == BOX_OFFLINE)
		{
			wxImage naImage(wxString::FromUTF8(Slic3r::var("unWorkNA.png")), wxBITMAP_TYPE_PNG);
			naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
			gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
		}
		else if (m_currentStatus == BOX_DISWORK)
		{
			if (m_materialName == "?")
			{
				wxColor borderColor = wxColor(93, 93, 96);
				wxColor bgColor = wxColor(41, 42, 45);
				//wxColor bgColor = wxColor(36, 37, 39);
				wxColor textColor = wxColor(133, 134, 136);				
				wxColor realTxColor = wxColor(textColor.GetRed(), textColor.GetGreen(), textColor.GetBlue(), 51);
				wxColor realBgColor = wxColor(bgColor.GetRed(), bgColor.GetGreen(), bgColor.GetBlue(), 51);

				gc->SetBrush(realTxColor);
				gc->SetPen(wxPen(realTxColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(16));
				gc->DrawPath(contentPath);
				
				gc->SetBrush(wxBrush(bgColor));
				gc->SetPen(wxPen(bgColor));
				wxGraphicsPath borderPath = gc->CreatePath();
				borderPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(borderPath);
				
				wxSize textSize = dc.GetTextExtent(m_materialName);
				int textWidth = textSize.GetWidth();
				int widgetWidth = GetRect().GetWidth();
				int materialPointX = (widgetWidth - textWidth) / 2;

				gc->SetBrush(wxBrush(realTxColor));
				gc->SetFont(ANKER_FONT_NO_1, realTxColor);
				gc->SetPen(wxPen(realTxColor));
				gc->DrawText(m_materialName, materialPointX, AnkerLength(14));
			}
			else if (m_materialName == "N/A")
			{
				wxImage naImage(wxString::FromUTF8(Slic3r::var("unWorkNA.png")), wxBITMAP_TYPE_PNG);
				naImage.Rescale(28, 28, wxIMAGE_QUALITY_HIGH);
				gc->DrawBitmap(naImage, AnkerLength(naX), AnkerLength(7), naImage.GetWidth(), naImage.GetHeight());
			}
			else
			{				
				wxColor realColor = wxColor(bgColor.GetRed(), bgColor.GetGreen(), bgColor.GetBlue(), 51);
				gc->SetBrush(realColor);
				gc->SetPen(wxPen(realColor));
				wxGraphicsPath contentPath = gc->CreatePath();
				contentPath.AddCircle(rCenter, AnkerLength(21), AnkerLength(15));
				gc->DrawPath(contentPath);
			}
		}
		else
		{

		}		

		if (m_currentStatus != BOX_DISWORK)
		{
			gc->SetBrush(wxBrush(m_borderColor));			
			gc->SetFont(ANKER_FONT_NO_1, m_borderColor);
			gc->SetPen(wxPen(m_borderColor));
		}
		else
		{			
			wxColor realColor = wxColor(m_borderColor.GetRed(), m_borderColor.GetGreen(), m_borderColor.GetBlue(), 51);			
			gc->SetBrush(wxBrush(realColor));			
			gc->SetFont(ANKER_FONT_NO_1, realColor);
			gc->SetPen(wxPen(realColor));
		}
				
		wxSize textSize = dc.GetTextExtent(m_materialName);
		int textWidth = textSize.GetWidth();
		int widgetWidth = GetClientRect().GetWidth();
		int materialPointX = (widgetWidth - textWidth) / 2;		

		if(!m_materialName.IsEmpty())
			gc->DrawText(m_materialName, materialPointX, AnkerLength(42));
		delete gc;
	}	
}