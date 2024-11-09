 #[==========================================[
  *  Author  : NTheme - All rights reserved
  *  Created : 03 November 2024, 11:24 PM
  *  File    : 
  *  Project : Backup
#]==========================================]

 cmake_minimum_required(VERSION 3.28)
 set(CMAKE_C_STANDARD 23)
 set(CMAKE_CXX_STANDARD 26)

 project(Backup)

 set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
 set(CMAKE_C_FLAGS_DEBUG "-std=c23 -g -Wall -Wextra -Werror -fsanitize=address,leak,undefined")
 set(CMAKE_C_FLAGS_RELEASE "-std=c23 -O3 -Wall -Wextra")
 set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address,leak,undefined -std=c++26 -g -Wall -Wextra -Werror")
 set(CMAKE_CXX_FLAGS_RELEASE "-std=c++26 -O3 -Wall -Wextra")

 file(GLOB_RECURSE HEADERS "*.h" "*.hpp")
 file(GLOB_RECURSE SOURCES "*.c" "*.cpp")
 list(FILTER HEADERS EXCLUDE REGEX ".*/cmake-build-debug/*")
 list(FILTER SOURCES EXCLUDE REGEX ".*/cmake-build-debug/*")

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

 add_executable(Backup ${HEADERS} ${SOURCES})

 install(TARGETS Backup DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
