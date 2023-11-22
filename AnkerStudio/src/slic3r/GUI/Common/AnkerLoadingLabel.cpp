#include "AnkerLoadingLabel.hpp"
#include <wx/univ/theme.h>
#include <wx/artprov.h>
#include "libslic3r/Utils.hpp"


wxDEFINE_EVENT(wxCUSTOMEVT_ANKER_LOADING_LABEL_CLICKED, wxCommandEvent);

BEGIN_EVENT_TABLE(AnkerLoadingLabel, wxControl)
EVT_PAINT(AnkerLoadingLabel::OnPaint)
EVT_LEFT_DOWN(AnkerLoadingLabel::OnPressed)
EVT_LEFT_DCLICK(AnkerLoadingLabel::OnDClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(AnkerLoadingLabel, wxControl)

AnkerLoadingLabel::AnkerLoadingLabel(wxWindow* parent, const wxBitmap& bitmap,const wxString& bgColor)
	: wxControl(parent,wxID_ANY)
	, m_btnImg(bitmap)
	, m_strbgColor(bgColor)
	, m_labelStatus (Load_Nor)
{
	init();
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

AnkerLoadingLabel::AnkerLoadingLabel()
{

}

AnkerLoadingLabel::~AnkerLoadingLabel()
{

}

void AnkerLoadingLabel::startLoading()
{
	if (m_labelStatus != Load_Nor)
		return;

	m_loadingTimer->Start(300);
	m_labelStatus = Load_Ing;
	Refresh();
	Layout();

}

void AnkerLoadingLabel::stopLoading()
{
	readlStopLoading();
	timeCount = 0;
}

void AnkerLoadingLabel::OnPressed(wxMouseEvent& event)
{
	if (m_labelStatus != Load_Nor)
		return;

	startLoading();
	
	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_LOADING_LABEL_CLICKED);
	evt.SetEventObject(this);
	wxPostEvent(this, evt);
}

void AnkerLoadingLabel::OnDClick(wxMouseEvent& event)
{
	if (m_labelStatus != Load_Nor)
		return;

	startLoading();

	wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_ANKER_LOADING_LABEL_CLICKED);
	evt.SetEventObject(this);
	wxPostEvent(this, evt);
}

void AnkerLoadingLabel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	wxBitmap bitmap;
	if (m_labelStatus == Load_Nor)
	{
		bitmap = m_btnImg;
	}
	else
	{
		wxString imgName;
		imgName.Printf(wxT("preparingVideo%s.png"), (wxString::Format(wxT("%d"), m_imgIndex)));

		bitmap = wxImage(wxString::FromUTF8(Slic3r::var(imgName.ToStdString())), wxBITMAP_TYPE_PNG);

	}

	if(!m_strbgColor.IsEmpty())
	{
		wxColour  bgColor = wxColour(m_strbgColor);
		wxBrush brush(bgColor);
		dc.SetBrush(brush);
		dc.DrawRectangle(-1, -1, GetRect().GetWidth() + 4, GetRect().GetHeight() + 4);
	}

	wxImage image = bitmap.ConvertToImage();
	image.Rescale(16, 16, wxIMAGE_QUALITY_HIGH);
	wxBitmap scaledBitmap(image);

	dc.DrawBitmap(scaledBitmap, 0, 0, true);
}

void AnkerLoadingLabel::OnTimer(wxTimerEvent& event)
{
	if (event.GetId() == m_loadingTimer->GetId())
	{
		if (m_imgIndex >= 8)		
			m_imgIndex = 1;

		m_imgIndex++;	
		timeCount++;

		if (timeCount >= 30)
		{
			readlStopLoading();
			timeCount = 0;
		}

		Refresh();
	}
	
}

void AnkerLoadingLabel::init()
{
	m_loadingTimer = new wxTimer(this, wxID_ANY);
	m_loadingTimer->SetOwner(this, m_loadingTimer->GetId());
	
	Connect(m_loadingTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler(AnkerLoadingLabel::OnTimer), NULL, this);		
}

void AnkerLoadingLabel::readlStopLoading()
{
	m_loadingTimer->Stop();	

	m_imgIndex = 1;
	m_labelStatus = Load_Nor;

	Refresh();
}
