#include <cstdlib>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.cpp"
#include "../util/cfg.cpp"
#include "dominators.hpp"
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::unordered_map<std::string, std::vector<BasicBlockDom>> collectVars(std::vector<BasicBlockDom> &bbs)
{
  std::unordered_map<std::string, std::vector<BasicBlockDom>> vars;
  for (auto &bb : bbs)
  {
    for (auto &instr : bb.instructions)
    {
      if (instr.contains("dest"))
      {
        vars[instr["dest"]].push_back(bb);
      }
    }
  }
  return vars;
}

void insert_phi_nodes(std::vector<BasicBlockDom> &bbs, CFGVisitor<BasicBlockDom> &cfg)
{
  getDominators(bbs, cfg.getEntryBlock());
  dominanceFrontier(bbs);
  auto vars = collectVars(bbs);
  for (auto &var : vars)
  {
    for (auto &d : var.second)
    {
      auto &df = d.dfront;
      for (auto &f : df)
      {
        // no phi node already
        auto block = *f;
        if (!block.instructions.empty() && (block.instructions[0].contains("op") && !block.instructions[0]["op"].contains("phi")))
        {
          json j;
          j["op"] = "phi";
          j["dest"] = var.first;
          std::vector<std::string> args;
          for (auto &p : block.predecessors)
          {
            args.push_back(var.first);
          }
          j["args"] = args;
          block.instructions.push_back(j);
          for (int i = block.instructions.size() - 1; i >= 1; i--)
          {
            block.instructions[i] = block.instructions[i - 1];
          }
          block.instructions[0] = j;
        }
        var.second.push_back(block);
      }
    }
  }
}

int main(int argc, char *argv[])
{
  const json prog = cli_parse_program(argc, argv);

  for (const json &func : prog["functions"])
  {
    std::vector<BasicBlockDom> bbs = find_blocks<BasicBlockDom>(func);
    CFGVisitor<BasicBlockDom> cfg = CFGVisitor<BasicBlockDom>(bbs);
    insert_phi_nodes(bbs, cfg);
  }

  return EXIT_SUCCESS;
}