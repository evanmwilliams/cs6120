#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

int main(int argc, char *argv[]) {
  // Check if the filename is provided as a command-line argument
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  // Read the filename from command-line argument
  std::string filename = argv[1];

  // Read the JSON file into a std::string
  std::ifstream ifs(filename);
  std::string json_str((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  // Parse the JSON string into a json object
  auto j = json::parse(json_str);

  // Initialize the count of add instructions to zero
  // std::unordered_set<std::string> used;

  // Loop through all functions
  for (const auto &func : j["functions"]) {
    // Loop through all instructions in each function
    for (const auto &instr : func["instrs"]) {
      // Check if the operation is 'add'
      std::cout << "instr" << std::endl;
    }
  }

  return 0;
}