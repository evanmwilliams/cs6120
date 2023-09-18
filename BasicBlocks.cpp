#include "BasicBlocks.hpp"

BasicBlock::BasicBlock() = default;

void BasicBlock::addInstruction(instruction instr)
{
  instructions.push_back(instr);
}

size_t BasicBlock::size()
{
  return instructions.size();
}

void BasicBlock::clear()
{
  instructions.clear();
}

instruction BasicBlock::getLastInstruction() const
{
  return instructions[instructions.size() - 1];
}

bool BasicBlock::isTerminal() const
{
  if (instructions.size() == 0)
  {
    return false;
  }

  auto instr = getLastInstruction();
  return instr.contains("op") and (instr["op"] == "jmp" or instr["op"] == "br");
}

bool BasicBlock::hasLabel(std::string label) const
{
  for (auto &instr : instructions)
  {
    if (instr.contains("label") and instr["label"] == label)
    {
      return true;
    }
  }
  return false;
}

std::unordered_set<std::string> BasicBlock::getDefs()
{
  std::unordered_set<std::string> defs;
  for (auto &instr : instructions)
  {
    if (instr.contains("dest"))
    {
      defs.insert(instr["dest"]);
    }
  }
  return defs;
}

std::unordered_set<std::string> BasicBlock::getKills()
{
  std::unordered_set<std::string> kills;
  std::unordered_set<std::string> seen;

  for (auto it = instructions.rbegin(); it != instructions.rend(); it++)
  {
    if (it->contains("dest") && seen.find((*it)["dest"]) == seen.end())
    {
      kills.insert((*it)["dest"]);
      seen.insert((*it)["dest"]);
    }
  }

  return kills;
}

std::vector<BasicBlock> find_basic_blocks(json const &func)
{
  std::vector<BasicBlock> blocks;
  BasicBlock current_block;

  assert(func.contains("instrs"));

  for (auto const &instr : func["instrs"])
  {
    if (instr.contains("op") and (instr["op"] == "jmp" or instr["op"] == "br"))
    {
      current_block.addInstruction(instr);
      blocks.push_back(current_block);
      current_block.clear();
    }
    else if (instr.contains("label"))
    {
      blocks.push_back(current_block);
      current_block.clear();
      current_block.addInstruction(instr);
    }
    else
    {
      current_block.addInstruction(instr);
    }
  }

  if (current_block.size() > 0)
  {
    blocks.push_back(current_block);
  }

  return blocks;
}