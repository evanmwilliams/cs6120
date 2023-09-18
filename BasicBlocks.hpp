#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;
using instruction = json;

class BasicBlock
{
private:
  std::vector<instruction> instructions;
  std::vector<BasicBlock *> successors;
  std::vector<BasicBlock *> predecessors;

public:
  BasicBlock();
  void addInstruction(instruction instr);
  size_t size();
  void clear();
  instruction getLastInstruction() const;
  bool isTerminal() const;
  bool hasLabel(std::string label) const;
  std::unordered_set<std::string> getDefs();
  std::unordered_set<std::string> getKills();
};

std::vector<BasicBlock> find_basic_blocks(json const &func);