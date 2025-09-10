#pragma once
#include <string>
#include <vector>

class DeviceVersionUtil
{
public:
    // tarVec < currentVec retrun true, others return false;
    static bool IsTargetLessCurrent(const std::string& tarVec, const std::string& currentVec);    
    // tarVec == currentVec retrun true, others return false;
    static bool Equal(const std::string& tarVec, const std::string& currentVec);
private:
    static std::vector<int> GainArrFromVersion(const std::string& vec);
};