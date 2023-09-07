#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

json cli_parse_program(int argc, char *argv[]) {
  // Check if the filename is provided as a command-line argument
  std::string json_str;
  if (argc < 2) {
    std::string line;
    while (std::getline(std::cin, line)) {
      json_str += line;
    }
  } else {
    // Read the filename from command-line argument
    std::string filename = argv[1];

    // Read the JSON file into a std::string
    std::ifstream ifs(filename);
    json_str = std::string((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
  }

  // Parse the JSON string into a json object
  return json::parse(json_str);
}