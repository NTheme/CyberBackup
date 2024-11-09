/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 07 November 2024, 10:50 PM
 *  File    : main.cpp
 *  Project : Backup
\******************************************/

#include "include/CyberBackup.hpp"

signed main(int argc, const char** argv) {
  nt::CyberBackup(argc, argv).process();
  std::cout.flush();
  return 0;
}
