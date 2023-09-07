#include "find_blocks.cpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include "../util/parse_program.hpp"

using json = nlohmann::json;
using instruction = json;

std::string remove_quotes(const std::string& s){
  return s.substr(1, s.length() - 2);
}

json replace_args(json const& arg_list, 
    std::vector<std::pair<json, std::string>> const &idx_value_name){
    json args_as_vars = json::array();
    for(auto& lvn_id : arg_list){
      int idx = std::stoi(lvn_id.dump());
      args_as_vars.push_back(idx_value_name[idx].second);
    }
    return args_as_vars;
}

json make_lvn_val(json instr,
                  std::vector<std::pair<json, std::string>> const &idx_value_name,
                  std::unordered_map<std::string, int> const &variable_index)
{
  std::string op = instr["op"];
  json canonical_val;
  if (op == "jmp" || op == "br")
  {
    // do nothing
  }
  else if (op == "call" or op == "print")
  {
    // do nothing
  }
  else if(op == "const"){
    canonical_val["op"] = op;
    canonical_val["value"] = instr["value"];
  }
  else
  {
    canonical_val["op"] = op;
    std::vector<int> canonical_args;
    for (json const &arg : instr["args"])
    {
      canonical_args.push_back(variable_index.at(remove_quotes(arg.dump())));
    }
    sort(canonical_args.begin(), canonical_args.end());
    canonical_val["args"] = canonical_args;
  }
  return canonical_val;
}

int find_val(json instr_as_lvn_val,
             std::vector<std::pair<json, std::string>> const &idx_value_name)
{
  for (int i = 0; i < idx_value_name.size(); i++)
  {
    auto &[cand_val, canonical_var] = idx_value_name[i];
    if (cand_val == instr_as_lvn_val)
    {
      return i;
    }
  }
  return -1; // not found
}

json lvn(const json &program)
{
  json lvn_program;
  lvn_program["functions"] = json::array();

  for (auto const &func : program["functions"]) {
    json curr_function;
    curr_function["name"] = func["name"];
    curr_function["instrs"] = json::array();
    std::vector<BasicBlock> basic_blocks = find_blocks(func);

    for (BasicBlock& block : basic_blocks) {
      std::unordered_map<std::string, int> variable_index;      // x -> 0
      std::vector<std::pair<json, std::string>> idx_value_name; // idx: 0, ({const 4}, x)
      for (auto instr : block.instructions)
      {
        std::string op = instr["op"];

        if (op == "print") {
          std::string print_arg = remove_quotes(instr["args"][0].dump());
          instr["args"] = json::array({idx_value_name[variable_index[print_arg]].second});
          curr_function["instrs"].push_back(instr);
          continue;
        } else if(op == "jump" or op == "call") {
          curr_function["instrs"].push_back(instr);
          continue;
        }

        json lvn_ify_instr = make_lvn_val(instr, idx_value_name, variable_index);
        int value_number = find_val(lvn_ify_instr, idx_value_name);

        // value not in the table
        if (value_number == -1)
        {
          // give this instr a fresh value number
          idx_value_name.push_back({lvn_ify_instr, instr["dest"]});
          variable_index[instr["dest"]] = idx_value_name.size() - 1;
          if(instr.contains("args"))
            instr["args"] = replace_args(lvn_ify_instr["args"], idx_value_name); 
        }
        // value in the table
        else
        {
          // replace instr with found val
          variable_index[instr["dest"]] = value_number;
          auto&[instr_val, canonical_name] = idx_value_name[value_number];
          instr["op"] = "id";
          instr["args"] = json::array({canonical_name});
          //std::cout << "MODIFIED INSN: " << instr << std::endl;
        }
        curr_function["instrs"].push_back(instr);
      }
    }
    lvn_program["functions"].push_back(curr_function);
  }
  return lvn_program;
}

int main(int argc, char *argv[])
{
  auto j = cli_parse_program(argc, argv);

  json lvn_j = lvn(j);

  std::cout << lvn_j << std::endl;
}