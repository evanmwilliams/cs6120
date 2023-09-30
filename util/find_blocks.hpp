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
  std::string label;
  std::vector<json> instructions;
  std::vector<json> args;
  size_t id; // set in CFG visitor, but maybe we should just set it in constructor

  std::vector<BasicBlock *> predecessors;
  std::vector<BasicBlock *> successors;
  
  static int label_number;

  BasicBlock() = default;
  virtual void addInstr(const json &insn) {
    if (insn.contains("label")) {
      label = insn["label"];
      //std::cout << "Label generated: " << label << std::endl;

      return;
    } else if(label == "" && instructions.size() == 0){
      // Make a label name
      label = "_" + std::to_string(label_number); 
      //std::cout << "Label generated: " << label << std::endl;

      label_number++;
    }
    instructions.push_back(insn); 

  }
  virtual void addPredecessor(BasicBlock *bb) { predecessors.push_back(bb); }
  virtual void addSuccessor(BasicBlock *bb) { successors.push_back(bb); }

  std::string getLabel() const {
    /*if (instructions.size() == 0) {
      throw std::runtime_error("BasicBlock has no instructions");
    }*/
    return label;
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
    //std::cout << "looking for " << label << " in " << this->label << std::endl;
    return this->label == label;
    /*for (auto &instr : instructions) {
      if (instr.contains("label") and instr["label"] == label) {
        return true;
      }
    }
    return false;*/
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

int BasicBlock::label_number = 0;

struct PhiNode {
  std::string dest;    // x14
  std::string original_name; // x
  std::vector<std::string> args;
  std::vector<std::string> labels;

  PhiNode(std::string dest, 
          std::string original_name,
          std::vector<std::string> args, 
          std::vector<std::string> labels) : 
          dest(dest), original_name(original_name), args(args), labels(labels) {}

  void updatePhiArg(const std::string& phi_arg, const std::string& from_label ){
    args[phi_index_update] = phi_arg;
    labels[phi_index_update] = from_label;
    phi_index_update++;
  }
  json toInstruction() const {
    json j;
    j["op"] = "phi";
    j["dest"] = dest;
    j["args"] = args;
    j["labels"] = labels;
    return j;
  }

private:
  int phi_index_update = 0;

};

class BasicBlockDom : public BasicBlock {
public:
// set of basic blocks that dominate current block
  boost::dynamic_bitset<> dom_by;
  // direct successors in dom tree
  std::vector<BasicBlockDom*> dom_succs;
  BasicBlockDom* idom;
  std::unordered_set<BasicBlockDom*> dfront;

  std::unordered_map<std::string, int> var_to_phi;

  std::vector<PhiNode> phi_nodes;

  void addInstr(const json &insn) override {
    if(insn.contains("op") && insn["op"] == "phi"){
      std::string dest = insn["dest"];
      std::string original_name = "";
      std::vector<std::string> args;
      std::vector<std::string> labels;
      for(auto& arg : insn["args"]){
        args.push_back(arg);
      }
      for(auto& label : insn["labels"]){
        labels.push_back(label);
      }
      phi_nodes.push_back(PhiNode(dest, original_name, args, labels));
      return;
    }
    BasicBlock::addInstr(insn);
  }

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
      current_block = BBType();
    } else if (instr.contains("label")) {
      if (current_block.label != "") {
        basic_blocks.push_back(current_block);
      }
      current_block = BBType();
      current_block.addInstr(instr);
    } else {
      current_block.addInstr(instr);
    }
  }

  if (current_block.label != "") {
    basic_blocks.push_back(current_block);
  }

  return basic_blocks;
}