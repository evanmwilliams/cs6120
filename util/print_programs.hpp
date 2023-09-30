#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include "find_blocks.hpp"

class ProgramPrinter {
  std::ostream &out;
  json prog;

public:
  ProgramPrinter(std::ostream &out) : out(out) {
    prog["functions"] = json::array();
  }

  void printFunction(const json& func, const std::vector<BasicBlockDom>& bbs){
    json function;
    function["name"] = func["name"];
    if(func.contains("args")){
      function["args"] = func["args"];
    }
    if(func.contains("type")){
      function["type"] = func["type"];
    }
    json instructions = json::array();
    for (auto &bb : bbs) {
      json label{ {"label", bb.getLabel()} };
      instructions.push_back(label);
      for (auto &phi : bb.phi_nodes) {
        instructions.push_back(phi.toInstruction());
      }

      for (auto &instr : bb.instructions) {
        instructions.push_back(instr);
      }
    }
    function["instrs"] = instructions;
    prog["functions"].push_back(function);
  }
  void print() { out << prog << std::endl; }

};
