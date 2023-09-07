#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include "../util/parse_program.hpp"
#include "find_blocks.cpp"


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

std::unordered_set<std::string> find_used_variables(BasicBlock const &bb) {
  std::unordered_set<std::string> used;
  for (const auto &instr : bb.instructions) {
      if (!instr.contains("args"))
        continue;
      used.insert(instr["args"].begin(), instr["args"].end());
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

// can totally template this function
int erase_unused_variables(BasicBlock &bb,
                           std::unordered_set<std::string> const &used) {
  int erased_insns = 0;
  auto& instructions = bb.instructions;
    // Loop through all instructions in each function
  for (auto instr = instructions.begin(); instr != instructions.end();) {
    json insn = *instr;
    if (insn.contains("dest") && used.find(insn["dest"]) == used.end()) {
      instr = instructions.erase(instr);
      erased_insns++;
    } else {
      instr++;
    }
  }
  return erased_insns;
}


void local_dce(const json& prog){
  json new_prog;
  new_prog["functions"] = json::array();

  for (auto const &func : prog["functions"]) {
    json curr_function;
    curr_function["name"] = func["name"];
    curr_function["instrs"] = json::array();

    std::vector<BasicBlock> basic_blocks = find_blocks(func);
    for(auto& bb : basic_blocks){
      int erased_insns = 1;
      while (erased_insns != 0) {
        std::unordered_set<std::string> used = find_used_variables(bb);
        erased_insns = erase_unused_variables(bb, used);
      }
    }

    for(auto const& bb : basic_blocks){
      for(auto const& insn : bb.instructions){
        curr_function["instrs"].push_back(insn);
      }
    }
    new_prog["functions"].push_back(curr_function);
  }

  std::cout << new_prog << std::endl;
}

void global_dce(json& prog){
  int erased_insns = 1;
  while (erased_insns != 0) {
    std::unordered_set<std::string> used = find_used_variables(prog);
    erased_insns = erase_unused_variables(prog, used);
  }

  std::cout << prog << std::endl;
}

int main(int argc, char *argv[]) {
  // Parse the JSON string into a json object
  json prog = cli_parse_program(argc, argv);
  std::cout << "success" << std::endl;

  if(std::strcmp(argv[1], "-g") == 0){
    global_dce(prog);
  } else {
    local_dce(prog);
  }

  return 0;
}