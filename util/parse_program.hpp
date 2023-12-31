#pragma once
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

std::string remove_quotes(const std::string &s) {
  auto str = s.substr(1, s.length() - 2);
  return s.substr(1, s.length() - 2);
}

json cli_parse_program(int argc, char *argv[]) {
  // Check if the filename is provided as a command-line argument
  std::string json_str;

  bool is_global = argc > 1 and std::strcmp(argv[1], "-g") == 0;
  bool has_file = argc == 3 or (!is_global && argc == 2);

  if (!has_file) {
    std::string line;
    while (std::getline(std::cin, line)) {
      // if line is a comment
      if(line[0] == '#'){
        continue;
      }
      json_str += line;
    }
  } else {
    // Read the filename from command-line argument
    int file_idx = is_global ? 2 : 1;
    std::string filename = argv[file_idx];
    std::ifstream ifs(filename);
    json_str = std::string((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
  }
  // Parse the JSON string into a json object
  return json::parse(json_str);
}