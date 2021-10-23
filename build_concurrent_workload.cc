#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <functional>
#include <optional>
#include <limits>
#include <cstdlib>
#include <unordered_set>
#include "include/LCT.hpp"
#include "include/UnionFind.hpp"

using namespace std::chrono;
static constexpr unsigned infty = std::numeric_limits<unsigned>::max();
using Pair = std::pair<unsigned, unsigned>;
using Triple = std::tuple<unsigned, unsigned, unsigned>;
using Workload = std::vector<Pair>;

template<class Generator>
std::vector<unsigned> sample(std::vector<unsigned>& list, unsigned capacity, Generator& engine) {
  // Compute the reservoir sampling
  std::vector<unsigned> indices(capacity);
  for (unsigned index = 0; index != capacity; ++index)
    indices[index] = list[index];

  unsigned expand = capacity + 1;
  for (unsigned index = capacity, limit = list.size(); index != limit; ++index) {
    unsigned r = std::uniform_int_distribution<unsigned>{0, expand++}(engine);
    if (r < capacity)
      indices[r] = list[index];
  }
  return indices;
}

static uint64_t randomState = 123;

std::vector<Pair> buildEdges(unsigned n, std::vector<unsigned>& tree) {
  std::vector<Pair> edges;
  for (unsigned index = 0; index != n; ++index) {
    if (index == tree[index]) continue;
    edges.push_back({index, tree[index]});
  }
  return edges;
}

