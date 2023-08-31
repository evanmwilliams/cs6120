#include <iostream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
  // Check if the filename is provided as a command-line argument
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  // Read the filename from command-line argument
  std::string filename = argv[1];

  // Read the JSON file into a std::string
  std::ifstream ifs(filename);
  std::string json_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

  // Parse the JSON string into a json object
  auto j = json::parse(json_str);

  // Initialize the count of add instructions to zero
  int add_count = 0;

  // Loop through all functions
  for (const auto &func : j["functions"])
  {
    // Loop through all instructions in each function
    for (const auto &instr : func["instrs"])
    {
      // Check if the operation is 'add'
      if (instr.find("op") != instr.end() && instr["op"] == "add")
      {
        add_count++;
      }
    }
  }

  // Print the count of add instructions
  std::cout << "Number of add instructions: " << add_count << std::endl;

  return 0;
}