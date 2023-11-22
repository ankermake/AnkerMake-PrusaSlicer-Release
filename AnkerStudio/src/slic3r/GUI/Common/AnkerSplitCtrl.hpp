#ifndef _ANKER_SPLIT_CTRL_H_
#define _ANKER_SPLIT_CTRL_H_

#include "wx/wx.h"


class AnkerSplitCtrl : public wxControl
{
public:
	AnkerSplitCtrl(wxWindow* parent);
	~AnkerSplitCtrl();

private:
	void initUI();
};

#endif // _ANKER_SPLIT_CTRL_H_

