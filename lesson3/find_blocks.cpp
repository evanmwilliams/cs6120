#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include "../../util/parse_program.hpp"

using instruction = json;

std::vector<std::vector<instruction>> find_blocks(json const &program)
{
  std::vector<std::vector<instruction>> basic_blocks;
  std::vector<instruction> current_block;

  for (auto const &func : program["functions"]) {
    for (auto const &instr : func["instrs"]) {
      std::string op = instr["op"];
      if(op == "jmp" or op == "branch"){
        current_block.push_back(op);
        basic_blocks.push_back(current_block);
        current_block.clear();
      } else if (op == "label"){
        basic_blocks.push_back(current_block);
        current_block.clear();
        current_block.push_back(op);
      } else {
        current_block.push_back(instr);
      }
    }
  }

  return basic_blocks;
}