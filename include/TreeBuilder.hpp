#pragma once
#include <string>
#include <vector>

using Pair = std::pair<unsigned, unsigned>;

std::vector<Pair> buildEdges(unsigned n, std::vector<unsigned>& tree) {
  std::vector<Pair> edges;
  for (unsigned index = 0; index != n; ++index) {
    if (index == tree[index]) continue;
    edges.push_back({index, tree[index]});
  }
  return edges;
}

std::vector<Pair> buildRandomTree(unsigned n) {
  uint64_t randomState = 123;
  auto nextRandom = [&]() {
    uint64_t x=randomState;
    x = x ^ (x >> 12);
    x = x ^ (x << 25);
    x = x ^ (x >> 27);
    randomState = x;
    return x * 0x2545F4914F6CDD1Dull;
  };
  
  auto randomInt = [&](unsigned lower, unsigned upper) {
    return (upper <= lower) ? lower : (lower + (nextRandom() % (upper - lower + 1)));
  };
  
  std::vector<unsigned> tree(n);
  for (unsigned index = 1; index != n; ++index) {
    tree[index] = randomInt(0, index - 1);
  }
  return buildEdges(n, tree);
}

std::vector<Pair> buildKAryTree(unsigned n, std::string tree_type) {
  auto k = atoi(tree_type.substr(0, tree_type.find("-")).data());
  std::vector<unsigned> tree(n);
  for (unsigned index = 0; index != n; ++index) {
    if (!index) continue;
    tree[index] = (index - 1) / k;
  }
  return buildEdges(n, tree);
}