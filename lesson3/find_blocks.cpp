#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;
using instruction = json;

class BasicBlock {
public:
  std::vector<json> instructions;
  BasicBlock() = default;
  void addInstr(const json& insn){
    instructions.push_back(insn);
  }

  size_t size(){
    return instructions.size();
  }

  void clear(){
    instructions.clear();
  }
  
};

std::vector<BasicBlock> find_blocks(json const &func) {
  std::vector<BasicBlock> basic_blocks;
  BasicBlock current_block;

  for (auto const &instr : func["instrs"]) {
    if (instr.contains("op") and (instr["op"] == "jmp" or instr["op"] == "br")) {
      current_block.addInstr(instr);
      basic_blocks.push_back(current_block);
      current_block.clear();
    } else if (instr.contains("label")) {
      basic_blocks.push_back(current_block);
      current_block.clear();
      current_block.addInstr(instr);
    } else {
      current_block.addInstr(instr);
    }
  }
  
  if(current_block.size() > 0){
    basic_blocks.push_back(current_block);
  }
  
  /*std::cout << "blocks" << std::endl;

  for(auto& block : basic_blocks){
    std::cout << "NEW BLOCK: " << std::endl;
    for(auto& insn: block){
      std::cout << "\t" << insn << std::endl;
    }
  }*/

  return basic_blocks;
}