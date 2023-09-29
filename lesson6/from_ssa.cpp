#include <cstdlib>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.hpp"
#include "../util/cfg.hpp"
#include "dominators.hpp"
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "../util/print_programs.hpp"

using json = nlohmann::json;

void fromSSA(const json &prog) {
  ProgramPrinter printer(std::cout);

  for (const json &func : prog["functions"])
  {
    std::vector<BasicBlockDom> bbs = find_blocks<BasicBlockDom>(func);
    CFGVisitor<BasicBlockDom> cfg = CFGVisitor<BasicBlockDom>(bbs);

    std::vector<BasicBlockDom> new_phi_bbs;

    // in each basic block, look for a phi node
    // if there is a phi node, then we need to insert a new basic block
    // before the current basic block
    for(auto bb_itr = bbs.begin(); bb_itr != bbs.end(); bb_itr++){
      auto& bb = *bb_itr;
      int newly_added_blocks = 0;
      for(auto &instruction : bb.instructions){
        if(instruction.contains("op") && instruction["op"] == "phi"){
          // insert new basic block
          auto dest = instruction["dest"];
          // for each of the phi args
          for(int i = 0; i < instruction["args"].size(); i++){
            BasicBlockDom new_bb;
            //std::cout << "NEW BB TIME" <<std::endl;
            auto arg = instruction["args"][i];
            auto from_label = instruction["labels"][i];
            std::string new_bb_label = "_" + std::to_string(i) + remove_quotes(from_label.dump());

            json bb_label = {
              {"label", new_bb_label}
            };
            new_bb.instructions.push_back(bb_label);
            json id_insn = {
              {"dest", dest},
              {"op", "id"},
              {"args", json::array({arg})}
            };

            //std::cout << "id_insn: " << id_insn << "\n";
            new_bb.instructions.push_back(id_insn);
            json jmp_insn = {
              {"op", "jmp"},
              {"labels", json::array({bb.getLabel()})}
            };
            new_bb.instructions.push_back(jmp_insn);

            // look through the predecessors for a match, to jump to the new bb
            for(auto& pred : bb.predecessors){
              if(pred->getLabel() == from_label){
                json pred_jmp_insn = {
                  {"op", "jmp"},
                  {"labels", json::array({new_bb_label})}
                };
                // replace last jump or add it
                if(pred->getLastInstruction()["op"] == "jmp"){
                  pred->instructions.pop_back();
                }
                pred->instructions.push_back(pred_jmp_insn);
                pred->successors.push_back(&new_bb);
                new_bb.predecessors.push_back(pred);
                break;
              }
            }

            new_phi_bbs.push_back(new_bb);
            //bb_itr = bbs.insert(bb_itr, new_bb);
            //newly_added_blocks++;
            //std::cout << "Finished a phi arg " << std::endl;
          }
        }
      }
      // remove phi nodes from this block
      bb.instructions.erase(std::remove_if(bb.instructions.begin(), bb.instructions.end(), [](auto& insn){
        return insn.contains("op") && insn["op"] == "phi";
      }), bb.instructions.end());
      //bb_itr+=newly_added_blocks+1;

    }

    bbs.insert(bbs.end(), new_phi_bbs.begin(), new_phi_bbs.end());
    printer.printFunction(func, bbs);
  }
  printer.print();

}

int main (int argc, char *argv[]) {
  const json prog = cli_parse_program(argc, argv);
  fromSSA(prog);
  
  return EXIT_SUCCESS;

}