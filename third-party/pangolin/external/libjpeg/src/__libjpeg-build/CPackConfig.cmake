# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


SET(CPACK_BINARY_7Z "")
SET(CPACK_BINARY_BUNDLE "")
SET(CPACK_BINARY_CYGWIN "")
SET(CPACK_BINARY_DEB "")
SET(CPACK_BINARY_DRAGNDROP "")
SET(CPACK_BINARY_IFW "")
SET(CPACK_BINARY_NSIS "")
SET(CPACK_BINARY_OSXX11 "")
SET(CPACK_BINARY_PACKAGEMAKER "")
SET(CPACK_BINARY_RPM "")
SET(CPACK_BINARY_STGZ "")
SET(CPACK_BINARY_TBZ2 "")
SET(CPACK_BINARY_TGZ "")
SET(CPACK_BINARY_TXZ "")
SET(CPACK_BINARY_TZ "")
SET(CPACK_BINARY_WIX "")
SET(CPACK_BINARY_ZIP "")
SET(CPACK_CMAKE_GENERATOR "Visual Studio 12 2013 Win64")
SET(CPACK_COMPONENTS_ALL "Runtime;Library;Header;Data;Documentation;Example;Other")
SET(CPACK_COMPONENTS_ALL_SET_BY_USER "TRUE")
SET(CPACK_COMPONENT_DATA_DESCRIPTION "Application data. Installed into share/libjpeg.")
SET(CPACK_COMPONENT_DATA_DISPLAY_NAME "libjpeg Data")
SET(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Application documentation. Installed into share/libjpeg/doc.")
SET(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "libjpeg Documentation")
SET(CPACK_COMPONENT_EXAMPLE_DESCRIPTION "Examples and their associated data. Installed into share/libjpeg/example.")
SET(CPACK_COMPONENT_EXAMPLE_DISPLAY_NAME "libjpeg Examples")
SET(CPACK_COMPONENT_HEADER_DESCRIPTION "Headers needed for development. Installed into include.")
SET(CPACK_COMPONENT_HEADER_DISPLAY_NAME "libjpeg Development Headers")
SET(CPACK_COMPONENT_LIBRARY_DESCRIPTION "Static and import libraries needed for development. Installed into lib or bin.")
SET(CPACK_COMPONENT_LIBRARY_DISPLAY_NAME "libjpeg Development Libraries")
SET(CPACK_COMPONENT_OTHER_DESCRIPTION "Other unspecified content. Installed into share/libjpeg/etc.")
SET(CPACK_COMPONENT_OTHER_DISPLAY_NAME "libjpeg Unspecified Content")
SET(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Executables and runtime libraries. Installed into bin.")
SET(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "libjpeg Runtime")
SET(CPACK_COMPONENT_TEST_DESCRIPTION "Tests and associated data. Installed into share/libjpeg/test.")
SET(CPACK_COMPONENT_TEST_DISPLAY_NAME "libjpeg Tests")
SET(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
SET(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
SET(CPACK_GENERATOR "ZIP")
SET(CPACK_INSTALL_CMAKE_PROJECTS "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-build;libjpeg;ALL;/")
SET(CPACK_INSTALL_PREFIX "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg")
SET(CPACK_MODULE_PATH "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg/cmake")
SET(CPACK_NSIS_DISPLAY_NAME "libjpeg 8.4.0")
SET(CPACK_NSIS_INSTALLER_ICON_CODE "")
SET(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
SET(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
SET(CPACK_NSIS_PACKAGE_NAME "libjpeg 8.4.0")
SET(CPACK_OUTPUT_CONFIG_FILE "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-build/CPackConfig.cmake")
SET(CPACK_PACKAGE_DEFAULT_LOCATION "/")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "C:/Program Files (x86)/CMake/share/cmake-3.5/Templates/CPack.GenericDescription.txt")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "libjpeg built using CMake")
SET(CPACK_PACKAGE_FILE_NAME "libjpeg-8.4.0-win64")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "libjpeg 8.4.0")
SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "libjpeg 8.4.0")
SET(CPACK_PACKAGE_NAME "libjpeg")
SET(CPACK_PACKAGE_RELOCATABLE "true")
SET(CPACK_PACKAGE_VENDOR "LuaDist")
SET(CPACK_PACKAGE_VERSION "8.4.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_RESOURCE_FILE_LICENSE "C:/Program Files (x86)/CMake/share/cmake-3.5/Templates/CPack.GenericLicense.txt")
SET(CPACK_RESOURCE_FILE_README "C:/Program Files (x86)/CMake/share/cmake-3.5/Templates/CPack.GenericDescription.txt")
SET(CPACK_RESOURCE_FILE_WELCOME "C:/Program Files (x86)/CMake/share/cmake-3.5/Templates/CPack.GenericWelcome.txt")
SET(CPACK_SET_DESTDIR "OFF")
SET(CPACK_SOURCE_7Z "ON")
SET(CPACK_SOURCE_CYGWIN "")
SET(CPACK_SOURCE_GENERATOR "7Z;ZIP")
SET(CPACK_SOURCE_OUTPUT_CONFIG_FILE "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-build/CPackSourceConfig.cmake")
SET(CPACK_SOURCE_TBZ2 "")
SET(CPACK_SOURCE_TGZ "")
SET(CPACK_SOURCE_TXZ "")
SET(CPACK_SOURCE_TZ "")
SET(CPACK_SOURCE_ZIP "ON")
SET(CPACK_STRIP_FILES "TRUE")
SET(CPACK_SYSTEM_NAME "win64")
SET(CPACK_TOPLEVEL_TAG "win64")
SET(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-build/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
