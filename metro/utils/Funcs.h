/**
 *  某些用到的额外函数，这里仅是一个splitString函数
 */
#pragma once
#include <string>
#include <vector>

namespace metro
{



inline std::vector<std::string> splitString(const std::string &s,
                                             const std::string &delimiter, 
                                             bool acceptEmptyString = false)
{
    std::vector<std::string> items;
    if(s.empty())
        return items;
    int last{0}, pos{0};

    while((pos = s.find(delimiter, last)) != std::string::npos)
    {
        if(pos > last || acceptEmptyString)
            items.push_back(s.substr(last, pos - last));
        last = pos + delimiter.size();
    }
    if(s.size() > last || acceptEmptyString)
        items.push_back(s.substr(last, pos - last));
    return items;
} 
}