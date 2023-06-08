#ifndef ANKER_MSG_TEXT_HPP
#define ANKER_MSG_TEXT_HPP

#include <utility>
#include <string>
#include "basetype.hpp"
#include "../slic3r/GUI/I18N.hpp"

namespace NetworkMsgText {
    struct NetworkMsg {
        std::string title = "";
        std::string context = "";
        int btnNum;
        int id;
        std::string btn1Text = "";
        std::string btn2Text = "";
    };

}



#endif // !Anker_Msg_TEXT_HPP
