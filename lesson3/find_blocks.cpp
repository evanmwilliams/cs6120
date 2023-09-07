#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;
using instruction = json;

std::vector<std::vector<instruction>> find_blocks(json const &program) {
  std::vector<std::vector<instruction>> basic_blocks;
  std::vector<instruction> current_block;

  for (auto const &func : program["functions"]) {
    for (auto const &instr : func["instrs"]) {
      std::string op = instr["op"];
      if (op == "jmp" or op == "br") {
        current_block.push_back(instr);
        basic_blocks.push_back(current_block);
        current_block.clear();
      } else if (instr.contains("label")) {
        basic_blocks.push_back(current_block);
        current_block.clear();
        current_block.push_back(instr);

      } else {
        current_block.push_back(instr);
      }
    }
  }
  if(current_block.size() > 0){
    basic_blocks.push_back(current_block);
  }
  
  std::cout << "blocks" << std::endl;

  for(auto& block : basic_blocks){
    std::cout << "NEW BLOCK: " << std::endl;
    for(auto& insn: block){
      std::cout << "\t" << insn << std::endl;
    }
  }

  return basic_blocks;
}