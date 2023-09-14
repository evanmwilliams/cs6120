#include "find_blocks.cpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include "../util/parse_program.hpp"

using json = nlohmann::json;
using instruction = json;

std::string remove_quotes(const std::string &s)
{
  auto str = s.substr(1, s.length() - 2);
  return s.substr(1, s.length() - 2);
}

json replace_args(json const &arg_list,
                  std::vector<std::pair<json, std::string>> const &idx_value_name)
{
  json args_as_vars = json::array();
  for (auto &lvn_id : arg_list)
  {
    int idx = std::stoi(lvn_id.dump());
    args_as_vars.push_back(idx_value_name[idx].second);
  }
  return args_as_vars;
}

json mangle_args(json const &arg_list, std::string &name)
{
  json args_as_vars = json::array();
  for (auto &lvn_id : arg_list)
  {
    if (lvn_id == name)
    {
      args_as_vars.push_back(std::string("_") + lvn_id.dump());
    }
    else
    {
      args_as_vars.push_back(lvn_id);
    }
  }
  return args_as_vars;
}

json make_lvn_val(json instr,
                  std::vector<std::pair<json, std::string>> const &idx_value_name,
                  std::unordered_map<std::string, int> const &variable_index,
                  std::unordered_map<std::string, std::string> const &var2mangled_name)
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
  else if (op == "const")
  {
    canonical_val["op"] = op;
    canonical_val["value"] = instr["value"];
  }
  else
  {
    canonical_val["op"] = op;
    std::vector<int> canonical_args;
    for (json const &arg : instr["args"])
    {

      std::string var = remove_quotes(arg.dump());

      // if this name was mangled, get the mangled one
      if (var2mangled_name.count(var) > 0)
      {
        var = var2mangled_name.at(var);
      }

      try
      {
        canonical_args.push_back(variable_index.at(var));
      }
      catch (std::exception &e)
      {
        // std::cout << e.what() << " KEY: " << arg << std::endl;
      }
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

std::unordered_map<std::string, int> get_def_count(json const &func)
{
  std::unordered_map<std::string, int> def_count;

  for (json const &func_args : func["args"])
  {
    def_count[remove_quotes(func_args["name"])]++;
  }

  for (json const &instr : func["instrs"])
  {
    if (!instr.contains("dest"))
    {
      continue;
    }

    def_count[remove_quotes(instr["dest"].dump())]++;
  }
  return def_count;
}

json lvn(const json &program)
{
  std::cout << "Made it to the top of the lvn function?" << std::endl;
  json lvn_program;
  lvn_program["functions"] = json::array();

  std::cout << "Made it to the function loop?" << std::endl;

  for (auto const &func : program["functions"])
  {
    std::unordered_map<std::string, int> var_def_count = get_def_count(func);
    std::unordered_map<std::string, std::string> var_to_curr_mangled;

    json curr_function;
    curr_function["name"] = func["name"];
    curr_function["args"] = func["args"];
    curr_function["instrs"] = json::array();
    std::vector<BasicBlock> basic_blocks = find_blocks(func);

    std::cout << "BASIC BLOCKS FOUND" << std::endl;
    // std::cout << "BASIC BLOCKs FOUND" << std::endl;

    // for(const json& arg : func["args"]){
    //     json arg_json;
    //     arg_json["op"] = "id";
    //     arg_json["args"] = json::array({arg["name"]});
    //     idx_value_name.push_back({arg_json, arg["name"]});
    //     //std::cout << "ADDING ARG: " << arg["name"] << std::endl;
    //     variable_index[arg["name"]] = idx_value_name.size()-1;
    // }

    for (BasicBlock &block : basic_blocks)
    {
      std::unordered_map<std::string, int> variable_index;      // x -> 0
      std::vector<std::pair<json, std::string>> idx_value_name; // idx: 0, ({const 4}, x)

      std::unordered_set<std::string> defined_vars;
      for (auto instr : block.instructions)
      {
        if (instr.contains("dest"))
        {
          defined_vars.insert(remove_quotes(instr["dest"].dump()));
        }
        else
        {
          if (instr.contains("args"))
          {
            for (auto arg : instr["args"])
            {
              // if the arg has not been defined, create a mapping from it to itself (i.e. x -> id x)
              if (defined_vars.count(remove_quotes(arg.dump())) == 0)
              {
                json instr;
                instr["op"] = "id";
                instr["args"] = json::array({arg});
                idx_value_name.push_back({instr, arg});
                variable_index[arg] = idx_value_name.size() - 1;
              }
            }
          }
        }
      }

      for (auto instr : block.instructions)
      {
        // std::cout << "INSTRUCTION: " << instr << std::endl;
        std::string op = instr.contains("op") ? instr["op"] : "label";
        if (op == "print")
        {
          std::string print_arg = remove_quotes(instr["args"][0].dump());
          instr["args"] = json::array({idx_value_name[variable_index[print_arg]].second});
          curr_function["instrs"].push_back(instr);
          continue;
        }
        else if (op == "ret" or op == "jmp" or op == "call" or op == "label" or op == "br")
        {
          curr_function["instrs"].push_back(instr);
          continue;
        }

        json lvn_ify_instr = make_lvn_val(instr, idx_value_name, variable_index, var_to_curr_mangled);

        int value_number = find_val(lvn_ify_instr, idx_value_name);

        // value not in the table
        if (value_number == -1)
        {
          std::string dest = remove_quotes(instr["dest"].dump());

          if (var_def_count[dest] > 1)
          {
            var_def_count[dest]--;
            std::string mangled = std::string(var_def_count[dest], '_') + dest; // 5*'_' + dest
            var_to_curr_mangled[dest] = mangled;
            instr["dest"] = mangled;
          }

          idx_value_name.push_back({lvn_ify_instr, instr["dest"]});
          variable_index[instr["dest"]] = idx_value_name.size() - 1;
          if (instr.contains("args"))
            instr["args"] = replace_args(lvn_ify_instr["args"], idx_value_name);
        }
        // value in the table
        else
        {
          // replace instr with found val
          variable_index[instr["dest"]] = value_number;
          auto &[instr_val, canonical_name] = idx_value_name[value_number];
          instr["op"] = "id";
          instr["args"] = json::array({canonical_name});
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