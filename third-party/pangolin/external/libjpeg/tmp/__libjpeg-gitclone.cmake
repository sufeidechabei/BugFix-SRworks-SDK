if("bc8f8be222287fec977ec3f47a5cb065cceb2ee9" STREQUAL "")
  message(FATAL_ERROR "Tag for git checkout should not be empty.")
endif()

set(run 0)

if("D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitinfo.txt" IS_NEWER_THAN "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitclone-lastrun.txt")
  set(run 1)
endif()

if(NOT run)
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: 'D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E remove_directory "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: 'D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg'")
endif()

# try the clone 3 times incase there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "D:/Program Files/Git/cmd/git.exe" clone --origin "origin" "https://github.com/LuaDist/libjpeg.git" "__libjpeg"
    WORKING_DIRECTORY "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/LuaDist/libjpeg.git'")
endif()

execute_process(
  COMMAND "D:/Program Files/Git/cmd/git.exe" checkout bc8f8be222287fec977ec3f47a5cb065cceb2ee9
  WORKING_DIRECTORY "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'bc8f8be222287fec977ec3f47a5cb065cceb2ee9'")
endif()

execute_process(
  COMMAND "D:/Program Files/Git/cmd/git.exe" submodule init 
  WORKING_DIRECTORY "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to init submodules in: 'D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg'")
endif()

execute_process(
  COMMAND "D:/Program Files/Git/cmd/git.exe" submodule update --recursive 
  WORKING_DIRECTORY "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: 'D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitinfo.txt"
    "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitclone-lastrun.txt"
  WORKING_DIRECTORY "D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: 'D:/PCL/ext_lib/Pangolin-master/vs2013/external/libjpeg/src/__libjpeg-stamp/__libjpeg-gitclone-lastrun.txt'")
endif()

