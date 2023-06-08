# This sets the following variables:
# gmp target

if(NOT TARGET Qhull)

	find_package(Qhull 7.2 REQUIRED)
	add_library(qhull INTERFACE)

	if(NOT QHULL_INSTALL_ROOT)
		set(QHULL_INSTALL_ROOT $ENV{THIRD_PART_ROOT})
	endif()
	message("===============${QHULL_INSTALL_ROOT}==========")
	
	set(Qhull_INCLUDE_DIRS ${QHULL_INSTALL_ROOT}/usr/local/include/)
	set(Qhull_LIBRARIES_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/lib/qhulld.lib")
	set(Qhull_LIBRARIES_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/lib/qhull.lib")
	set(Qhull_LOC_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/bin/qhulld.dll")
	set(Qhull_LOC_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/bin/qhull.dll")	

	__test_import(Qhull dll)


	set(Qhull_p_INCLUDE_DIRS ${QHULL_INSTALL_ROOT}/usr/local/include/)
	set(Qhull_p_LIBRARIES_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/lib/qhull_pd.lib")
	set(Qhull_p_LIBRARIES_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/lib/qhull_p.lib")
	set(Qhull_p_LOC_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/bin/qhull_pd.dll")
	set(Qhull_p_LOC_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/bin/qhull_p.dll")	

	__test_import(Qhull_p dll)


	set(Qhull_r_INCLUDE_DIRS ${QHULL_INSTALL_ROOT}/usr/local/include/)
	set(Qhull_r_LIBRARIES_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/lib/qhull_rd.lib")
	set(Qhull_r_LIBRARIES_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/lib/qhull_r.lib")
	set(Qhull_r_LOC_DEBUG "${QHULL_INSTALL_ROOT}/usr/local/bin/qhull_rd.dll")
	set(Qhull_r_LOC_RELEASE "${QHULL_INSTALL_ROOT}/usr/local/bin/qhull_r.dll")	

	__test_import(Qhull_r dll)

endif()
