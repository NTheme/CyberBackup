/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:09 PM
 *  File    : CyberRestore.hpp
 *  Project : Backup
\******************************************/

#pragma once

#include "CyberBase.hpp"

namespace nt {
class CyberRestore : protected CyberBase {
 public:
  CyberRestore(int argc, const char** argv);
  void process() const noexcept final;

 private:
};

}  // namespace nt
