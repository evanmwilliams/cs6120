#include <cstdlib>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.hpp"
#include "../util/cfg.hpp"
#include "dominators.hpp"
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "../util/print_programs.hpp"

using json = nlohmann::json;

void interleavePhiBBs(std::vector<BasicBlockDom> &bbs, std::vector<BasicBlockDom> & new_phi_bbs){
  for(auto& phi_bb : new_phi_bbs){
    auto jmp_label = phi_bb.getLastInstruction()["labels"][0];
    auto bb_itr = std::find_if(bbs.begin(), bbs.end(), [&jmp_label](auto& bb){
      try {
        auto label = bb.getLabel();
      } catch (std::runtime_error& e){
        return false;
      }
      return bb.getLabel() == jmp_label;
    });
    bbs.insert(bb_itr, phi_bb);
  }

}


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
      std::unordered_map<std::string, int> label_to_phi_bb;
      for(auto &instruction : bb.instructions){
        // map of var to existing phi node block assignment predecessor

        if(instruction.contains("op") && instruction["op"] == "phi"){
          // insert new basic block
          auto dest = instruction["dest"];
          // for each of the phi args
          for(int i = 0; i < instruction["args"].size(); i++){
            //std::cout << "NEW BB TIME" <<std::endl;
            auto arg = instruction["args"][i];
            auto from_label = instruction["labels"][i];
            // std::cout << "Looking for PHI BB for " << from_label << std::endl;

            // // print out label_to_phi_bb
            // for(auto &[label, bb] : label_to_phi_bb){
            //   std::cout << label << " -> " << bb << std::endl;
            // } 

            if(label_to_phi_bb.count(from_label) > 0){
              //std::cout << "Found a block to add to!" << std::endl;
              json id_insn = {
                {"dest", dest},
                {"op", "id"},
                {"args", json::array({arg})}
              };
              auto& phi_bb = new_phi_bbs[label_to_phi_bb[from_label]];
              auto jmp = phi_bb.instructions.back();
              phi_bb.instructions.pop_back();
              phi_bb.instructions.push_back(id_insn);
              phi_bb.instructions.push_back(jmp);
              //swap last insn with jump
              //std::cout << "Added to block " << from_label << std::endl;
              continue;
            }

            BasicBlockDom new_bb;
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
            label_to_phi_bb[from_label] = new_phi_bbs.size()-1;
            
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

    interleavePhiBBs(bbs, new_phi_bbs);
    //bbs.insert(bbs.end(), new_phi_bbs.begin(), new_phi_bbs.end());
    printer.printFunction(func, bbs);
  }
  printer.print();

}

int main (int argc, char *argv[]) {
  const json prog = cli_parse_program(argc, argv);
  fromSSA(prog);
  
  return EXIT_SUCCESS;

}