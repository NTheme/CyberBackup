# **CyberBackupRestore**

A simple tool for backing up and restoring your work files.

## Build
1) `num_cores` is a number of your processor's cores (usually 16).
2) `type` can be `backup` or `restore`. Select what you need. 
3) Make sure you are in the root project folder.
4) Output files will be in `bin` folder.
  ```sh
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. -DMYTYPE=<type> -G"Unix Makefiles"
  cmake --build build --config Release -- -j <num_cores + 1>
  sudo cmake --install build
  ```

## Launch

### Backup
  ```
  ./bin/my_backup <flags>
  ```

### Restore
  ```
  ./bin/my_restore <flags>
  ```

Use `help` flag for every tool to get more information.

---

Author : ***NTheme*** - All rights reserved 
