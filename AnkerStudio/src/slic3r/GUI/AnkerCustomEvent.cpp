#include "AnkerCustomEvent.hpp"

AnkerCustomEvent::AnkerCustomEvent(wxEventType eventType, int winid, const wxPoint& post)
	:wxEvent(winid, eventType)
{
	m_pos.x = post.x;
	m_pos.y = post.y;
}

wxPoint AnkerCustomEvent::GetPoint()
{
	return m_pos;
}
wxEvent* AnkerCustomEvent::Clone() const
{
	return new AnkerCustomEvent(*this);
}
