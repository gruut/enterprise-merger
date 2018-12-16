#pragma once

#include <fstream>
#include <iostream>
#include <string>

class FileIo {
public:
  static std::string file2str(const std::string &file_path) {
    std::string ret_str;

    std::ifstream ifs(file_path, std::ios::in);

    if (ifs && ifs.is_open()) {
      ifs.seekg(0, std::ios::end);
      ret_str.reserve((size_t)ifs.tellg());
      ifs.seekg(0, std::ios::beg);

      ret_str.assign((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());

      ifs.close();
    }

    return ret_str;
  }
};