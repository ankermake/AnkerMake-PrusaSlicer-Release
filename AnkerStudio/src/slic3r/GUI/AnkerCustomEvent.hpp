#ifndef _ANKER_CUSTOM_EVENT_HPP_
#define _ANKER_CUSTOM_EVENT_HPP_
#include "wx/wx.h"
class AnkerCustomEvent : public wxEvent
{
public:
	AnkerCustomEvent(wxEventType eventType,
		int winid,
		const wxPoint& post);

	wxPoint GetPoint();

	~AnkerCustomEvent() {};
	virtual wxEvent* Clone() const;


protected:

private:
	wxPoint m_pos;
};

wxDEFINE_EVENT(ANKER_CUSTOM_LOGIN, AnkerCustomEvent);
typedef void(wxEvtHandler::* AnkerCustomLOGINFunction)(AnkerCustomEvent&);
#define wxAnkerCustomLgoinEventHandler(func) wxEVENT_HANDLER_CAST(AnkerCustomLOGINFunction,func)

#define ANKER_EVT_CUSTOM_LOGIN(id, func)  \
 wx__DECLARE_EVT1(ANKER_CUSTOM_LOGIN, id, AnkerCustomLgoinEventHandler(func))

wxDEFINE_EVENT(ANKER_CUSTOM_ENTER, AnkerCustomEvent);
typedef void(wxEvtHandler::* AnkerCustomEnterFunction)(AnkerCustomEvent&);
#define wxAnkerCustomEnterEventHandler(func) wxEVENT_HANDLER_CAST(AnkerCustomEnterFunction,func)

#define ANKER_EVT_CUSTOM_ENTER(id, func)  \
 wx__DECLARE_EVT1(ANKER_CUSTOM_ENTER, id, AnkerCustomEnterEventHandler(func))


wxDEFINE_EVENT(ANKER_CUSTOM_CLICKED, AnkerCustomEvent);
typedef void(wxEvtHandler::* AnkerCustomFunction)(AnkerCustomEvent&);
#define wxAnkerCustomClickedEventHandler(func) wxEVENT_HANDLER_CAST(AnkerCustomFunction,func)

#define ANKER_EVT_CUSTOM_CLICK(id, func)  \
wx__DECLARE_EVT1(ANKER_CUSTOM_CLICKED, id, AnkerCustomClickedEventHandler(func))

#endif

