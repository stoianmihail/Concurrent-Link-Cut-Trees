#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <optional>
#include <future>
#include <csignal>
#include "include/LCT.hpp"
#include "include/UnionFind.hpp"

using namespace std::chrono;

#define DEBUG_BENCHMARK 1

using Workload = std::vector<std::pair<unsigned, unsigned>>;
using WorkloadTriple = std::vector<std::tuple<unsigned, unsigned, unsigned>>;

double link_benchmark_lct(unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "LCT";
  auto checkForCorrectness = [&]() -> void {
    LinkCutTree lct;
    std::vector<LinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new LinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    // Start linking.
    UnionFind uf(n);
    for (unsigned index = 0; index != m; ++index) {
      lct.link(nodes[workload[index].first], nodes[workload[index].second]);
      uf.unionSets(workload[index].first, workload[index].second);
      auto c1 = uf.areUnion(workload[index].first, workload[index].second);
      auto c2 = lct.areConnected(nodes[workload[index].first], nodes[workload[index].second]);
      if (c1 != c2)
        std::cerr << "c1=" << c1 << " c2=" << c2 << std::endl;
      assert(c1 == c2);
    }
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();

  auto benchmark = [&]() -> double {
    LinkCutTree lct;
    std::vector<LinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new LinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    // Start linking.
    auto start = high_resolution_clock::now();
    for (unsigned index = 0; index != m; ++index) {
      lct.link(nodes[workload[index].first], nodes[workload[index].second]);
    }
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double link_benchmark_rlct(double probability, unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "RandomLCT(" + std::to_string(probability) + ")";
  auto checkForCorrectness = [&]() -> void {
    RandomLinkCutTree lct(probability);
    std::vector<RandomLinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new RandomLinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    // Start linking.
    UnionFind uf(n);
    for (unsigned index = 0; index != m; ++index) {
      lct.link(nodes[workload[index].first], nodes[workload[index].second]);
      uf.unionSets(workload[index].first, workload[index].second);
      auto c1 = uf.areUnion(workload[index].first, workload[index].second);
      auto c2 = lct.areConnected(nodes[workload[index].first], nodes[workload[index].second]);
      if (c1 != c2)
        std::cerr << "c1=" << c1 << " c2=" << c2 << std::endl;
      assert(c1 == c2);
    }
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();

  auto benchmark = [&]() -> double {
    RandomLinkCutTree lct(probability);
    std::vector<RandomLinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new RandomLinkCutTree::Node();
      nodes[index]->value = index;
    }
    
    // Start linking.
    auto start = high_resolution_clock::now();
    for (unsigned index = 0; index != m; ++index) {
      lct.link(nodes[workload[index].first], nodes[workload[index].second]);
    }
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double link_benchmark(std::string filename, unsigned n) {
  Workload workload;
  std::ifstream input(filename);
  if (!input.is_open()) {
    std::cerr << "Workload \"" << filename << "\" could not be loaded. Check its existance!" << std::endl;
    exit(-1);
  }
  
  std::cerr << "Start loading workload.." << std::endl;
  auto pos = input.tellg();
  input.seekg(0, std::ios::end);  
  auto size = input.tellg() - pos;
  input.seekg(0, std::ios::beg);
  unsigned elements = size / sizeof(std::pair<unsigned, unsigned>);
  workload.resize(elements);
  input.read(reinterpret_cast<char*>(workload.data()), elements * sizeof(std::pair<unsigned, unsigned>));
  unsigned m = workload.size();

  return link_benchmark_lct(n, workload);
#if 0
  for (unsigned index = 1; index <= 10; ++index)
    link_benchmark_rlct(0.1 * index, n, workload);
#endif
}

double lookup_benchmark_lct(unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "LCT";
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
          uf.unionSets(op.first, op.second);
          assert(uf.areUnion(op.first, op.second) == lct.areConnected(nodes[op.first], nodes[op.second]));
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

  auto benchmark = [&]() -> double {
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
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double lookup_benchmark_glct(unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "GlassLCT";
  auto checkForCorrectness = [&]() -> void {
    std::vector<GlassLinkCutTree::GlassNode*> nodes(n); 
    GlassLinkCutTree lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new GlassLinkCutTree::GlassNode();
      nodes[index]->label = index;
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
          uf.unionSets(op.first, op.second);
          assert(uf.areUnion(op.first, op.second) == lct.areConnected(nodes[op.first], nodes[op.second]));
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
          if (root != nodes[op.second])
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->label << " vs " << nodes[op.second]->label << std::endl;
          assert(lct.findRoot(nodes[op.first]) == nodes[op.second]);
        }
      }
      return true;
    };
    
    while (read()) {}
  };

  std::cerr << "Check for correctness.." << std::endl;
  //checkForCorrectness();

  auto benchmark = [&]() -> double {
    std::vector<GlassLinkCutTree::GlassNode*> nodes(n); 
    GlassLinkCutTree lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new GlassLinkCutTree::GlassNode();
      nodes[index]->label = index;
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
          lct.debug();
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double lookup_benchmark_rlct(double probability, unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "RandomLCT(" + std::to_string(probability) + ")";
  auto checkForCorrectness = [&]() -> void {
    RandomLinkCutTree rlct(probability);
    std::vector<RandomLinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new RandomLinkCutTree::Node();
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
          rlct.link(nodes[op.first], nodes[op.second]);
          uf.unionSets(op.first, op.second);
          assert(uf.areUnion(op.first, op.second) == rlct.areConnected(nodes[op.first], nodes[op.second]));
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = rlct.findRoot(nodes[op.first]);
          if (root != nodes[op.second])
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->value << " vs " << nodes[op.second]->value << std::endl;
          assert(rlct.findRoot(nodes[op.first]) == nodes[op.second]);
        }
      }
      return true;
    };
    
    while (read()) {}
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();

  auto benchmark = [&]() -> double {
    RandomLinkCutTree rlct(probability);
    std::vector<RandomLinkCutTree::Node*> nodes(n); 
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new RandomLinkCutTree::Node();
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
          rlct.link(nodes[op.first], nodes[op.second]);
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = rlct.findRoot(nodes[op.first]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double lookup_benchmark(std::string filename, unsigned n) {
  Workload workload;
  std::ifstream input(filename);
  if (!input.is_open()) {
    std::cerr << "Workload \"" << filename << "\" could not be loaded. Check its existance!" << std::endl;
    exit(-1);
  }
  
  std::cerr << "Start loading workload.." << std::endl;
  auto pos = input.tellg();
  input.seekg(0, std::ios::end);  
  auto size = input.tellg() - pos;
  input.seekg(0, std::ios::beg);
  unsigned elements = size / sizeof(std::pair<unsigned, unsigned>);
  workload.resize(elements);
  input.read(reinterpret_cast<char*>(workload.data()), elements * sizeof(std::pair<unsigned, unsigned>));
  unsigned m = workload.size();

  return lookup_benchmark_lct(n, workload);
#if 0
  for (unsigned index = 1; index <= 10; ++index)
    lookup_benchmark_rlct(0.1 * index, n, workload);
#endif
}

double cut_benchmark_lct(unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "LCT";
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
          auto op = workload[currIndex++];
          lct.cut(nodes[op.first]);
          assert(!lct.areConnected(nodes[op.first], nodes[op.second]));
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

  auto benchmark = [&]() -> double {
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
        }
      } else if (type == 2) {
        while (count--) {
          auto op = workload[currIndex++];
          lct.cut(nodes[op.first]);
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double cut_benchmark_ett(unsigned n, Workload& workload) {
  unsigned m = workload.size();
  std::string name = "ETT";
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
          auto op = workload[currIndex++];
          lct.cut(nodes[op.first]);
          assert(!lct.areConnected(nodes[op.first], nodes[op.second]));
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

  auto benchmark = [&]() -> double {
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
        }
      } else if (type == 2) {
        while (count--) {
          auto op = workload[currIndex++];
          lct.cut(nodes[op.first]);
        }
      } else {
        while (count--) {
          auto op = workload[currIndex++];
          auto root = lct.findRoot(nodes[op.first]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double cut_benchmark(std::string filename, unsigned n) {
  Workload workload;
  std::ifstream input(filename);
  if (!input.is_open()) {
    std::cerr << "Workload \"" << filename << "\" could not be loaded. Check its existance!" << std::endl;
    exit(-1);
  }
  
  std::cerr << "Start loading workload.." << std::endl;
  auto pos = input.tellg();
  input.seekg(0, std::ios::end);  
  auto size = input.tellg() - pos;
  input.seekg(0, std::ios::beg);
  unsigned elements = size / sizeof(std::pair<unsigned, unsigned>);
  workload.resize(elements);
  input.read(reinterpret_cast<char*>(workload.data()), elements * sizeof(std::pair<unsigned, unsigned>));
  unsigned m = workload.size();

  std::cerr << "Workload " << filename << ".size()=" << m << std::endl;
  
  return cut_benchmark_lct(n, workload);
}

double glass_lookup_benchmark(std::string filename, unsigned n) {
  Workload workload;
  std::ifstream input(filename);
  if (!input.is_open()) {
    std::cerr << "Workload \"" << filename << "\" could not be loaded. Check its existance!" << std::endl;
    exit(-1);
  }
  
  std::cerr << "Start loading workload.." << std::endl;
  auto pos = input.tellg();
  input.seekg(0, std::ios::end);  
  auto size = input.tellg() - pos;
  input.seekg(0, std::ios::beg);
  unsigned elements = size / sizeof(std::pair<unsigned, unsigned>);
  workload.resize(elements);
  input.read(reinterpret_cast<char*>(workload.data()), elements * sizeof(std::pair<unsigned, unsigned>));
  unsigned m = workload.size();
  
  std::cerr << "Workload:" << std::endl;
  for (auto elem: workload) {
    std::cerr << elem.first << "," << elem.second << std::endl;
  }
  std::cerr << "-------------" << std::endl;
  
  return lookup_benchmark_glct(n, workload);
}

double lca_benchmark_lct(unsigned n, WorkloadTriple& workload) {
  unsigned m = workload.size();
  std::string name = "LCT";
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
      auto [type, count, _] = workload[currIndex++];
      if (type == 1) {
        while (count--) {
          auto [u, v, _] = workload[currIndex++];
          lct.link(nodes[u], nodes[v]);
          uf.unionSets(u, v);
          assert(uf.areUnion(u, v) == lct.areConnected(nodes[u], nodes[v]));
        }
      } else {
        while (count--) {
          auto [u, v, lca] = workload[currIndex++];
          auto ret = lct.lca(nodes[u], nodes[v]);
          if (ret->value != lca)
            std::cerr << "op=(" << u << "," << v << ") lca=" << ret->value << " vs " << lca << std::endl;
          assert(ret->value == lca);
        }
      }
      return true;
    };
    
    while (read()) {}
  };

  std::cerr << "Check for correctness.." << std::endl;
  checkForCorrectness();

  auto benchmark = [&]() -> double {
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
      auto [type, count, _] = workload[currIndex++];
      if (type == 1) {
        while (count--) {
          auto [u, v, _] = workload[currIndex++];
          lct.link(nodes[u], nodes[v]);
        }
      } else {
        while (count--) {
          auto [u, v, _] = workload[currIndex++];
          lct.lca(nodes[u], nodes[v]);
        }
      }
      return true;
    };

    auto start = high_resolution_clock::now();
    while (read()) {}    
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };

  auto time = benchmark();
  std::cerr << name << ": " << time << " ms" << std::endl;
  return time;
}

double lca_benchmark(std::string filename, unsigned n) {
  WorkloadTriple workload;
  std::ifstream input(filename);
  if (!input.is_open()) {
    std::cerr << "Workload \"" << filename << "\" could not be loaded. Check its existance!" << std::endl;
    exit(-1);
  }
  
  std::cerr << "Start loading workload.." << std::endl;
  auto pos = input.tellg();
  input.seekg(0, std::ios::end);  
  auto size = input.tellg() - pos;
  input.seekg(0, std::ios::beg);
  unsigned elements = size / sizeof(std::pair<unsigned, unsigned>);
  workload.resize(elements);
  input.read(reinterpret_cast<char*>(workload.data()), elements * sizeof(std::tuple<unsigned, unsigned, unsigned>));
  unsigned m = workload.size();

  return lca_benchmark_lct(n, workload);
#if 0
  for (unsigned index = 1; index <= 10; ++index)
    lookup_benchmark_rlct(0.1 * index, n, workload);
#endif
}

void benchmark(std::string filename) { 
#if 0
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, n - 1);
#endif
#if 0
  auto debug = [&](std::pair<unsigned, unsigned> p) -> std::string {
     return "(" + std::to_string(p.first) + ", " + std::to_string(p.second) + ")";
  };
  
  std::cerr << "Debug workload" << std::endl;
  for (unsigned index = 0, limit = workload.size(); index != limit; ++index)
    std::cerr << debug(workload[index]) << std::endl;
#endif
    
  auto tokenize = [&]() -> std::vector<std::string> {
    auto pos = filename.find_last_of("/");
    auto tmp = filename.substr(1 + pos, filename.size());
    std::vector<std::string> tokens;
    pos = 0;
    for (std::string delimiter = "-"; (pos = tmp.find(delimiter)) != std::string::npos; tmp.erase(0, pos + 1))
      tokens.push_back(tmp.substr(0, pos));
    tokens.push_back(tmp);
    return std::move(tokens);
  };

  auto tokens = tokenize();
  auto type = tokens.front();
  auto w = tokens[1];
  auto b = tokens[tokens.size() - 2];
  auto n = atoi(tokens.back().substr(0, tokens.back().find_last_of(".")).data());
  std::cerr << "Start benchmarking \"" << type << "(" << std::to_string(n) << ")\"" << std::endl;
  double time = 0;
  if (type == "glass") {
    time = glass_lookup_benchmark(filename, n);
  } else if (type == "link") {
    time = link_benchmark(filename, n);
  } else if (type == "lookup") {
    time = lookup_benchmark(filename, n);
  } else if (type == "lca") {
    time = lca_benchmark(filename, n);
  } else if (type == "cut") {
    time = cut_benchmark(filename, n);
  }

  std::ofstream log("../logs/" + type + "-p_0" + "-w_" + w + "-b_" + b + "-n_" + std::to_string(n) + ".log");
  log << time << " ms" << std::endl;}

void sample_benchmark(unsigned n) {
  Workload workload;
  workload.push_back({1, 10});
  workload.push_back({1, 0});
  workload.push_back({4, 1});
  workload.push_back({3, 0});
  workload.push_back({2, 0});
  workload.push_back({5, 2});
  workload.push_back({6, 2});
  workload.push_back({8, 6});
  workload.push_back({9, 8});
  workload.push_back({10, 9});
  workload.push_back({7, 6});
  workload.push_back({0, 1});
  workload.push_back({9, 0});
  
  assert(n == 11);
  lookup_benchmark_glct(n, workload);
}

void benchmark_sample() {
  sample_benchmark(11);
} 

void signalHandler( int signum ) {
   std::cout << "Interrupt signal (" << signum << ") received.\n";

   // cleanup and close up stuff here  
   // terminate program  

   exit(signum);  
}

int main(int argc, char** argv) {
  //signal(SIGINT, signalHandler);  
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <workload:file> <sample:bool>" << std::endl;
    exit(-1);
  }
  if (argc == 2) {
    benchmark(argv[1]);
    return 0;
  }
  benchmark_sample();
  return 0;
}
