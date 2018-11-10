# Gruut-Hana Merger Project

### Prerequisite
  - CMake (**3.12**)
  - vcpkg (**optional**)
    * 라이브러리 패키지 매니저(https://github.com/Microsoft/vcpkg)
  - clang-tidy
    * CLion -> Preference -> Inspection -> Clang-Tidy -> Option에서 `Use IDE setting`을 해제

### [Project Directory Structure](https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/)
  - build: This folder contains all object files, and is removed on a clean.
  - include: All project header files. All necessary third-party header files that do not exist under /usr/local/include are also placed here.
  - src: The application and only the application’s source files.
  - tests: All test code files.
  - scripts: script files. i.e., run-clang-tidy 
  