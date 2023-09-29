#include <cstdlib>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.hpp"
#include "../util/cfg.hpp"
#include "dominators.hpp"
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "../util/print_programs.hpp"

using json = nlohmann::json;

struct PhiVariable {
  std::string var_name;
  int num;
  static std::unordered_map<std::string, int> highest_num;

  std::string toVariable(){
    return var_name + std::to_string(num);
  }

  static void updateHighestVar(std::string var_name, int num){
    highest_num[var_name] = num;
  }
  static int getHighestVar(std::string var_name){
    return highest_num[var_name];
  }

  static void clearHighestVars(){
    highest_num.clear();
  }
};



std::unordered_map<std::string, std::vector<BasicBlockDom*>> collectVars(std::vector<BasicBlockDom> &bbs)
{
  std::unordered_map<std::string, std::vector<BasicBlockDom*>> vars_to_bb;
  for (auto &bb : bbs)
  {
    for (auto &instr : bb.instructions)
    {
      if (instr.contains("dest"))
      {
        vars_to_bb[instr["dest"]].push_back(&bb);
      }
    }
  }
  return vars_to_bb;
}

void insert_phi_nodes(std::vector<BasicBlockDom> &bbs, CFGVisitor<BasicBlockDom> &cfg)
{
  getDominators(bbs, cfg.getEntryBlock());
  dominanceTree(bbs);
  dominanceFrontier(bbs);
  auto vars_to_bb = collectVars(bbs);
  for (auto &[var, def_blocks] : vars_to_bb)
  {
    for (int d = 0; d < def_blocks.size(); d++) //BasicBlockDom &d : var.second)
    {
      BasicBlockDom & bb_var = *def_blocks[d];
      for (auto frontier : bb_var.dfront)
      {
        // no phi node already
        bool has_phi = (*frontier).var_to_phi[var] > 0;

        if (!has_phi) //|| !(*frontier).instructions[0].contains("op"))
        {
          json j;
          j["op"] = "phi";
          j["dest"] = var;
          std::vector<std::string> args;
          std::vector<std::string> labels;
          for (auto &p : (*frontier).predecessors) {
            args.push_back(var);
            labels.push_back(var);
          }
          j["args"] = args;
          j["labels"] = labels;
          
          //std::cout << "going to phi node " << j << std::endl;
          // if first instruction is a label
          if((*frontier).instructions[0].contains("label")){
            (*frontier).instructions.insert((*frontier).instructions.begin() + 1, j);
            (*frontier).var_to_phi[var]++;
            //std::cout << "inserted phi node " << j << std::endl;
          } else {
            (*frontier).instructions.insert((*frontier).instructions.begin(), j);
            (*frontier).var_to_phi[var]++;
            //std::cout << "inserted phi node " << j << std::endl;
          }
          //(*frontier).instructions.insert((*frontier).instructions.begin(), j);
          //(*frontier).var_to_phi[var]++;
          //std::cout << "inserted phi node " << j << std::endl;
        }
        def_blocks.push_back(frontier);
      }
      //std::cout << "finished a thing " << std::endl;
    }
  }
}

void rename(BasicBlockDom& bb, 
            std::unordered_map<std::string, std::stack<PhiVariable>> &variable_names){
  //std::cout << "rename " << bb.id << std::endl;
  // rename step in SSA
  std::unordered_map<std::string, int> var_to_newly_added;

  for(auto &instr : bb.instructions){
    if(instr.contains("args") && instr.contains("op") && instr["op"] != "phi"){
      for(auto &arg : instr["args"]){
        arg = variable_names[arg].top().toVariable();
      }
    }

    if(instr.contains("dest")){
      std::string dest = remove_quotes(instr["dest"].dump());
      //std::cout << "DEST******* " << dest << std::endl;
      // give dest new name
      if(variable_names.count(dest) > 0){
        //std::cout << "pushing nonzero" << std::endl;
        variable_names[dest].push({dest, PhiVariable::getHighestVar(dest) + 1});
        PhiVariable::updateHighestVar(dest, PhiVariable::getHighestVar(dest) + 1);
      } else {
        //std::cout << "pushing zero" << std::endl;
        variable_names[dest].push({dest, 0});
        PhiVariable::updateHighestVar(dest, 0);
      }
      var_to_newly_added[dest]++;
      //std::cout << dest << " NOW NEEDS TO POP " << var_to_newly_added[dest] << std::endl;

      instr["dest"] = variable_names[dest].top().toVariable();

      //std::cout << "Here is the dest instruction:" << std::endl;
      //std::cout << instr << std::endl;
      //std::cout << "End of the dest instruction" << std::endl;
    }
  }
  //std::cout << "did renaming " << std::endl;

  for(auto& succ : bb.successors){
    //std::cout << "succ " << succ->id << std::endl;
    for(auto &phi : (*succ).instructions){
      if(phi.contains("op") && phi["op"] == "phi"){
        
        // loop phi = a, a, a
        // for the first "a", change it to "a1"
        // then BREAK
        for(int arg_int = 0; arg_int < phi["args"].size(); arg_int++){
          auto arg = remove_quotes(phi["args"][arg_int].dump());
          if(variable_names.count(arg) > 0){
            arg = variable_names[arg].top().toVariable();
            phi["args"][arg_int] = arg;
            phi["labels"][arg_int] = bb.getLabel();
            break;
          }
        }
      }
    }
  }
  // std::cout << "did succs, Now needs to RENAME " << bb.dom_succs.size() << " DOMSUCC BLOCKS" << std::endl;
  for(auto succ : bb.dom_succs){
    rename(*succ, variable_names);
  }
  //std::cout << "did dom succs " << std::endl;
  // pop all the names we just pushed onto the stack
  for(auto &[var, num] : var_to_newly_added){
    for(int i = 0; i < num; i++){
      //std::cout << var << " JUST POPPED " << std::endl;

      variable_names[var].pop();
    }
  }
  //std::cout << "did popping " << std::endl;

}

void toSSA(const json &prog){
  ProgramPrinter printer(std::cout);

  for (const json &func : prog["functions"])
  {
    std::vector<BasicBlockDom> bbs = find_blocks<BasicBlockDom>(func);
    CFGVisitor<BasicBlockDom> cfg = CFGVisitor<BasicBlockDom>(bbs);

    insert_phi_nodes(bbs, cfg);
    /*for (auto bb : bbs)
    {
      for (auto instr : bb.instructions)
      {
        std::cout << instr << std::endl;
      }
    }*/

    auto vars_to_bb = collectVars(bbs);

    std::unordered_map<std::string, std::stack<PhiVariable>> variable_names;
    for(auto &[var, defs] : vars_to_bb){
      variable_names[var].push({var, 0});

    }
    if(func.contains("args")){
      for(auto &arg : func["args"]){
        variable_names[arg].push({arg, 0});
      }
    }

    rename(bbs[0], variable_names);

    /*for (auto bb : bbs)
    {
      for (auto instr : bb.instructions)
      {
        std::cout << instr << std::endl;
      }
    }*/
    printer.printFunction(func, bbs);
    PhiVariable::clearHighestVars();
    printer.print();

  }

}

std::unordered_map<std::string, int> PhiVariable::highest_num;

int main(int argc, char *argv[])
{
  const json prog = cli_parse_program(argc, argv);
  toSSA(prog);
  
  return EXIT_SUCCESS;
}