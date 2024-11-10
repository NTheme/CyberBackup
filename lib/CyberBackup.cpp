/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:08 PM
 *  File    : CyberBackup.cpp
 *  Project : Backup
\******************************************/

#include "../include/CyberBackup.hpp"

#include <fstream>
#include <list>
#include <ranges>

namespace nt {

CyberBackup::CyberBackup(int argc, const char** argv) {
  if (argc == 1) {
    abort(static_cast<int>(std::errc::invalid_argument), "Missing a backup type. Try 'my_backup help' for more information.");
  }

  type = argv[1];
  if (argc == 2 && type == "help") {
    std::cout << "Usage: my_backup [TYPE] [SOURCE] [DESTINATION] [OPTIONAL FLAGS]\n"
                 "A tool for creating a backup of your files and folders from SOURCE to DESTINATION in timestamp name folder.\n"
                 "\nTwo backup types are available: full and incremental.\n"
                 "  full             creates a full backup copy of the SOURCE\n"
                 "  incremental      creates a copy of the SOURCE with differences between current state and last full backup\n"
                 "\nOptions\n"
                 "  create       Create a backup folder if it does not exist\n"
                 "  full_info    Display a backup information after process\n"
                 "  error_info   Display info only about errors\n"
                 "  silent       Silent mode (do not show errors)\n"
                 "  process      Show progress status\n"
                 "  ignore       Continue backing up despite errors\n"
              << std::endl;
    std::exit(0);
  }

  if (argc > 2 && type == "help") {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Use just 'my_backup help' without any extra arguments for more information.");
  }

  if (argc >= 2 && type != "full" && type != "incremental") {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Wrong backup type. Now are supported: 'full', 'incremental'. Try 'my_backup help' for more information.");
  }

  if (argc == 2) {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Missing a path of the entity to backup. Try 'my_backup help' for more information.");
  }

  source = argv[2];
  if (argc == 3) {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Missing a destination path of the entity to backup. Try 'my_backup help' for more information.");
  }

  destination = argv[3];
  params = static_cast<int>(Parameter::REMOVE_BASE);
  for (int ind = 4; ind < argc; ++ind) {
    if (std::string(argv[ind]) == "create") {
      params = enableParams(params, Parameter::CREATE_DESTINATION);
    } else if (std::string(argv[ind]) == "ignore") {
      params = enableParams(params, Parameter::IGNORE_ERRORS);
    } else if (std::string(argv[ind]) == "full_info") {
      params = enableParams(params, Parameter::SHOW_ERROR_STAT, Parameter::SHOW_BACKUP_STAT);
    } else if (std::string(argv[ind]) == "error_info") {
      params = enableParams(params, Parameter::SHOW_ERROR_STAT);
    } else if (std::string(argv[ind]) == "silent") {
      params = enableParams(params, Parameter::SILENT);
    } else if (std::string(argv[ind]) == "process") {
      params = enableParams(params, Parameter::PROCESS);
    } else {
      abort(static_cast<int>(std::errc::invalid_argument),
            "Wrong operand. Did you mean 'create', 'full_info', 'error_info', 'silent', 'process' or 'ignore'?");
    }
  }
}

std::string CyberBackup::getTime() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream timestamp;
  timestamp << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S");
  return timestamp.str();
}

std::pair<fs::path, std::string> CyberBackup::findLastFull() const {
  std::vector<fs::path> backup_dirs;

  for (const auto& entry : fs::directory_iterator(destination)) {
    if (entry.is_directory()) {
      const auto& dir_name = entry.path().filename().string();
      if (std::regex_match(dir_name, timestamp_pattern)) {
        backup_dirs.push_back(entry.path());
      }
    }
  }

  std::sort(backup_dirs.begin(), backup_dirs.end());

  std::pair<fs::path, std::string> ret;
  for (const auto& backup_dir : std::ranges::reverse_view(backup_dirs)) {
    if (fs::exists(backup_dir / SUM_NAME)) {
      std::ifstream sum_file(backup_dir / SUM_NAME);
      std::string backup_type;

      if (!sum_file.is_open() || !(sum_file >> backup_type) || backup_type != "full" || !(sum_file >> ret.second) ||
          !std::regex_match(ret.second, timestamp_pattern)) {
        continue;
      }

      ret.first = backup_dir;
      break;
    }
  }

  if (ret.second.empty()) {
    abort(static_cast<int>(std::errc::no_such_file_or_directory),
          "Correct full backup has not been found, Try to create it first.");
  }

  return ret;
}

