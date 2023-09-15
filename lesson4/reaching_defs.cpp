#include "../util/parse_program.hpp"
#include "cfg.cpp"
#include "find_blocks.cpp"
#include <iostream>
#include <queue>
#include <stdlib.h>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;
using instruction = json;

template <typename T>
std::unordered_set<T> setUnion(const std::unordered_set<T> &set1,
                               const std::unordered_set<T> &set2) {
  // Start with the first set
  std::unordered_set<T> result(set1.begin(), set1.end());

  // Insert elements from the second set
  result.insert(set2.begin(), set2.end());

  return result;
}

template <typename T>
std::unordered_set<T> setMinus(const std::unordered_set<T> &setA,
                               const std::unordered_set<T> &setB) {
  std::unordered_set<T> result;

  for (const T &elem : setA) {
    if (setB.find(elem) == setB.end()) { // If the element is not in setB
      result.insert(elem);
    }
  }

  return result;
}

void reaching_definitions(const json &func) {
  // std::cout << "FUNCTION: " << func["name"] << std::endl;
  std::vector<BasicBlock> blocks = find_blocks(func);
  /*for (int i = 0; i < blocks.size(); i++) {
    std::cout << "Block " << i << std::endl;
    for (auto &insn : blocks[i].instructions) {
      std::cout << "\t" << insn << std::endl;
    }
  }*/

  CFG cfg{blocks};

  std::unordered_map<CFG::BasicBlockPtr, std::unordered_set<std::string>> in;
  std::unordered_map<CFG::BasicBlockPtr, std::unordered_set<std::string>> out;
  std::queue<CFG::BasicBlockPtr> worklist;

  std::unordered_set<std::string> starting_args;

  for (int i = 0; i < cfg.blocks.size(); i++) {
    worklist.push(cfg.blocks[i]);

    // add args if needed
    if (i == 0 && func.contains("args")) {
      for (const json &arg : func["args"]) {
        starting_args.insert(remove_quotes(arg["name"].dump()));
      }
    }
  }

  auto start_block = cfg.blocks[0];
  while (worklist.size() > 0) {
    CFG::BasicBlockPtr block = worklist.front();
    worklist.pop();
    // std::cout << "processing block with last insn "
    //          << block->getLastInstruction() << std::endl;

    // in[b] = merge(out[p] for p in preds[b])
    std::unordered_set<std::string> new_in;
    if (block == start_block) {
      new_in.insert(starting_args.begin(),
                    starting_args.end()); // we want to keep starting args
    }
    for (CFG::BasicBlockPtr predecessor : cfg.predecessors[block]) {
      // std::cout << "out[predecessor] has " << out[predecessor].size() << "
      // vars"
      //          << std::endl;
      new_in = setUnion(new_in, out[predecessor]);
    }
    in[block] = new_in;

    // out[b] = transfer(b, in[b])
    const std::unordered_set<std::string> original_out = out[block];
    auto new_out =
        setUnion(block->getDefs(), setMinus(in[block], block->getKills()));
    // if out[b] changed, add all successors of b to worklist
    out[block] = new_out;
    if (!(new_out == original_out)) {
      for (CFG::BasicBlockPtr successor : cfg.successors[block]) {
        worklist.push(successor);
      }
    }
  }

  for (int i = 0; i < cfg.blocks.size(); i++) {
    std::cout << "BLOCK " << i << std::endl;
    std::cout << "IN: ";
    for (auto in_var : in[cfg.blocks[i]]) {
      std::cout << in_var << " ";
    }
    std::cout << std::endl;
    std::cout << "OUT: ";
    for (auto out_var : out[cfg.blocks[i]]) {
      std::cout << out_var << " ";
    }
    std::cout << std::endl;
  }
}

int main(int argc, char *argv[]) {
  const json prog = cli_parse_program(argc, argv);

  for (const json &func : prog["functions"]) {
    reaching_definitions(func);
  }

  return EXIT_SUCCESS;
}