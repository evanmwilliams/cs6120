#pragma once

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;
using instruction = json;

class BasicBlock
{
public:
  std::vector<json> instructions;
  BasicBlock() = default;
  void addInstr(const json &insn)
  {
    instructions.push_back(insn);
  }

  size_t size()
  {
    return instructions.size();
  }

  void clear()
  {
    instructions.clear();
  }

  instruction getLastInstruction() const
  {
    return instructions[instructions.size() - 1];
  }

  bool isTerminal() const
  {
    if (instructions.size() == 0)
    {
      return false;
    }
    auto instr = getLastInstruction();
    return instr.contains("op") and (instr["op"] == "jmp" or instr["op"] == "br");
  }

  bool hasLabel(std::string label) const
  {
    for (auto &instr : instructions)
    {
      if (instr.contains("label") and instr["label"] == label)
      {
        return true;
      }
    }
    return false;
  }

  std::unordered_set<std::string> getDefs()
  {
    std::unordered_set<std::string> defs;
    for (auto &instr : instructions)
    {
      if (instr.contains("dest"))
      {
        defs.insert(instr["dest"]);
      }
    }
    return defs;
  }

  std::unordered_set<std::string> getKills()
  {
    std::unordered_set<std::string> kills;
    std::unordered_set<std::string> seen;

    for (auto it = instructions.rbegin(); it != instructions.rend(); it++)
    {
      if (it->contains("dest") && seen.find((*it)["dest"]) == seen.end())
      {
        kills.insert((*it)["dest"]);
        seen.insert((*it)["dest"]);
      }
    }

    return kills;
  }
};

std::vector<BasicBlock> find_blocks(json const &func)
{
  std::vector<BasicBlock> basic_blocks;
  BasicBlock current_block;

  for (auto const &instr : func["instrs"])
  {
    if (instr.contains("op") and (instr["op"] == "jmp" or instr["op"] == "br"))
    {
      current_block.addInstr(instr);
      basic_blocks.push_back(current_block);
      current_block.clear();
    }
    else if (instr.contains("label"))
    {
      basic_blocks.push_back(current_block);
      current_block.clear();
      current_block.addInstr(instr);
    }
    else
    {
      current_block.addInstr(instr);
    }
  }

  if (current_block.size() > 0)
  {
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