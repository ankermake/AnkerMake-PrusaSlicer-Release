//#pragma once
#ifndef _STRING_HELPER_CPP_
#define _STRING_HELPER_CPP_


#include "StringHelper.hpp"

namespace Slic3r {

	std::string join_strings_with_newlines(const std::vector<std::string>& strings)
	{
		std::string result;
		for (const auto& str : strings) {
			if (!result.empty()) {
				result += "\n";
			}
			result += str;
		}
		return result;
	}


	std::vector<std::string> split_string_by_carriage_return(const std::string& str)
	{
		std::vector<std::string> result;
		std::istringstream iss(str);
		std::string line;

		while (std::getline(iss, line, '\n'))
		{
			result.push_back(line);
		}

		return result;
	}



}
#endif
