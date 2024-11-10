 #[==========================================[
  *  Author  : NTheme - All rights reserved
  *  Created : 03 November 2024, 11:24 PM
  *  File    : 
  *  Project : BackupRestoreRestore
#]==========================================]

 cmake_minimum_required(VERSION 3.28)
 set(CMAKE_C_STANDARD 23)
 set(CMAKE_CXX_STANDARD 26)

 project(BackupRestore)

 set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 set(CMAKE_C_FLAGS_DEBUG "-std=c23 -g -Wall -Wextra -Werror -fsanitize=address,leak,undefined")
 set(CMAKE_C_FLAGS_RELEASE "-std=c23 -O3 -Wall -Wextra")
 set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address,leak,undefined -std=c++26 -g -Wall -Wextra -Werror")
 set(CMAKE_CXX_FLAGS_RELEASE "-std=c++26 -O3 -Wall -Wextra")

 file(GLOB_RECURSE HEADERS "*.h" "*.hpp")
 file(GLOB_RECURSE SOURCES "*.c" "*.cpp")
 list(FILTER HEADERS EXCLUDE REGEX ".*/build/*")
 list(FILTER SOURCES EXCLUDE REGEX ".*/build/*")

if(DEFINED MYTYPE)
  if(MYTYPE STREQUAL "backup")
    list(FILTER SOURCES EXCLUDE REGEX ".*/my_restore.cpp")
  elseif(MYTYPE STREQUAL "restore")
    list(FILTER SOURCES EXCLUDE REGEX ".*/my_backup.cpp")
  else()
    message(FATAL_ERROR "Invalid value for 'MYTYPE'. Expected 'backup' or 'restore', but got '${MYTYPE}'.")
  endif()
else()
  message(FATAL_ERROR "Argument '-DMYTYPE' was not passed. Exiting.")
endif()

 message("")
 message(STATUS "HEADERS list:")
 foreach (header ${HEADERS})
     message(STATUS ${header})
 endforeach ()
 message("")
 message(STATUS "SOURCES list:")
 foreach (source ${SOURCES})
     message(STATUS ${source})
 endforeach ()
 message("")

 add_executable(BackupRestore ${HEADERS} ${SOURCES})

 if(MYTYPE STREQUAL "backup")
   set_target_properties(BackupRestore PROPERTIES OUTPUT_NAME "my_backup")
 elseif(MYTYPE STREQUAL "restore")
   set_target_properties(BackupRestore PROPERTIES OUTPUT_NAME "my_restore")
endif()

install(TARGETS BackupRestore DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
