#pragma once

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <boost/dynamic_bitset.hpp>
#include "parse_program.hpp"

using json = nlohmann::json;
using instruction = json;

class BasicBlock {
public:
  std::vector<json> instructions;
  std::vector<json> args;
  size_t id; // set in CFG visitor, but maybe we should just set it in constructor

  std::vector<BasicBlock *> predecessors;
  std::vector<BasicBlock *> successors;

  BasicBlock() = default;
  void addInstr(const json &insn) { instructions.push_back(insn); }
  virtual void addPredecessor(BasicBlock *bb) { predecessors.push_back(bb); }
  virtual void addSuccessor(BasicBlock *bb) { successors.push_back(bb); }

  std::string getLabel() const {
    if (instructions.size() == 0) {
      throw std::runtime_error("BasicBlock has no instructions");
    }
    auto instr = instructions[0];
    if (instr.contains("label")) {
      /*std::string label = remove_quotes(instr["label"].dump());
      if(label[0] != '.'){
        return std::string(".") + label;
      }*/
      return instr["label"];
    } else {
      throw std::runtime_error("I feel like you should not be asking for a label RN");
    }
  }

  size_t size() { return instructions.size(); }

  void clear() { instructions.clear(); }

  instruction getLastInstruction() const {
    return instructions[instructions.size() - 1];
  }

  bool isTerminal() const {
    if (instructions.size() == 0) {
      return false;
    }
    auto instr = getLastInstruction();
    return instr.contains("op") and
           (instr["op"] == "jmp" or instr["op"] == "br" or
            instr["op"] == "ret");
  }

  bool hasLabel(std::string label) const {
    for (auto &instr : instructions) {
      if (instr.contains("label") and instr["label"] == label) {
        return true;
      }
    }
    return false;
  }

  std::unordered_set<std::string> getDefs() {
    std::unordered_set<std::string> defs;
    for (auto &instr : instructions) {
      if (instr.contains("dest")) {
        defs.insert(instr["dest"]);
      }
    }
    return defs;
  }

  std::unordered_set<std::string> getKills() {
    std::unordered_set<std::string> kills;
    std::unordered_set<std::string> seen;

    for (auto it = instructions.rbegin(); it != instructions.rend(); it++) {
      if (it->contains("dest") && seen.find((*it)["dest"]) == seen.end()) {
        kills.insert((*it)["dest"]);
        seen.insert((*it)["dest"]);
      }
    }

    return kills;
  }
}; // BasicBlocks

class BasicBlockDom : public BasicBlock {
public:
// set of basic blocks that dominate current block
  boost::dynamic_bitset<> dom_by;
  // direct successors in dom tree
  std::vector<BasicBlockDom*> dom_succs;
  BasicBlockDom* idom;
  std::unordered_set<BasicBlockDom*> dfront;

  std::unordered_map<std::string, int> var_to_phi;

};

template <typename BBType>
std::vector<BBType> find_blocks(json const &func) {
  std::vector<BBType> basic_blocks;
  BBType current_block;

  for (auto const &instr : func["instrs"]) {
    if (instr.contains("op") and
        (instr["op"] == "jmp" or instr["op"] == "br")) {
      current_block.addInstr(instr);
      basic_blocks.push_back(current_block);
      current_block.clear();
    } else if (instr.contains("label")) {
      if (current_block.size() > 0) {
        basic_blocks.push_back(current_block);
      }
      current_block.clear();
      current_block.addInstr(instr);
    } else {
      current_block.addInstr(instr);
    }
  }

  if (current_block.size() > 0) {
    basic_blocks.push_back(current_block);
  }

  return basic_blocks;
}