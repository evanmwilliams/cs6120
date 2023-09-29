#pragma once
#include "dominators.hpp"
#include <boost/dynamic_bitset.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include "../util/parse_program.hpp"
#include "../util/find_blocks.hpp"
#include "../util/cfg.hpp"

void getDominators(std::vector<BasicBlockDom> &bbs, BasicBlockDom *entry)
{
  size_t n = bbs.size();
  // std::vector<DominatorValue> vals;
  for (auto &bb : bbs)
  {
    bb.dom_by.resize(n, true);
  }
  entry->dom_by.reset();
  size_t entry_id = entry->id;
  entry->dom_by.set(entry_id);

  bool dom_changes = true;
  while (dom_changes)
  {
    dom_changes = false;

    for (auto &bb : bbs)
    {
      if (bb.id == entry_id)
        continue;
      auto old_dom_by = bb.dom_by;
      auto &dom_by = bb.dom_by;
      for (auto &pred : bb.predecessors)
      {
        dom_by &= dynamic_cast<BasicBlockDom *>(pred)->dom_by;
      }
      dom_by.set(static_cast<size_t>(bb.id)); // set reflexive dom

      if (!dom_changes and dom_by != old_dom_by)
      {
        dom_changes = true;
      }
    }
  }
}
#undef ADD_TO_WL

void dominanceTree(std::vector<BasicBlockDom> &bbs)
{
  boost::dynamic_bitset<> temp(bbs.size());
  for (auto &bb : bbs)
  {
    for (size_t i = 0; i < bbs.size(); i++)
    {
      if (bb.dom_by.test(i) && static_cast<size_t>(bb.id) != i)
      {
        auto &dom = bbs[i];
        temp.reset();
        // dom is bb's idom iff
        // dom's dominators are equal to bb's dominators - bb
        temp |= dom.dom_by;
        temp.set(static_cast<size_t>(bb.id));
        if (temp == bb.dom_by)
        {
          bb.idom = &dom;
          dom.dom_succs.push_back(&bb);
          break;
        }
      }
    }
  }
}

void domTreeGV(std::ostream &os, std::vector<BasicBlockDom> &bbs)
{

  os << "digraph ";
  os << "function";
  os << " {\nnode[shape=rectangle]\n";
  os << "label=";
  os << ("\"fn_name\"");
  os << ";\n";
  // for (auto &bb : bbs)
  // {
  //   os << "\"bb.id\"";
  //   os << " [label=\"";
  //   os << bb.id;
  //   os << "\"];\n";
  // }

  for (auto &bb : bbs)
  {
    for (auto &dom : bb.dom_succs)
    {
      os << (bb.id);
      os << "->";
      os << (dom->id);
      os << ";\n";
    }
  }
  os << "}\n";
}

void dominanceFrontierHelper(BasicBlockDom &bb, boost::dynamic_bitset<> &visible)
{
  if (!visible.test(static_cast<size_t>(bb.id)))
  {
    auto &dominance_front = bb.dfront;

    // {n' | n ≻ n'}

    for (auto &succ : bb.successors)
    {
      dominance_front.insert(dynamic_cast<BasicBlockDom *>(succ));
    }
    // U_{n idom c} DF[c]
    for (auto bb2 : bb.dom_succs)
    {
      dominanceFrontierHelper(*bb2, visible);
      dominance_front.insert(bb2->dfront.begin(), bb2->dfront.end());
    }

    // {n' | n dom n'}
    for (auto it = dominance_front.begin(); it != dominance_front.end();)
    {
      auto front = *it;
      if (front->dom_by.test(bb.id))
      {
        it = dominance_front.erase(it);
      }
      else
      {
        ++it;
      }
    }

    visible.set(bb.id);
  }
}

void dominanceFrontier(std::vector<BasicBlockDom> &bbs)
{
  // vis_.reset();
  for (auto &bb : bbs)
  {
    boost::dynamic_bitset<> visible(bbs.size());
    dominanceFrontierHelper(bb, visible);
  }
  // if (os_) cfg::printFnGV(bbs_, DfrontPrint(*os_, cu_, *this), fn_.name);
}