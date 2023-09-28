#include <cstdlib>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.cpp"
#include "../util/cfg.hpp"
#include "dominators.cpp"
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
        bool has_phi = false;
        for (auto instr : (*f).instructions)
        {
          if (instr.contains("op") and instr["op"] == "phi")
          {
            has_phi = true;
            break;
          }
        }
        if (!has_phi || !(*f).instructions[0].contains("op"))
        {
          json j;
          j["op"] = "phi";
          j["dest"] = var.first;
          std::vector<std::string> args;
          for (auto &p : (*f).predecessors)
          {
            args.push_back(var.first);
          }
          j["args"] = args;
          (*f).instructions.push_back(j);
          for (int i = (*f).instructions.size() - 1; i >= 1; i--)
          {
            (*f).instructions[i] = (*f).instructions[i - 1];
          }
          (*f).instructions[0] = j;
        }
        var.second.push_back((*f));
      }
    }
  }
}

int main(int argc, char *argv[])
{
  const json prog = cli_parse_program(argc, argv);

  for (const json &func : prog["functions"])
  {
    std::cout << func["name"] << ":" << std::endl;
    std::vector<BasicBlockDom> bbs = find_blocks<BasicBlockDom>(func);
    CFGVisitor<BasicBlockDom> cfg = CFGVisitor<BasicBlockDom>(bbs);
    insert_phi_nodes(bbs, cfg);
    for (auto bb : bbs)
    {
      for (auto instr : bb.instructions)
      {
        std::cout << instr << std::endl;
      }
    }
  }

  return EXIT_SUCCESS;
}