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

void benchmark(std::string filename) {
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
  auto workload_type = tokens.front();
  auto w = tokens[1];
  auto b = tokens[tokens.size() - 2];
  auto n = atoi(tokens.back().substr(0, tokens.back().find_last_of(".")).data());
  std::cerr << "Start benchmarking \"" << workload_type << "(" << std::to_string(n) << ")\"" << std::endl;
  double time = 0;
  if (workload_type == "lookup") {
    time = lookup_benchmark(filename, n);  
  } else if (workload_type == "cut") {
    time = cut_benchmark(filename, n);  
  } else {
    std::cerr << "Workload \"" << workload_type << "\" not yet supported!" << std::endl;
    exit(-1);
  }

  std::ofstream log("../logs/" + workload_type + "-p_0" + "-w_" + w + "-b_" + b + "-n_" + std::to_string(n) + ".log");
  log << time << " ms" << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <workload:file>" << std::endl;
    exit(-1);
  }

  // Benchmark.
  benchmark(argv[1]);
  return 0;
}
