#ifndef _ANKER_CONFIG_HPP_
#define _ANKER_CONFIG_HPP_

#include <string>
#include <map>

namespace AnkerConfig
{
	std::string get_current_dir();

	std::map<std::string, std::string> read_ini_file(const std::string& filename);

	std::map<std::string, std::string> GetAnkerConfigIni();

	std::string getankerDomainUrl();

	std::vector<std::string> GetAnkerNetUrls();

	#define LoginWebUrl	AnkerConfig::getankerDomainUrl() + "passport-ct/?nocache=%s" + "#/login"
}
#endif 