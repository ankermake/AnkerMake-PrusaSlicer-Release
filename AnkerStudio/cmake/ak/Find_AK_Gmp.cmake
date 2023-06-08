# This sets the following variables:
# gmp target

if(NOT TARGET gmp)
	if(NOT GMP_INSTALL_ROOT)
		set(GMP_INSTALL_ROOT $ENV{THIRD_PART_ROOT})
	endif()
message("===============${GMP_INSTALL_ROOT}==========")
	
	set(gmp_INCLUDE_DIRS ${GMP_INSTALL_ROOT}/usr/local/include/)
		set(gmp_LIBRARIES_DEBUG "${GMP_INSTALL_ROOT}/usr/local/lib/libgmp-10.lib")
	    set(gmp_LIBRARIES_RELEASE "${GMP_INSTALL_ROOT}/usr/local/lib/libgmp-10.lib")
	    set(gmp_LOC_DEBUG "${GMP_INSTALL_ROOT}/usr/local/bin/libgmp-10.dll")
	    set(gmp_LOC_RELEASE "${GMP_INSTALL_ROOT}/usr/local/bin/libgmp-10.dll")	



	__test_import(gmp dll)
endif()
