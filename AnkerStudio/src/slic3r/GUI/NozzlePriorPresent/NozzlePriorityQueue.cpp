#include "NozzlePriorityQueue.hpp"


void NozzlePriorityQueue::AddNozzle(const std::string& nozzle,int seq) {
    mNozzlePriorityQueue.push({ nozzle,seq });
}

void NozzlePriorityQueue::RemoveNozzle(const std::string& nozzle) {
    while (!mNozzlePriorityQueue.empty()) {
        auto top = mNozzlePriorityQueue.top();
        mNozzlePriorityQueue.pop();
        if (top.m_Nozzle != nozzle) {
            mNozzlePriorityQueue.push(top);
        }
    }
}

NozzleElement NozzlePriorityQueue::GetTopElem()const {
    if (!mNozzlePriorityQueue.empty()) {
        return mNozzlePriorityQueue.top();
    }
    return NozzleElement("", -1);
}