std::vector<Pair> buildRandomTree(unsigned n, unsigned batch_size) {
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

std::vector<Pair> buildKAryTree(unsigned n, std::string tree_type, unsigned batch_size) {
  auto k = atoi(tree_type.substr(0, tree_type.find("-")).data());
  std::cerr << "Build " << tree_type << " tree of " << n << " nodes!" << std::endl;
  std::vector<unsigned> tree(n);
  for (unsigned index = 0; index != n; ++index) {
    if (!index) continue;
    tree[index] = (index - 1) / k;
  }
  return buildEdges(n, tree);
}

Workload buildLookupWorkload(unsigned n, std::vector<Pair> edges, unsigned batch_size) {
  std::vector<std::optional<unsigned>> parent(n);
  auto debug = [&](std::optional<unsigned> x) -> std::string {
    return (x.has_value() ? std::to_string(x.value()) : "none");
  };

  UnionFind uf(n);
  std::function<unsigned(unsigned)> climb = [&](unsigned x) -> unsigned {
    if (!parent[x].has_value()) return x;
    auto val = climb(parent[x].value());
    parent[x] = val;
    return val;
  };

  std::function<bool(unsigned&, unsigned&)> preprocess = [&](unsigned &u, unsigned &v) -> bool {
    if (!parent[u]) return true;
    if (!parent[v]) {
      std::swap(u, v);
      return true;
    }
    u = climb(u);
    v = climb(v);
    return true;
  };

  std::cerr << "Start building workload.." << std::endl;
  unsigned barrier = batch_size;
  std::vector<Pair> workload, inserts, workingSet;
  workingSet.assign(n, {infty, 0});
  
  auto complete = [&]() -> void {
    if (inserts.size() <= 1)
      return;
    workload.push_back(std::make_pair(1, inserts.size()));
    workload.insert(workload.end(), inserts.begin(), inserts.end());
    
    std::sort(workingSet.begin(), workingSet.end(), [&](auto lhs, auto rhs) {
      return lhs.first < rhs.first;
    });
    
    std::vector<Pair> lookups;
    std::unordered_set<unsigned> already;
    for (unsigned index = 0, limit = workingSet.size(), minLimit = std::min(barrier, limit); (index != minLimit) && (workingSet[index].first != infty); ++index) {
      auto [_, node] = workingSet[index];
      if (already.find(node) != already.end())
        continue;
      lookups.push_back(std::make_pair(node, parent[node].has_value() ? climb(parent[node].value()) : node));
      already.insert(node);
    }
    workload.push_back(std::make_pair(0, lookups.size()));
    workload.insert(workload.end(), lookups.begin(), lookups.end());
    workingSet.assign(n, {infty, 0});
    inserts.clear();
  };
  
  unsigned m = edges.size();
  for (unsigned index = 0; index != m; ++index) {
#if 1
    if (index % 1024 == 0) std::cerr << "Checkpoint: index=" << index << std::endl;
#endif
    unsigned u = edges[index].first, v = edges[index].second;
    
    workingSet[u] = {index, u};
    workingSet[v] = {index, v};
    inserts.push_back(std::make_pair(u, v));
    parent[u] = v;
    uf.unify(u, v);
    
    if ((index) && (index % barrier == 0)) {
      complete();
    }
  }
  if (!inserts.empty()) {
    complete();
  }
  m = workload.size();
  
  auto checkForCorrectness = [&]() -> void {
    LinkCutTree lct;
    std::vector<LinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new LinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    UnionFind uf(n);
    unsigned currIndex = 0;
    auto read = [&]() -> bool {
      if (currIndex == workload.size())
        return false;
      auto [type, count] = workload[currIndex++];
      if (type == 1) {
        while (count--) {
          auto op = workload[currIndex++];
          lct.link(nodes[op.first], nodes[op.second]);
          uf.unify(op.first, op.second);
          assert(uf.areConnected(op.first, op.second) == lct.areConnected(nodes[op.first], nodes[op.second]));
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
          if (root != nodes[op.second])
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->value << " vs " << nodes[op.second]->value << std::endl;
          assert(lct.findRoot(nodes[op.first]) == nodes[op.second]);
        }
      }
      return true;
    };
    
    while (read()) {}
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();
  return std::move(workload);
}

Workload buildCutWorkload(unsigned n, std::vector<Pair> edges, unsigned batch_size) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, n - 1);
  std::vector<std::optional<unsigned>> parent(n);

  auto debug = [&](std::optional<unsigned> x) -> std::string {
    return (x.has_value() ? std::to_string(x.value()) : "none");
  };

  std::function<unsigned(unsigned)> climb = [&](unsigned x) -> unsigned {
    if (!parent[x].has_value()) return x;
    auto val = climb(parent[x].value());
    return val;
  };

  std::function<bool(unsigned&, unsigned&)> preprocess = [&](unsigned &u, unsigned &v) -> bool {
    if (!parent[u]) return true;
    if (!parent[v]) {
      std::swap(u, v);
      return true;
    }
    u = climb(u);
    v = climb(v);
    return true;
  };

  // std::cerr << "Start building workload.." << std::endl;
  unsigned barrier = batch_size;
  std::vector<Pair> workload, inserts, workingSet, persistent;
  unsigned buffPtr = 0;
  std::vector<bool> taken;
  workingSet.assign(n, {infty, 0});
  
  auto complete = [&]() -> void {
    if (inserts.size() <= 1)
      return;
    auto processWorkingSet = [&]() -> void {
      std::sort(workingSet.begin(), workingSet.end(), [&](auto lhs, auto rhs) {
        return lhs.first < rhs.first;
      });
      
      std::vector<Pair> lookups;
      std::unordered_set<unsigned> already;
      for (unsigned index = 0, limit = workingSet.size(), minLimit = std::min(barrier, limit); (index != minLimit) && (workingSet[index].first != infty); ++index) {
        auto [_, node] = workingSet[index];
        if (already.find(node) != already.end())
          continue;
        lookups.push_back(std::make_pair(node, parent[node].has_value() ? climb(parent[node].value()) : node));
        already.insert(node);
      }
      workload.push_back(std::make_pair(0, lookups.size()));
      workload.insert(workload.end(), lookups.begin(), lookups.end());
      workingSet.assign(n, {infty, 0});
    };
    
    workload.push_back(std::make_pair(1, inserts.size()));
    workload.insert(workload.end(), inserts.begin(), inserts.end());
    processWorkingSet();
    inserts.clear();
    
    std::vector<Pair> cuts;
    unsigned cnt = 0, limCut = barrier;
    while (buffPtr != persistent.size()) {
      if (taken[buffPtr]) continue;
      ++cnt;
      if (cnt == limCut) break;
      cuts.push_back(persistent[buffPtr]);
      taken[buffPtr] = true;
      auto [u, v] = persistent[buffPtr];
      parent[u] = std::nullopt;
      workingSet[u] = {buffPtr, u};
      workingSet[v] = {buffPtr, v};
      ++buffPtr;
    }    
    workload.push_back(std::make_pair(2, cuts.size()));
    workload.insert(workload.end(), cuts.begin(), cuts.end());
    processWorkingSet();
  };
  
  UnionFind uf(n);
  for (unsigned index = 0, limit = edges.size(); index != limit; ++index) {
#if 1
    if (index % 1024 == 0) std::cerr << "Checkpoint: index=" << index << std::endl;
#endif
    unsigned u = edges[index].first, v = edges[index].second;
    uf.unify(u, v);
    workingSet[u] = {index, u};
    workingSet[v] = {index, v};
    inserts.push_back(std::make_pair(u, v));
    persistent.push_back(std::make_pair(u, v));
    taken.push_back(false);
    parent[u] = v;
    
    if ((index) && (index % barrier == 0)) {
      std::cerr << "index=" << index << " complete!" << std::endl;
      complete();
    }
  }
  if (!inserts.empty()) {
    complete();
  }
  unsigned m = workload.size();
  
#if 0
  std::cerr << "m=" << m << std::endl;
  if (m <= 150) {
    for (auto elem: workload) {
      std::cerr << elem.first << "," << elem.second << std::endl;
    }
  }
#endif

  auto checkForCorrectness = [&]() -> void {
    LinkCutTree lct;
    std::vector<LinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new LinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    unsigned currIndex = 0;
    auto read = [&]() -> bool {
      if (currIndex == workload.size())
        return false;
      auto [type, count] = workload[currIndex++];
      if (type == 1) {
        while (count--) {
          auto op = workload[currIndex++];
          lct.link(nodes[op.first], nodes[op.second]);
          assert(lct.areConnected(nodes[op.first], nodes[op.second]));
        }
      } else if (type == 2) {
        while (count--) {
          auto [u, v] = workload[currIndex++];
          lct.cut(nodes[u]);
          assert(!lct.areConnected(nodes[u], nodes[v]));
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
          if (root != nodes[op.second])
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->value << " vs " << nodes[op.second]->value << std::endl;
          assert(lct.findRoot(nodes[op.first]) == nodes[op.second]);
        }
      }
      return true;
    };
    
    while (read()) {}
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();
  return std::move(workload);
}

void buildConcurrentWorkload(unsigned n, std::string tree_type, std::string workload_type, unsigned batch_size) {
  std::vector<Pair> edges;
  if (tree_type == "random") {
    edges = buildRandomTree(n, batch_size);
  } else {
    edges = buildKAryTree(n, tree_type, batch_size);
  }
  assert(!edges.empty());
  
  // Shuffle the edges.
  std::random_shuffle(edges.begin(), edges.end());

  // Take only half of the edges.
  edges.resize(static_cast<unsigned>(0.5 * edges.size()));
  
  // Build the workload.
  Workload workload;
  if (workload_type == "lookup") {
    workload = buildLookupWorkload(n, edges, batch_size);
  } else if (workload_type == "cut") {
    workload = buildCutWorkload(n, edges, batch_size);
  } else {
    std::cerr << "Workload \"" << workload_type << "\" not yet supported!" << std::endl;
    exit(-1);
  }
  
  // And flush the output.
  std::ofstream output("../workloads/" + workload_type + "-" + tree_type + "-" + std::to_string(batch_size) + "-" + std::to_string(n) + ".bin");
  output.write(reinterpret_cast<char*>(workload.data()), workload.size() * sizeof(std::pair<unsigned, unsigned>));
}

int main(int argc, char** argv) {
  // Example: Construct worload of path with n=10000 (#nodes) β=1000 (batch size): ./build_batch_workload 10000 1 random cut 1000
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " <n:unsigned> <tree_type:string[k-ary,random]> <workload_type:string[lookup,cut]> <β:unsigned[>= 100]>" << std::endl;
    exit(-1);
  }
  buildConcurrentWorkload(atoi(argv[1]), argv[2], argv[3], atoi(argv[4]));
}
