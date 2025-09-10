#pragma once

#include <queue>
#include <string>
#include <algorithm>
#include <iostream>

struct NozzleElement {
    std::string m_Nozzle;
    int m_seq;
    NozzleElement(const std::string& s, int n) : m_Nozzle(s), m_seq(n) {}
    NozzleElement(const NozzleElement& e) {
        m_Nozzle = e.m_Nozzle;
        m_seq = e.m_seq;
    }
};

class NozzlePriorityQueue {
public:
    // Add nozzle to queue
    void AddNozzle(const std::string& nozzle, int seq);

    // Delete nozzle from queue
    void RemoveNozzle(const std::string& nozzle);

    // Get the nozzle of higest priority
    NozzleElement GetTopElem()const;

    // Judge the current's machine whether is empty
    bool IsEmpty() const {
        return mNozzlePriorityQueue.empty();
    }

    void Clear() {
        while (!mNozzlePriorityQueue.empty()) {
            mNozzlePriorityQueue.pop();
        }
    }

public:
    NozzlePriorityQueue() = default;
    ~NozzlePriorityQueue() = default;

private:
    struct Compare {
        bool operator()(const NozzleElement& e1, const NozzleElement& e2) {
            // ensuring 0.4 is top priority
            if (e1.m_Nozzle == "0.4") return false;
            if (e2.m_Nozzle == "0.4") return true;
            // except 0.4, other's nozzles orderd by number
            return std::stof(e1.m_Nozzle) > std::stof(e2.m_Nozzle);
        }
    };
private:
    std::priority_queue<NozzleElement, std::vector<NozzleElement>, Compare> mNozzlePriorityQueue;
};