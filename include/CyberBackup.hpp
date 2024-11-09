/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:08 PM
 *  File    : CyberBackup.hpp
 *  Project : Backup
\******************************************/

#pragma once

#include "CyberBase.hpp"

namespace nt {

class CyberBackup : CyberBase {
 public:
  CyberBackup(int argc, const char** argv);
  void process() const noexcept final;

 private:
  [[nodiscard]] std::pair<fs::path, std::string> findLastFull() const;
  static std::string getTime();
};

}  // namespace nt
