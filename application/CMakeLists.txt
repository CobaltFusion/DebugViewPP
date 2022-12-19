add_subdirectory(CobaltFusion)
add_subdirectory(CobaltFusionTest)
add_subdirectory(DbgMsgSrc)
add_subdirectory(DebugViewpp)
add_subdirectory(DebugViewppLib)
add_subdirectory(DebugViewppTest)
add_subdirectory(DebugViewConsole)
add_subdirectory(GDIGraphicsPOC)
add_subdirectory(IndexedStorageLib)
add_subdirectory(Win32Lib)
add_subdirectory(Libraries EXCLUDE_FROM_ALL)

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ALL)
set(CPACK_PACKAGE_VERSION "${DEBUGVIEW_VERSION}")
set(CPACK_GENERATOR "ZIP")

# this include needs to be after all install() rules
include(CPack)
