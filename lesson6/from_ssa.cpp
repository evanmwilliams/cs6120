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
    std::vector<BasicBlockDom> ordered_bbs;

    std::unordered_map<std::string, int> label_to_phi_bb;

    // in each basic block, look for a phi node
    // if there is a phi node, then we need to insert a new basic block
    // before the current basic block
    for(auto bb_itr = bbs.begin(); bb_itr != bbs.end(); bb_itr++){
      auto& bb = *bb_itr;
      //std::cout << "***** RESETTING LABEL TO PHI BB *******" << std::endl;
      for(auto &phi_node : bb.phi_nodes){
          //std::cout << "working on phi node: " << phi_node.toInstruction() << std::endl;
          auto dest = phi_node.dest;
          // for each of the phi args
          for(int i = 0; i < phi_node.args.size(); i++){
            auto arg = phi_node.args[i];
            auto from_label = phi_node.labels[i];

            //std::cout << "Looking for phi bb for label: " << from_label << std::endl;
            if(label_to_phi_bb.count(from_label) > 0){
              //std::cout << "Found existing phi bb for label: " << from_label << std::endl;
              json id_insn = {
                {"dest", dest},
                {"op", "id"},
                {"args", json::array({arg})}
              };
              auto phi_bb = ordered_bbs[label_to_phi_bb[from_label]];
              auto jmp = phi_bb.instructions.back();
              phi_bb.instructions.pop_back();
              phi_bb.instructions.push_back(id_insn);
              phi_bb.instructions.push_back(jmp);

              ordered_bbs[label_to_phi_bb[from_label]] = phi_bb;
              //swap last insn with jump
              //std::cout << "Added to block " << from_label << std::endl;
              continue;
            }

            BasicBlockDom new_bb;
            std::string new_bb_label = "_" + std::to_string(i) + from_label;

            json bb_label = {
              {"label", new_bb_label}
            };
            new_bb.addInstr(bb_label);
            //std::cout << "CREATED NEW PHI BB WITH LABEL: " << new_bb.getLabel () << std::endl;
            json id_insn = {
              {"dest", dest},
              {"op", "id"},
              {"args", json::array({arg})}
            };

            //std::cout << "id_insn: " << id_insn << "\n";
            new_bb.addInstr(id_insn);
            json jmp_insn = {
              {"op", "jmp"},
              {"labels", json::array({bb.getLabel()})}
            };
            new_bb.addInstr(jmp_insn);

            // look through the predecessors for a match, to jump to the new bb
            for(auto& pred : bb.predecessors){
              if(pred->getLabel() == from_label){
                json pred_jmp_insn = {
                  {"op", "jmp"},
                  {"labels", json::array({new_bb_label})}
                };
                // replace last jump or add it
                if(pred->instructions.size() > 0 && pred->getLastInstruction()["op"] == "jmp"){
                  pred->instructions.pop_back();
                  pred->addInstr(pred_jmp_insn);
                } else if(pred->instructions.size() > 0 && pred->getLastInstruction()["op"] == "br"){
                  auto br_insn = pred->instructions.back();
                  pred->instructions.pop_back();
                  for(auto& label : br_insn["labels"]){
                    if(label == bb.getLabel()){
                      label = new_bb_label;
                    }
                  }
                  pred->addInstr(br_insn);
                } else {
                  pred->addInstr(pred_jmp_insn);
                }
                
                //also update this pred_bb in ordered_bbs
                for(int i = 0; i < ordered_bbs.size(); i++){
                  if(ordered_bbs[i].getLabel() == pred->getLabel()){
                    ordered_bbs[i] = *dynamic_cast<BasicBlockDom* >(pred);
                    break;
                  }
                }
                break;
              }
            }

            //with reordering
            new_phi_bbs.push_back(new_bb);

            // no reordering
            ordered_bbs.push_back(new_bb);
            //label_to_phi_bb[from_label] = new_phi_bbs.size()-1;
            label_to_phi_bb[from_label] = ordered_bbs.size()-1;
          }
        
      }
      // remove phi nodes from this block
      bb.phi_nodes.clear();
      ordered_bbs.push_back(bb);

    }

    //interleavePhiBBs(bbs, new_phi_bbs);
    //printer.printFunction(func, bbs);
    printer.printFunction(func, ordered_bbs);
  }
  printer.print();

}

int main (int argc, char *argv[]) {
  const json prog = cli_parse_program(argc, argv);
  fromSSA(prog);
  
  return EXIT_SUCCESS;

}