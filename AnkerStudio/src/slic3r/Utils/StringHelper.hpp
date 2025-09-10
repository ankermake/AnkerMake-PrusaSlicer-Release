//#pragma once
#ifndef _STRING_HELPER_HPP_
#define _STRING_HELPER_HPP_

#include <string>
#include <vector>
#include <iostream>
#include <sstream>


namespace Slic3r {

	std::string join_strings_with_newlines(const std::vector<std::string>& strings);
	std::vector<std::string> split_string_by_carriage_return(const std::string& str);
}
#endif
