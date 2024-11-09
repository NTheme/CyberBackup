/******************************************\
 *  Author  : NTheme - All rights reserved
 *  Created : 09 November 2024, 1:05 PM
 *  File    : CyberBase.cpp
 *  Project : Backup
\******************************************/

#include "../include/CyberBase.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

namespace nt {

const char* CyberBase::DIR_NAME = "data";
const char* CyberBase::SUM_NAME = "type.nt";
const std::regex CyberBase::timestamp_pattern(R"(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})");

void CyberBase::printInfo(const std::vector<std::pair<fs::path, fs::path>>& info, const std::string& title,
                          const std::string& empty) {
  auto format = std::string(MAX_STR - (title.size() - 7) / 2, '-');
  std::cout << std::endl << format << title << format << std::endl;

  if (!info.empty()) {
    for (const auto& [lhs, rhs] : info) {
      std::cout << preparePathOutput(lhs) << "  -->  " << preparePathOutput(rhs) << std::endl;
    }
  } else {
    std::cout << empty << std::endl;
  }
}

void CyberBase::abort(int code, const std::string& msg, int params, const std::string& base) {
  if (!getParam(params, Parameter::SILENT)) {
    std::cerr << msg << std::endl;
  }
  if (!getParam(params, Parameter::IGNORE_ERRORS)) {
    if (getParam(params, Parameter::REMOVE_BASE)) {
      try {
        fs::remove_all(base);
      } catch (const fs::filesystem_error&) {
      }
    }
    if (getParam(params, Parameter::REMOVE_INSIDE_ONLY)) {
      for (const auto& entry : fs::directory_iterator(base)) {
        try {
          fs::remove_all(entry.path());
        } catch (const fs::filesystem_error&) {
        }
      }
    }
    std::exit(code);
  }
}

std::string CyberBase::preparePathOutput(const fs::path& path) {
  size_t path_start = std::max(MAX_STR, path.string().size()) - MAX_STR;
  std::string path_formatted = path.string().substr(path_start, path.string().size());
  if (path_start != 0) {
    path_formatted = "..." + path_formatted.substr(3, path_formatted.size());
  }
  path_formatted += std::string(MAX_STR - path_formatted.size(), ' ');
  return path_formatted;
}

std::string CyberBase::setStat(const fs::path& src, const fs::path& dst, const std::string& base) const {
  std::string success;

  fs::filesystem_error error("WTF", src, dst, std::error_code(0, std::generic_category()));

  struct stat src_stat{};
  if (stat(src.c_str(), &src_stat) == 0) {
    if (chown(dst.c_str(), src_stat.st_uid, src_stat.st_gid) != 0) {
      success += processErrnoError(error, params, base);
      success += "  ";
    }

    if (chmod(dst.c_str(), src_stat.st_mode) != 0) {
      success += processErrnoError(error, params, base);
      success += "  ";
    }

    struct timespec times[2];
    times[0].tv_sec = src_stat.st_atime;
    times[0].tv_nsec = src_stat.st_atim.tv_nsec;

    times[1].tv_sec = src_stat.st_mtime;
    times[1].tv_nsec = src_stat.st_mtim.tv_nsec;

    if (utimensat(AT_FDCWD, dst.c_str(), times, 0) == -1) {
      success += processErrnoError(error, params, base);
      success += "  ";
    }
  }

  return success;
}

std::string CyberBase::processFSError(const fs::filesystem_error& error, int params, const std::string& base) {
  const auto& val = error.code().value();
  const auto& path1 = error.path1().string();
  const auto& path2 = error.path2().string();

  std::string what;
  switch (val) {
    case static_cast<int>(std::errc::permission_denied):
      what = "Cannot access '" + path1 + "' or '" + path2 + "'. Try to check permissions or launch my_backup as root.";
      break;
    case static_cast<int>(std::errc::no_such_file_or_directory):
      what = "Cannot find entry '" + path1 + "' or '" + path2 + "'. Try to check the path.";
      break;
    case static_cast<int>(std::errc::file_exists):
      what = "Entry '" + path2 + "' already exists.";
      break;
    case static_cast<int>(std::errc::not_a_directory):
      what = "Entry '" + path1 + "' is not a directory. Check if you selected correct entry or check the path.";
      break;
    case static_cast<int>(std::errc::is_a_directory):
      what = "Entry '" + path1 + "' is a directory. Ensure that you've selected correct entry or check the path.";
      break;
    case static_cast<int>(std::errc::directory_not_empty):
      what = "Directory '" + path1 + "' is not empty. Try to remove files from this path or select other directory.";
      break;
    case static_cast<int>(std::errc::no_space_on_device):
      what = "Not enough free space on the hard drive to complete the operation. Try to remove unnecessary files.";
      break;
    case static_cast<int>(std::errc::too_many_symbolic_link_levels):
      what = "Too long symbolic link level detected. Ensure that you don't have recursive dependencies on links.";
      break;
    case static_cast<int>(std::errc::read_only_file_system):
      what = "File system is in read-only mode. Check permissions.";
      break;
    case static_cast<int>(std::errc::device_or_resource_busy):
      what = "File system is busy. Try to wait or close unnecessary processes which use the system.";
      break;
    case static_cast<int>(std::errc::cross_device_link):
      what = "Attempt to create symlink between different file systems. Check if both drives has the same system.";
      break;
    case static_cast<int>(std::errc::operation_not_permitted):
      what = "Operation is not permitted. Try to check permissions to file or directory.";
      break;
    case static_cast<int>(std::errc::filename_too_long):
      what = "Filename '" + path1 + "' is too long. Try to rename file or directory.";
      break;
    case static_cast<int>(std::errc::file_too_large):
      what = "File '" + path1 + "' or '" + path2 + "' is too large. Try to process backup operation with different file.";
      break;
    case static_cast<int>(std::errc::resource_unavailable_try_again):
      what = "Resource is temporary unavailable. Try again later.";
      break;
    case static_cast<int>(std::errc::invalid_argument):
      what = "Broken backup. Try to use other or recreate backup.";
      break;
    default:
      what = error.code().message();
      break;
  }
  abort(val, what, params, base);

  return what;
}

std::string CyberBase::processErrnoError(const fs::filesystem_error& error, int params, const std::string& base) {
  const auto& path1 = error.path1().string();
  const auto& path2 = error.path2().string();

  std::string what;
  switch (errno) {
    case EPERM:
    case EACCES:
      what = "Cannot access '" + path1 + "' or '" + path2 + "'. Try to check permissions or launch my_backup as root.";
      break;
    case ENOENT:
    case EFAULT:
    case ENXIO:
      what = "Cannot find entry '" + path1 + "' or '" + path2 + +"'. Try to check the path.";
      break;
    case EIO:
      what = "Filesystem error. Check your drive.";
      break;
    case ELOOP:
      what = "Too long symbolic link level detected. Ensure that you don't have recursive dependencies on links.";
      break;
    case ENOSYS:
      what = "Operation is not supported by a system. Change your OS.";
      break;
    case ENOTTY:
      what = "Operation is not supported. Check your device.";
      break;
    case EBUSY:
    case EUSERS:
      what = "File system is busy. Try to wait or close unnecessary processes which use the system.";
      break;
    case ENAMETOOLONG:
      what = "Filename '" + path1 + "' or '" + path2 + "' is too long. Try to rename file or directory.";
      break;
    case ENOSPC:
      what = "Not enough free space on the hard drive to complete the operation. Try to remove unnecessary files.";
      break;
    case EROFS:
      what = "File system is in read-only mode. Check permissions.";
      break;
    default:
      what = strerror(errno);
      break;
  }
  abort(errno, what, params, base);

  return what;
}

bool CyberBase::getParam(int value, Parameter param) {
  return (value & static_cast<int>(param)) != 0;
}

}  // namespace nt
