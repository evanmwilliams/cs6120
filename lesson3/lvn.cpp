#include <fstream>
#include <iostream>
#include <nlohmann/json.cpp>
#include <string>
#include <unordered_map>
#include "find_blocks.cpp"
#include <pair>

using instruction = json;

int find_val(json instr, std::vector<std::pair<json, std::string>> const &num2val)
{
  op = instr["op"];
  if (op == "jump" || op == "branch")
  { // the end of a basic block
    labels = instr["labels"] value = op + labels
  }
  else if (op == "call")
  { // not the end
    funciont = instr["functions"] value = op + functions
  }
  else
  {
    value = op + lvn_ify(instr["args"]);
  }

  for (int i = 0; i < num2val.size(); i++)
  {
    auto &[instr_val, canonical_var] = num2val[i];
    if (instr_val == value)
    {
      return i;
    }
  }
  return -1;
}

std::vector<instruction> lvn(json const &program)
{
  std::vector<std::vector<instruction>> basic_blocks = find_blocks(program);

  for (auto block : basic_blocks)
  {
    std::unordered_map<std::string, int> var2num;      // x -> 0
    std::vector<std::pair<json, std::string>> num2val; // idx: 0, ({const 4}, x)
    for (auto instr : block)
    {
      int value_number = find_val(instr);
      if (find_val(instr) != -1)
      {
        // replace instr with found val
        // point var2num[dest] = value_number
      }
      else
      {
        // give this instr a fresh value number
      }
    }
  }
}