#include "find_blocks.cpp"

class CFG
{
public:
    using BasicBlockPtr = std::shared_ptr<BasicBlock>;

    std::vector<BasicBlockPtr> blocks;
    std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> successors;
    std::unordered_map<BasicBlockPtr, std::vector<BasicBlockPtr>> predecessors;

    CFG(const std::vector<BasicBlock> &basic_blocks)
    {
        for (size_t i = 0; i < basic_blocks.size(); i++)
        {
            blocks.push_back(std::make_shared<BasicBlock>(basic_blocks[i]));
        }

        for (size_t i = 0; i < basic_blocks.size(); i++)
        {
            auto block = std::make_shared<BasicBlock>(basic_blocks[i]);

            if (block->isTerminal())
            {
                const instruction &instr = block->getLastInstruction();

                if (instr.contains("op") and instr["op"] == "jmp")
                {
                    std::string target_label = instr["label"]; // Assuming "target" is the key holding the target label.
                    for (auto &potential_target_block : basic_blocks)
                    {
                        if (potential_target_block.hasLabel(target_label))
                        {
                            addEdge(block, std::make_shared<BasicBlock>(potential_target_block));
                            break;
                        }
                    }
                }
                else if (instr.contans("op") and instr["op"] == "br")
                {
                    std::string then_label = instr["then_target"];
                    std::string else_label = instr["else_target"];

                    for (auto &potential_target_block : basic_blocks)
                    {
                        if (potential_target_block.hasLabel(then_label))
                        {
                            addEdge(block, std::make_shared<BasicBlock>(potential_target_block));
                        }
                        else if (potential_target_block.hasLabel(else_label))
                        {
                            addEdge(block, std::make_shared<BasicBlock>(potential_target_block));
                        }
                    }
                }
            }
            else if (i < basic_blocks.size() - 1)
            { // Fall-through
                addEdge(block, std::make_shared<BasicBlock>(basic_blocks[i + 1]));
            }
        }
    }

    void addEdge(BasicBlockPtr from, BasicBlockPtr to)
    {
        successors[from].push_back(to);
        predecessors[to].push_back(from);
    }

    std::vector<BasicBlockPtr> getSuccessors(BasicBlockPtr block)
    {
        return successors[block];
    }
};
