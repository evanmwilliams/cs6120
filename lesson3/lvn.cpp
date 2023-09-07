#include "find_blocks.cpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <unordered_map>

using json = nlohmann::json;
using instruction = json;

json make_lvn_val(json instr,
                  std::vector<std::pair<json, std::string>> const &num2val,
                  std::unordered_map<std::string, int> const &var2num) {
  std::string op = instr["op"];
  json canonical_val;
  if (op == "jmp" || op == "br") {
    // do nothing
  } else if (op == "call") {
    // do nothing
  } else {
    canonical_val["op"] = op;
    std::vector<int> canonical_args;
    for (json const &arg : instr["args"]) {
      canonical_args.push_back(var2num.at(arg.dump()));
    }
    sort(canonical_args.begin(), canonical_args.end());
    canonical_val["args"] = canonical_args;
  }
  return canonical_val;
}

int find_val(json instr_as_lvn_val,
             std::vector<std::pair<json, std::string>> const &num2val) {
  for (int i = 0; i < num2val.size(); i++) {
    auto &[cand_val, canonical_var] = num2val[i];
    if (cand_val == instr_as_lvn_val) {
      return i;
    }
  }
  return -1;
}

void lvn(json const &program) {
  std::vector<std::vector<instruction>> basic_blocks = find_blocks(program);

  for (auto block : basic_blocks) {
    std::unordered_map<std::string, int> var2num;      // x -> 0
    std::vector<std::pair<json, std::string>> num2val; // idx: 0, ({const 4}, x)
    for (auto instr : block) {
      json lvn_ify_instr = make_lvn_val(instr, num2val, var2num);
      if (lvn_ify_instr.empty()) {
        continue;
      }

      int value_number = find_val(lvn_ify_instr, num2val);

      if (value_number == -1) {
        // give this instr a fresh value number
        num2val.push_back({lvn_ify_instr, instr["dest"]});
        var2num[instr["dest"]] = num2val.size();
      } else {
        // replace instr with found val
        // point var2num[dest] = value_number
      }
    }
  }
}

int main(int argc, char *argv[]) { return 0; }