void CyberBackup::process() const noexcept {
  if (!fs::is_directory(source)) {
    abort(static_cast<int>(std::errc::no_such_file_or_directory),
          "Source entity (" + source.string() + ") does not exist. Please check source path.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_BASE));
  }

  auto timestamp = getTime();

  fs::path full_backup_norm;
  std::string full_backup_timestamp;
  if (type == "incremental") {
    std::tie(full_backup_norm, full_backup_timestamp) = findLastFull();
    full_backup_norm = fs::canonical(fs::absolute(full_backup_norm));
  } else {
    full_backup_norm = destination / timestamp;
    full_backup_timestamp = timestamp;
  }

  if (!fs::is_directory(destination)) {
    if (!getParam(params, Parameter::CREATE_DESTINATION)) {
      abort(static_cast<int>(std::errc::no_such_file_or_directory),
            "Destination folder (" + destination.string() + ") does not exist. Please check path or add 'create' operand.",
            nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_BASE));
    } else {
      try {
        fs::create_directories(destination / timestamp / DIR_NAME);
      } catch (const fs::filesystem_error& error) {
        processFSError(error, nullifyParams(params, Parameter::IGNORE_ERRORS), destination / timestamp);
      }
    }
  } else {
    if (!fs::is_directory(destination)) {
      abort(static_cast<int>(std::errc::not_a_directory),
            "Destination path (" + destination.string() + ") is not a folder. Please check destination path.",
            nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_BASE), destination);
    }

    if (fs::is_directory(destination / timestamp)) {
      abort(static_cast<int>(std::errc::directory_not_empty), "Backup " + timestamp + " already exists.",
            nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_BASE));
    } else {
      try {
        fs::create_directories(destination / timestamp / DIR_NAME);
      } catch (const fs::filesystem_error& error) {
        processFSError(error, nullifyParams(params, Parameter::IGNORE_ERRORS), destination / timestamp);
      }
    }
  }

  if (!getParam(params, Parameter::SILENT)) {
    std::cout << "Backing up from " << source << " to " << destination << "..." << std::endl;
  }

  if (getParam(params, Parameter::PROCESS)) {
    std::cout << std::endl << std::string(MAX_STR, '-') << "PROCESS" << std::string(MAX_STR, '-') << std::endl;
  }

  auto source_norm = fs::canonical(fs::absolute(source));
  auto destination_norm = fs::canonical(fs::absolute(destination)) / timestamp;

  std::list<fs::path> list;
  for (const auto& entry : fs::recursive_directory_iterator(source_norm)) {
    if (fs::is_directory(entry.path())) {
      list.push_front(entry.path());
    } else {
      list.push_back(entry.path());
    }
  }

  std::vector<std::pair<fs::path, fs::path>> success, errors;
  for (const auto& entry : list) {
    auto relative_path = entry.string().substr(source_norm.string().size() + 1, entry.string().size());
    auto target_path = destination_norm / DIR_NAME / relative_path;
    auto full_backup_path = full_backup_norm / DIR_NAME / relative_path;

    bool modified = false;

    if (fs::is_directory(entry)) {
      modified = !fs::is_directory(full_backup_path) || fs::last_write_time(entry) != fs::last_write_time(full_backup_path);
      executeCopy(static_cast<bool (*)(const fs::path&)>(fs::create_directories), success, errors, entry, target_path,
                  destination / timestamp, modified, target_path);

    } else if (fs::is_symlink(entry)) {
      modified = !fs::is_symlink(full_backup_path) || (fs::read_symlink(entry) != fs::read_symlink(full_backup_path));
      executeCopy(static_cast<void (*)(const fs::path&, const fs::path&, fs::copy_options)>(fs::copy), success, errors, entry,
                  target_path, destination / timestamp, modified, entry, target_path, fs::copy_options::copy_symlinks);

    } else {
      modified = !fs::exists(full_backup_path) || fs::last_write_time(entry) != fs::last_write_time(full_backup_path) ||
                 fs::file_size(entry) != fs::file_size(full_backup_path);
      executeCopy(static_cast<void (*)(const fs::path&, const fs::path&, fs::copy_options)>(fs::copy), success, errors, entry,
                  target_path, destination / timestamp, modified, entry, target_path, fs::copy_options::copy_symlinks);
    }
  }

  std::vector<size_t> err_stat;
  for (size_t ind = 0; ind < success.size(); ++ind) {
    if (!fs::is_symlink(success[ind].first)) {
      auto result = setStat(success[ind].first, success[ind].second, destination / timestamp);
      if (!result.empty()) {
        success[ind].second = result;
        err_stat.push_back(ind);
      }
    }
  }

  std::vector<std::pair<fs::path, fs::path>> success_new;
  size_t pos = 0;
  for (const auto& ind : err_stat) {
    while (pos < success.size() && pos < ind) {
      success_new.push_back(success[pos++]);
    }
    errors.push_back(success[pos++]);
  }
  success_new.insert(success_new.end(), success.begin() + static_cast<int>(pos), success.end());
  success = success_new;

  std::ofstream sum_file(destination_norm / SUM_NAME, std::ios::out);
  if (!sum_file.is_open()) {
    sum_file.close();
    abort(static_cast<int>(std::errc::no_such_file_or_directory), "Cannot create summary file. Try to recreate backup.",
          nullifyParams(params, Parameter::IGNORE_ERRORS));
  }
  sum_file << type << " " << full_backup_timestamp << "\n\n";

  if (type == "incremental") {
    for (const auto& entry : fs::recursive_directory_iterator(full_backup_norm / DIR_NAME)) {
      auto relative_path =
          entry.path().string().substr((full_backup_norm / DIR_NAME).string().size() + 1, entry.path().string().size());
      auto target_path = source_norm / relative_path;

      if (!fs::is_symlink(target_path) && !fs::exists(target_path)) {
        success.emplace_back(target_path, "DELETE");
        sum_file << entry.path() << '\n';
      }
    }
  }

  sum_file.close();

  if (getParam(params, Parameter::SHOW_ERROR_STAT)) {
    printInfo(errors, "ERROR INFORMATION", "Everything is OK!");
  }
  if (getParam(params, Parameter::SHOW_BACKUP_STAT)) {
    printInfo(success, "BACK UP INFORMATION", "No one entry has been backed up!");
  }

  if (!getParam(params, Parameter::SILENT)) {
    std::cout << "\n--> Backup operation completed!" << std::endl;
  }
}

}  // namespace nt
