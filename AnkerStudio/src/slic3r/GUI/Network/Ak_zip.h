#ifndef ANKER_NETWORK_ZIP
#define ANKER_NETWORK_ZIP

#include <string>
#include "miniz/miniz.h"

class AkZip
{
public:
	AkZip();
	~AkZip();

	static std::string utf8ToLocal(const std::string& str);
	static std::string localToUtf8(const std::string& locStr);

	//static int compress_file(const std::string& sourceFilePath, const std::string& destinationFileName, int level = Z_DEFAULT_COMPRESSION);
	static bool zipFile(const std::string& sfullFileName, const std::string& zipFileName, const std::string &filename = "");
	static bool unzipFile(const std::string& zipFileName, const std::string& unzipFileName);

private:

};


#endif // !ANKER_NETWORK_ZIP
