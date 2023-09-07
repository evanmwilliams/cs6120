#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

std::unordered_set<std::string> find_used_variables(json const &prog) {
  std::unordered_set<std::string> used;
  for (const auto &func : prog["functions"]) {
    // Loop through all instructions in each function
    for (const auto &instr : func["instrs"]) {
      if (!instr.contains("args"))
        continue;
      used.insert(instr["args"].begin(), instr["args"].end());
    }
  }
  return used;
}

int erase_unused_variables(json &prog,
                           std::unordered_set<std::string> const &used) {
  int erased_insns = 0;
  for (auto &func : prog["functions"]) {
    // Loop through all instructions in each function
    for (auto instr = func["instrs"].begin(); instr != func["instrs"].end();) {
      json insn = *instr;
      if (insn.contains("dest") && used.find(insn["dest"]) == used.end()) {
        instr = func["instrs"].erase(instr);
        erased_insns++;
      } else {
        instr++;
      }
    }
  }
  return erased_insns;
}

int main(int argc, char *argv[]) {
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
  json prog = json::parse(json_str);

  // Initialize the count of add instructions to zero
  int erased_insns = 1;
  while (erased_insns != 0) {
    std::unordered_set<std::string> used = find_used_variables(prog);
    erased_insns = erase_unused_variables(prog, used);
  }

  std::cout << prog << std::endl;

  return 0;
}