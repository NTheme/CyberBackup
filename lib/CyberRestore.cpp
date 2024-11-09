/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:09 PM
 *  File    : CyberRestore.cpp
 *  Project : Backup
\******************************************/

#include "../include/CyberRestore.hpp"

#include <cstring>
#include <fstream>
#include <list>
#include <ranges>
#include <unordered_set>

namespace nt {

CyberRestore::CyberRestore(int argc, const char** argv) {
  if (argc == 1 && std::string(argv[1]) == "help") {
    std::cout << "Usage: my_restore [SOURCE] [DESTINATION] [OPTIONAL FLAGS]\n"
                 "A tool for restoring the backup of your files and folders from SOURCE timestamp name folder to DESTINATION"
                 " which was created by my_backup.\n"
                 "\nTwo backup types are available: full and incremental.\n"
                 "  full             creates a full backup copy of the SOURCE\n"
                 "  incremental      creates a copy of the SOURCE with differences between current state and last full backup\n"
                 "\nOptions\n"
                 "  create       Create a backup folder if it does not exist\n"
                 "  override     Remove files from DESTINATION or override them\n"
                 "  full_info    Display a backup information after process\n"
                 "  error_info   Display info only about errors\n"
                 "  silent       Silent mode (do not show errors)\n"
                 "  process      Show progress status\n"
                 "  ignore       Continue restoring despite errors\n"
              << std::endl;
    std::exit(0);
  }

  if (argc > 1 && type == "help") {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Use just 'my_restore help' without any extra arguments for more information.");
  }

  if (argc == 1) {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Missing a path of the entity to restore. Try 'my_restore help' for more information.");
  }

  source = argv[1];
  if (argc == 2) {
    abort(static_cast<int>(std::errc::invalid_argument),
          "Missing a destination path of the entity to restore. Try 'my_restore help' for more information.");
  }

  destination = argv[2];
  params = static_cast<int>(Parameter::REMOVE_INSIDE_ONLY);
  for (int ind = 3; ind < argc; ++ind) {
    if (std::string(argv[ind]) == "create") {
      params |= static_cast<int>(Parameter::CREATE_DESTINATION);
    } else if (std::string(argv[ind]) == "override") {
      params |= static_cast<int>(Parameter::OVERRIDE_DESTINATION);
    } else if (std::string(argv[ind]) == "ignore") {
      params |= static_cast<int>(Parameter::IGNORE_ERRORS);
    } else if (std::string(argv[ind]) == "full_info") {
      params |= static_cast<int>(Parameter::SHOW_BACKUP_STAT) | static_cast<int>(Parameter::SHOW_ERROR_STAT);
    } else if (std::string(argv[ind]) == "error_info") {
      params |= static_cast<int>(Parameter::SHOW_ERROR_STAT);
    } else if (std::string(argv[ind]) == "silent") {
      params |= static_cast<int>(Parameter::SILENT);
    } else if (std::string(argv[ind]) == "process") {
      params |= static_cast<int>(Parameter::PROCESS);
    } else {
      abort(static_cast<int>(std::errc::invalid_argument),
            "Wrong operand. Did you mean 'create, 'override', 'full_info', 'error_info', 'silent', 'process' or 'ignore'?");
    }
  }
}

void CyberRestore::process() const noexcept {
  if (!fs::is_directory(source / DIR_NAME)) {
    abort(static_cast<int>(std::errc::no_such_file_or_directory),
          "Source entity (" + source.string() + ") does not exist. Please check source path.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
  }

  std::ifstream sum_file(source / SUM_NAME);
  if (!sum_file.is_open()) {
    abort(static_cast<int>(std::errc::no_such_file_or_directory),
          "Cannot open summary file. Try to use other or recreate backup.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
  }

  std::string full_backup_timestamp;
  if (!(sum_file >> full_backup_timestamp)) {
    sum_file.close();
    abort(static_cast<int>(std::errc::invalid_argument), "Source file is empty.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
  }

  if (!(sum_file >> full_backup_timestamp) && !std::regex_match(full_backup_timestamp, timestamp_pattern)) {
    sum_file.close();
    abort(static_cast<int>(std::errc::invalid_argument), "Timestamp is corrupted.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
  }
  sum_file.close();

  fs::path full_backup_norm = source.parent_path() / full_backup_timestamp;
  if (!fs::is_directory(full_backup_norm / DIR_NAME)) {
    abort(static_cast<int>(std::errc::no_such_file_or_directory),
          "Full backup entity (" + full_backup_norm.string() + ") does not exist. Please use other backup.",
          nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
  }

  full_backup_norm = fs::canonical(fs::absolute(full_backup_norm));
  std::unordered_set<std::string> to_remove((std::istream_iterator<std::string>(sum_file)),
                                            std::istream_iterator<std::string>());

  if (!fs::is_directory(destination)) {
    if (!getParam(params, Parameter::CREATE_DESTINATION)) {
      abort(static_cast<int>(std::errc::no_such_file_or_directory),
            "Destination folder (" + destination.string() + ") does not exist. Please check path or add 'create' operand.",
            nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
    } else {
      try {
        fs::create_directories(destination);
      } catch (const fs::filesystem_error& error) {
        processFSError(error, nullifyParams(params, Parameter::IGNORE_ERRORS), destination);
      }
    }
  } else {
    if (!fs::is_directory(destination)) {
      abort(static_cast<int>(std::errc::not_a_directory),
            "Destination path (" + destination.string() + ") is not a folder. Please check destination path.",
            nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
    }

    for (const auto& entry : fs::directory_iterator(destination)) {
      if (getParam(params, Parameter::OVERRIDE_DESTINATION)) {
        try {
          fs::remove_all(entry.path());
        } catch (const fs::filesystem_error& error) {
          processFSError(error, nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY), destination);
        }
      } else {
        abort(static_cast<int>(std::errc::directory_not_empty),
              "Destination folder (" + destination.string() + ") is not empty. Please clear or save it first.",
              nullifyParams(params, Parameter::IGNORE_ERRORS, Parameter::REMOVE_INSIDE_ONLY));
      }
    }
  }

  if (!getParam(params, Parameter::SILENT)) {
    std::cout << "Restoring from " << source << " to " << destination << "..." << std::endl;
  }

  if (getParam(params, Parameter::PROCESS)) {
    std::cout << std::endl << std::string(MAX_STR, '-') << "PROCESS" << std::string(MAX_STR, '-') << std::endl;
  }

  auto source_norm = fs::canonical(fs::absolute(source));
  auto destination_norm = fs::canonical(fs::absolute(destination));

  std::list<fs::path> list_source, list_full_backup;
  for (const auto& entry : fs::recursive_directory_iterator(source_norm / DIR_NAME)) {
    if (fs::is_directory(entry.path())) {
      list_source.push_front(entry.path());
    } else {
      list_source.push_back(entry.path());
    }
  }
  for (const auto& entry : fs::recursive_directory_iterator(full_backup_norm / DIR_NAME)) {
    if (fs::is_directory(entry.path())) {
      list_full_backup.push_front(entry.path());
    } else {
      list_full_backup.push_back(entry.path());
    }
  }

  std::vector<std::pair<fs::path, fs::path>> success, errors;
  for (const auto& entry : list_full_backup) {
    auto relative_path =
        entry.string().substr(full_backup_norm.string().size() + std::strlen(DIR_NAME) + 2, entry.string().size());
    auto target_path = destination_norm / relative_path;
    auto source_path = source_norm / DIR_NAME / relative_path;
    auto full_backup_path = full_backup_norm / DIR_NAME / relative_path;

    if (fs::is_symlink(source_path) || fs::exists(source_path) || to_remove.count(full_backup_path) > 0) {
      continue;
    }

    if (fs::is_directory(entry)) {
      executeCopy(static_cast<bool (*)(const fs::path&)>(fs::create_directories), success, errors, entry, target_path,
                  destination, true, target_path);
    } else {
      executeCopy(static_cast<void (*)(const fs::path&, const fs::path&, fs::copy_options)>(fs::copy), success, errors, entry,
                  target_path, destination, true, entry, target_path, fs::copy_options::copy_symlinks);
    }
  }

  for (const auto& entry : list_source) {
    auto relative_path = entry.string().substr(source_norm.string().size() + std::strlen(DIR_NAME) + 2, entry.string().size());
    auto target_path = destination_norm / relative_path;

    if (fs::is_directory(entry)) {
      executeCopy(static_cast<bool (*)(const fs::path&)>(fs::create_directories), success, errors, entry, target_path,
                  destination, true, target_path);
    } else {
      executeCopy(static_cast<void (*)(const fs::path&, const fs::path&, fs::copy_options)>(fs::copy), success, errors, entry,
                  target_path, destination, true, entry, target_path, fs::copy_options::copy_symlinks);
    }
  }

  std::vector<size_t> err_stat;
  for (size_t ind = 0; ind < success.size(); ++ind) {
    if (!fs::is_symlink(success[ind].first)) {
      auto result = setStat(success[ind].first, success[ind].second, destination);
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

  if (getParam(params, Parameter::SHOW_ERROR_STAT)) {
    printInfo(errors, "ERROR INFORMATION", "Everything is OK!");
  }
  if (getParam(params, Parameter::SHOW_BACKUP_STAT)) {
    printInfo(success, "RESTORE INFORMATION", "No one entry has been backed up!");
  }

  if (!getParam(params, Parameter::SILENT)) {
    std::cout << "\n--> Restore operation completed!" << std::endl;
  }
}

}  // namespace nt
