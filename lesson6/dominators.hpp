#pragma once

#include "../util/find_blocks.cpp"
#include "../util/cfg.cpp"

void getDominators(std::vector<BasicBlockDom> &bbs, BasicBlockDom *entry);

void dominanceTree(std::vector<BasicBlockDom> &bbs);

void domTreeGV(std::ostream &os, std::vector<BasicBlockDom> &bbs);

void dominanceFrontier(std::vector<BasicBlockDom> &bbs);