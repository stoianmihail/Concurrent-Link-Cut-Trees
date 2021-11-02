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
#include <thread>
#include <atomic>
#include <unordered_set>
#include "include/LCT.hpp"
#include "include/UnionFind.hpp"
#include "include/TreeBuilder.hpp"
#include "include/ConcurrentLCT.hpp"
#include "include/LockCouplingLCT.hpp"

using namespace std::chrono;
static constexpr unsigned infty = std::numeric_limits<unsigned>::max();
using Workload = std::vector<Pair>;

std::vector<int> computeDepths(std::vector<std::optional<unsigned>>& parent) {
  std::vector<int> depth(parent.size(), 0);
  std::function<int(unsigned)> computeDepth = [&](unsigned u) -> unsigned {
    if (!parent[u].has_value()) {
      return depth[u] = 0;
    }
    return depth[u] = (1 + computeDepth(parent[u].value())); 
  };

  for (unsigned index = 0, limit = parent.size(); index != limit; ++index)
    computeDepth(index);
  return depth;
}

Workload buildLayerWorkload(unsigned n, const std::vector<Pair>& edges) {
  UnionFind uf(n);
  std::vector<std::optional<unsigned>> parent(n, std::nullopt);
  Workload workload, inserts;
  unsigned m = edges.size();
  for (unsigned index = 0; index != m; ++index) {
    unsigned u = edges[index].first, v = edges[index].second;
    inserts.push_back(std::make_pair(u, v));
    parent[u] = v;
    uf.unify(u, v);
  }

  workload.push_back(std::make_pair(1, inserts.size()));
  workload.insert(workload.end(), inserts.begin(), inserts.end());  
  auto depth = computeDepths(parent);

  std::vector<std::optional<unsigned>> root = parent;
  std::function<unsigned(unsigned)> climb = [&](unsigned x) -> unsigned {
    if (!root[x].has_value()) return x;
    auto val = climb(root[x].value());
    root[x] = val;
    return val;
  };

  std::cerr << "Computing pairs.." << std::endl;
  std::vector<Pair> pairs;
  for (unsigned diff = 1; diff != n; ++diff) {
    auto prev_size = pairs.size();
    for (unsigned u = 0; u != n; ++u) {
      for (unsigned v = u + 1; v != n; ++v) {
        if (std::abs(depth[u] - depth[v]) != diff) continue;
        if (climb(u) == climb(v)) {
          pairs.push_back({u, root[u].has_value() ? climb(root[u].value()) : u});
          pairs.push_back({v, root[v].has_value() ? climb(root[v].value()) : v});
        }
      }
    }
    std::cerr << "> diff=" << diff << " pairs.size()=" << pairs.size() << std::endl;
    if (pairs.size() == prev_size) break;
  }

  std::random_shuffle(pairs.begin(), pairs.end());

  workload.push_back({0, pairs.size()});
  workload.insert(workload.end(), pairs.begin(), pairs.end());
  return workload;
}

void checkWorkload(unsigned n, Workload& workload) {
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
}

#if 0
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
#endif

Workload buildCustomWorkload(unsigned n, std::string tree_type) {
  std::vector<Pair> edges;
  if (tree_type == "random") {
    edges = buildRandomTree(n);
  } else {
    edges = buildKAryTree(n, tree_type);
  }
  assert(!edges.empty());
  
  // Shuffle the edges.
  // std::random_shuffle(edges.begin(), edges.end());

  std::cerr << "Start building workload.." << std::endl;
  auto workload = buildLayerWorkload(n, edges);
  std::cerr << "Workload.size(): " << workload.size() << std::endl;
  checkWorkload(n, workload);

  // And flush the output.
  std::ofstream output("../workloads/layer-" + tree_type + "-" + std::to_string(n) + ".bin");
  output.write(reinterpret_cast<char*>(workload.data()), workload.size() * sizeof(Pair));
  return workload;
}

template <class TreeType, class NodeType>
void runWorkload(unsigned n, unsigned num_threads, Workload& workload) {
  std::vector<NodeType*> nodes(n); 
  TreeType lct(n, nodes);
  for (unsigned index = 0; index != n; ++index) {
    nodes[index] = new NodeType();
    nodes[index]->label = index;
  }
  
  auto deployLinks = [&](unsigned lb, unsigned ub) {
    unsigned taskSize = 1;
    unsigned numTasks = (ub - lb) / taskSize + ((ub - lb) % taskSize != 0);
    std::atomic<unsigned> taskIndex = 0;
    auto consume = [&]() -> void {
      while (taskIndex.load() < numTasks) {
        unsigned i = taskIndex++;
        if (i >= numTasks)
          return;
        
        // Compute the range.
        unsigned startIndex = lb + (taskSize * i);
        unsigned stopIndex = lb + (taskSize * (i + 1));
        if (i == numTasks - 1)
          stopIndex = ub;
        
        // Start linking.
        for (unsigned index = startIndex; index != stopIndex; ++index) {
          lct.link(nodes[workload[index].first], nodes[workload[index].second]);
        }
      }
    };
    
    // Single-threaded.
    consume();
  };

  auto deployLookups = [&](unsigned lb, unsigned ub) {
    unsigned taskSize = 1;
    unsigned numTasks = (ub - lb) / taskSize + ((ub - lb) % taskSize != 0);
    std::atomic<unsigned> taskIndex = 0;
    auto consume = [&]() -> void {
      while (taskIndex.load() < numTasks) {
        unsigned i = taskIndex++;
        if (i >= numTasks)
          return;
        
        // Compute the range.
        unsigned startIndex = lb + (taskSize * i);
        unsigned stopIndex = lb + (taskSize * (i + 1));
        if (i == numTasks - 1)
          stopIndex = ub;
        
        for (unsigned index = startIndex; index != stopIndex; ++index) {
          auto root = lct.findRoot(nodes[workload[index].first]);
        }
      }
    };
      
    std::vector<std::thread> threads;
    for (unsigned index = 0, limit = num_threads; index != limit; ++index) {
      threads.emplace_back(consume);
    }
    for (auto& thread : threads) {
      thread.join();
    }
  };
  
  unsigned currIndex = 0;
  do {
    auto elem = workload[currIndex];
    ++currIndex;
    
    if (elem.first == 1) {
      deployLinks(currIndex, currIndex + elem.second);
      std::cerr << "Deployed " << elem.second << " links!" << std::endl;
    } else {
      deployLookups(currIndex, currIndex + elem.second);
      std::cerr << "Deployed " << elem.second << " lookups!" << std::endl;
    }
    currIndex += elem.second;
  } while (currIndex != workload.size());
}

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <n:unsigned> <tree_type:string[k-ary,random]> <num_threads:unsigned>" << std::endl;
    exit(-1);
  }
  auto workload = buildCustomWorkload(atoi(argv[1]), argv[2]);
  runWorkload<LockCouplingLinkCutTrees, LockCouplingLinkCutTrees::CoNode>(atoi(argv[1]), atoi(argv[3]), workload);
  return 0;
}