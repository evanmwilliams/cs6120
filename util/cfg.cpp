#include "find_blocks.cpp"
#include <memory>

template <typename BB>
class CFGVisitor {
public:
  using BasicBlockPtr = BB *;

  std::vector<BasicBlockPtr> blocks;
  std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> successors;
  std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> predecessors;

  CFGVisitor(std::vector<BB> &basic_blocks) {
    for (size_t i = 0; i < basic_blocks.size(); i++) {
      blocks.push_back(&basic_blocks[i]);
      basic_blocks[i].id = i;
    }

    assert(blocks.size() == basic_blocks.size());

    for (size_t i = 0; i < blocks.size(); i++) {
      BasicBlockPtr block = (blocks)[i];

      if (block->isTerminal()) {
        const instruction &instr = block->getLastInstruction();

        if (instr.contains("op") and instr["op"] == "jmp" and
            instr.contains("labels")) {
          std::string target_label =
              instr["labels"][0]; // Assuming "target" is the key holding the
                                  // target label.
          for (auto &potential_target_block : basic_blocks) {
            if (potential_target_block.hasLabel(target_label)) {
              addEdge(block, &potential_target_block);
              break;
            }
          }
        } else if (instr.contains("op") and instr["op"] == "br") {
          std::string then_label = instr["labels"][0];
          std::string else_label = instr["labels"][1];

          bool then_found = false;
          bool else_found = false;

          for (auto &potential_target_block : basic_blocks) {
            if (then_found and else_found)
              break;

            if (potential_target_block.hasLabel(then_label)) {
              addEdge(block, &potential_target_block);
              then_found = true;
            } else if (potential_target_block.hasLabel(else_label)) {
              addEdge(block, &potential_target_block);
              else_found = true;
            }
          }
        }
      } else if (i < basic_blocks.size() - 1) { // Fall-through
        addEdge(block, &(basic_blocks[i + 1]));
      }
    }
  }

  void addEdge(BasicBlockPtr from, BasicBlockPtr to) {
    (*from).addSuccessor(to);
    (*to).addPredecessor(from);
    successors[from].push_back(to);
    predecessors[to].push_back(from);
  }

  std::vector<BasicBlockPtr> getSuccessors(BasicBlockPtr block) {
    return successors[block];
  }

  BasicBlockPtr getEntryBlock() { 
    for(auto block : blocks) {
      if(predecessors[block].size() == 0) {
        if (block != blocks[0]) std::cout << "Your first block is not entry block, perhaps investigate" <<std::endl;
        return block;
      }
    } 
    return blocks[0]; 
  }
};
