#include "find_blocks.cpp"
#include "cfg.cpp"
#include <unordered_set>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include "../util/parse_program.hpp"

using json = nlohmann::json;
using instruction = json;

template <typename T>
std::unordered_set<T> setUnion(const std::unordered_set<T> &set1, const std::unordered_set<T> &set2)
{
  // Start with the first set
  std::unordered_set<T> result(set1.begin(), set1.end());

  // Insert elements from the second set
  result.insert(set2.begin(), set2.end());

  return result;
}

template <typename T>
std::unordered_set<T> setMinus(const std::unordered_set<T> &setA, const std::unordered_set<T> &setB)
{
  std::unordered_set<T> result;

  for (const T &elem : setA)
  {
    if (setB.find(elem) == setB.end())
    { // If the element is not in setB
      result.insert(elem);
    }
  }

  return result;
}

void reaching_definitions(CFG &cfg)
{
  std::unordered_map<CFG::BasicBlockPtr, std::unordered_set<std::string>> in;
  std::unordered_map<CFG::BasicBlockPtr, std::unordered_set<std::string>> out;
  std::queue<CFG::BasicBlockPtr> worklist;

  for (int i = 0; i < cfg.blocks.size(); i++)
  {
    // in.push_back(std::unordered_set<std::string>());
    worklist.push(cfg.blocks[i]);
  }

  while (worklist.size() > 0)
  {
    CFG::BasicBlockPtr block = worklist.front();
    worklist.pop();

    // in[b] = merge(out[p] for p in preds[b])
    std::unordered_set<std::string> new_in;

    for (CFG::BasicBlockPtr predecessor : cfg.predecessors[block])
    {
      in[predecessor] = setUnion(in[predecessor], out[predecessor]);
    }
    // out[b] = transfer(b, in[b])
    auto original_out = out[block];
    out[block] = setUnion(block->getDefs(), setMinus(in[block], block->getKills()));
    // if out[b] changed, add all successors of b to worklist
    if (out[block] != original_out)
    {
      for (CFG::BasicBlockPtr successor : cfg.successors[block])
      {
        worklist.push(successor);
      }
    }
  }

  for (int i = 0; i < cfg.blocks.size(); i++)
  {
    std::cout << "BLOCK " << i << std::endl;
    std::cout << "IN: ";
    for (auto in_var : in[cfg.blocks[i]])
    {
      std::cout << in_var << " ";
    }
    std::cout << std::endl;
    std::cout << "OUT: ";
    for (auto out_var : out[cfg.blocks[i]])
    {
      std::cout << out_var << " ";
    }
    std::cout << std::endl;
  }
}

int main(int argc, char *argv[])
{
  const json prog = cli_parse_program(argc, argv);

  for (const json &func : prog["functions"])
  {
    std::cout << "FUNCTION: " << func["name"] << std::endl;
    std::vector<BasicBlock> blocks = find_blocks(func);
    CFG cfg{blocks};
    reaching_definitions(cfg);
  }

  return EXIT_SUCCESS;
}