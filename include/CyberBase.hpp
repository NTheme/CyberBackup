/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:05 PM
 *  File    : CyberBase.hpp
 *  Project : Backup
\******************************************/

#pragma once

#include <filesystem>
#include <iostream>
#include <regex>

namespace nt {

namespace fs = std::filesystem;

class CyberBase {
 public:
  CyberBase() = default;

  virtual void process() const noexcept = 0;

 protected:
  enum class Parameter {
    CREATE_DESTINATION = 1,
    IGNORE_ERRORS = 2,
    SHOW_ERROR_STAT = 4,
    SHOW_BACKUP_STAT = 8,
    SILENT = 16,
    PROCESS = 32,
    REMOVE_BASE = 64,
    REMOVE_INSIDE_ONLY = 128,
    OVERRIDE_DESTINATION = 256,
  };

  std::string type;
  fs::path source;
  fs::path destination;
  int params = 0;

  static constexpr size_t MAX_STR = 75;
  static const char* DIR_NAME;
  static const char* SUM_NAME;
  static const std::regex timestamp_pattern;

  [[nodiscard]] std::string setStat(const fs::path& src, const fs::path& dst, const std::string& base = "") const;

  static void printInfo(const std::vector<std::pair<fs::path, fs::path>>& info, const std::string& title,
                        const std::string& empty);
  static void abort(int code, const std::string& msg, int params = 0, const std::string& base = "");
  static std::string processFSError(const fs::filesystem_error& error, int params = static_cast<int>(Parameter::REMOVE_BASE),
                                    const std::string& base = "");
  static std::string processErrnoError(const fs::filesystem_error& error, int params = static_cast<int>(Parameter::REMOVE_BASE),
                                       const std::string& base = "");
  static std::string preparePathOutput(const fs::path& path);

  static bool getParam(int value, Parameter param);

  template <typename... Args>
  static int nullifyParams(int value, Args... args);

  template <typename... Args>
  static int enableParams(int value, Args... args);

  template <typename Func, typename... Args>
  void executeCopy(Func&& func, std::vector<std::pair<fs::path, fs::path>>& success,
                   std::vector<std::pair<fs::path, fs::path>>& errors, const fs::path& entry, const fs::path& target_path,
                   const fs::path& dst, bool modified, Args&&... args) const;
};

template <typename... Args>
int CyberBase::nullifyParams(int value, Args... args) {
  return (value & ... & (value ^ static_cast<int>(args)));
}

template <typename... Args>
int CyberBase::enableParams(int value, Args... args) {
  return (value | ... | static_cast<int>(args));
}

template <typename Func, typename... Args>
void CyberBase::executeCopy(Func&& func, std::vector<std::pair<fs::path, fs::path>>& success,
                            std::vector<std::pair<fs::path, fs::path>>& errors, const fs::path& entry,
                            const fs::path& target_path, const fs::path& dst, bool modified, Args&&... args) const {
  if (!modified) {
    return;
  }

  if (getParam(params, Parameter::PROCESS)) {
    std::cout << preparePathOutput(entry) << "  -->  " << preparePathOutput(target_path) << std::endl;
  }

  try {
    func(std::forward<Args>(args)...);
    success.emplace_back(entry, target_path);
  } catch (const fs::filesystem_error& error) {
    auto what = processFSError(error, params, dst);
    errors.emplace_back(entry, what);
  }
}

}  // namespace nt
