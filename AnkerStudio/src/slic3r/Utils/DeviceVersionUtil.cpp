#include "DeviceVersionUtil.hpp"
#include <sstream>
#include <algorithm>
#include <libslic3r/Utils.hpp>

// tarVec < currentVec retrun true, others return false;
// ex: V2.5.85_3.0.95 < V2.5.85_3.0.96
//     V2.5.85_3.0.96 < V2.5.85_3.0.96
bool DeviceVersionUtil::IsTargetLessCurrent(const std::string& tarVec, const std::string& currentVec) 
{    
    std::vector<int> tarVecArr = GainArrFromVersion(tarVec);
    std::vector<int> curVecArr = GainArrFromVersion(currentVec);
    size_t minCount = std::min(tarVecArr.size(), curVecArr.size());
    for (size_t i = 0; i < minCount; ++i) {
        if (curVecArr[i] > tarVecArr[i]) {
            ANKER_LOG_INFO << "tarVec: " << tarVec << " < currentVec: " << currentVec;
            return true;
        }
    }
    ANKER_LOG_INFO << "tarVec: " << tarVec << " >= currentVec: " << currentVec;
    return false;
}

std::vector<int> DeviceVersionUtil::GainArrFromVersion(const std::string& vec)
{
    std::vector<int> listData;
    std::string vecLowerCase = vec;
    std::transform(vecLowerCase.begin(), vecLowerCase.end(), vecLowerCase.begin(), ::tolower);
    std::replace(vecLowerCase.begin(), vecLowerCase.end(), 'v', ' ');
    std::istringstream iss(vecLowerCase);
    std::string token;
    while (std::getline(iss, token, '_')) {
        std::istringstream tokenStream(token);
        std::string pointStr;
        while (std::getline(tokenStream, pointStr, '.')) {
            if (pointStr.empty())
                continue;
            try {
                int value = std::stoi(pointStr);
                listData.push_back(value);
            }
            catch (...) {
                ANKER_LOG_ERROR << "pointStr: " << pointStr << " to int failed, " << vec;
            }
        }
    }
    return listData;
}
