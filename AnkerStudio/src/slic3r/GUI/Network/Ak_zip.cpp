#include "Ak_zip.h"
#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <boost/locale.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/locale/util.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <algorithm>
#include <locale>
#include <codecvt>


namespace loc = boost::locale;

#pragma pack(push, 1)
struct ZipHeader {
    uint32_t signature = 0x04034b50;  
    uint16_t version = 20;            
    uint16_t flags = 0;               
    uint16_t compression = 8;         
    uint16_t mod_time = 0;            
    uint16_t mod_date = 0;            
    uint32_t crc32 = 0;               
    uint32_t compressed_size = 0;     
    uint32_t uncompressed_size = 0;   
    uint16_t name_len = 0;           
    uint16_t extra_len = 0;           
};
#pragma pack(pop)

// Zip文件尾部信息
#pragma pack(push, 1)
struct ZipFooter {
    uint32_t signature = 0x06054b50;  
    uint16_t disk_num = 0;           
    uint16_t disk_start = 0;         
    uint16_t num_entries = 1;        
    uint16_t total_entries = 1;      
    uint32_t size = 0;              
    uint32_t offset = 0;             
    uint16_t comment_len = 0;         
};
#pragma pack(pop)

AkZip::AkZip()
{
}

AkZip::~AkZip()
{
}

std::string AkZip::utf8ToLocal(const std::string& str)
{
    std::string locStr = "";
#ifdef _WIN32
#elif defined(__APPLE__)
#else //Linux
#endif
    return boost::locale::conv::from_utf(str, boost::locale::util::get_system_locale());;
}

std::string AkZip::localToUtf8(const std::string& locStr)
{
    return boost::locale::conv::from_utf(locStr, "UTF-8");
}

//int AkZip::compress_file(const std::string& sourceFilePath, const std::string& destinationFileName, int level)
//{
//    
//    return 0;
//}

bool AkZip::zipFile(const std::string& sfullFileName, const std::string& zipFileName, const std::string& filename)
{
    if (sfullFileName.empty() || zipFileName.empty()) {
        std::cout << "File is empty. Source filename: " << sfullFileName << ", ZipFileName: " << zipFileName << std::endl;
        return false;
    }

    std::ifstream inputFile(sfullFileName, std::ios::binary);
    if (!inputFile) {
        std::cout << "Error opening input file: " << sfullFileName << std::endl;
        return false;
    }

    // Read input file text.
    inputFile.seekg(0, std::ios::end);
    size_t inputSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    std::vector<char> inputBuffer(inputSize);
    inputFile.read(inputBuffer.data(), inputSize);
    
    // Compress file.
    mz_zip_archive  zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_file(&zip, zipFileName.c_str(), 0)) {
        std::cout << "Error creating zip file: " << zipFileName << std::endl;
        return false;
    }

    mz_zip_writer_add_mem(&zip, filename.c_str(), inputBuffer.data(), inputBuffer.size(), MZ_BEST_COMPRESSION);
    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);

    // Close file.
    std::cout << "Compression successful!" << std::endl;
    inputFile.close();

    return true;
}

bool AkZip::unzipFile(const std::string& zipFileName, const std::string& unzipFileName)
{
    std::string localZipFileName = (zipFileName);
    std::string localUnzipFileName = (unzipFileName);
    if (localZipFileName.empty() || localUnzipFileName.empty()) {
        std::cerr << "File is empty. zipFileName: " << localZipFileName << ", unzipFileName: " << localUnzipFileName;
        return false;
    }

    mz_zip_archive archive;
    mz_bool status = 0;
    bool rc = false;
    memset(&archive, 0, sizeof(mz_zip_archive));

    // Open zip file.
    status = mz_zip_reader_init_file(&archive, localZipFileName.c_str(), 0);
    if (!status) {
        std::cerr << "Failed to open zip file " << localZipFileName << std::endl;
        rc = false;
        return false;
    }

    //  Iterate over the files in the zip file
    for (int i = 0; i < mz_zip_reader_get_num_files(&archive); i++) {
        mz_zip_archive_file_stat file_stat;

        // Get file info.
        if (mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            std::cout << "Failed to get file stat for index " << i << std::endl;
            rc = false;
            break;
        }

        // Output file name and size.
        std::cout << "File " << i + 1 << ": '" << file_stat.m_filename << "'" << " (" << file_stat.m_uncomp_size << " bytes)\n";

        // Unzip file.
        if (mz_zip_reader_is_file_a_directory(&archive, i)) {

            // If is dir.
            continue;
        }
        else {
            // If is file.
            void* p = malloc(file_stat.m_uncomp_size);
            if (p == NULL) {
                std::cout << "Failed to allocate memory for file " << file_stat.m_filename << std::endl;
                rc = false;
                break;
            }
            memset(p, 0, file_stat.m_uncomp_size);
            status = mz_zip_reader_extract_to_mem(&archive, i, p, file_stat.m_uncomp_size, 0);
            if (!status) {
                std::cout << "Failed to extract file " << file_stat.m_filename << std::endl;
                free(p);
                rc = false;
                break;
            }

            // Write the extracted data to a file.
            FILE* fp = fopen(file_stat.m_filename, "wb");
            if (!fp) {
                std::cout << "Failed to create file " << file_stat.m_filename << std::endl;
                free(p); 
                rc = false;
                break;
            }

            if (fwrite(p, file_stat.m_uncomp_size, 1, fp) != 1) {
                std::cout << "Failed to write file " << file_stat.m_filename;
                fclose(fp);
                free(p); 
                rc = false;
                break;
            }

            fclose(fp);
            free(p);
            rc = true;
        }

    }

    // Close zip file.
    mz_zip_reader_end(&archive);
    return rc;
}

