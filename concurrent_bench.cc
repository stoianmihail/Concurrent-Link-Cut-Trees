#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <optional>
#include <future>
#include <csignal>
#include "include/ConcurrentLCT.hpp"
#include "include/LockCouplingLCT.hpp"

using namespace std::chrono;

#define DEBUG_PRL_BENCHMARK 0

using Workload = std::vector<std::pair<unsigned, unsigned>>;
using WorkloadTriple = std::vector<std::tuple<unsigned, unsigned, unsigned>>;

template <class TreeType, class NodeType>
double lookup_benchmark_lct(unsigned n, unsigned num_threads, unsigned task_factor, Workload& workload) { 
  unsigned m = workload.size();

  // Perform sequential operations, when the task size is zero.
  auto sequential = [&](TreeType& lct, std::vector<NodeType*>& nodes, unsigned lb, unsigned ub, std::string type, bool verify = false) {
    if (type == "link") {
      // Link.
      for (unsigned index = lb; index != ub; ++index)
        lct.link(nodes[workload[index].first], nodes[workload[index].second]);
    } else if (type == "lookup") {
      // Lookup.
      for (unsigned index = lb; index != ub; ++index) {
        auto op = workload[index];
        auto root = lct.findRoot(nodes[op.first]);
        
        // Verify.
        if (verify) {
          if (root->label != nodes[op.second]->label)
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->label << " vs " << nodes[op.second]->label << std::endl;
          assert(root->label == nodes[op.second]->label);
        }
      }
    } else if (type == "cut") {
      // Cut.
      for (unsigned index = lb; index != ub; ++index) {
        lct.cut(nodes[workload[index].first]);
      }
    }
  };
  
  auto checkForCorrectness = [&]() -> void {
    std::cerr << "**************** CHECK FOR CORRECTNESS ****************" << std::endl;
    std::vector<NodeType*> nodes(n); 
    TreeType lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new NodeType();
      nodes[index]->label = index;
    }
    
    auto deployLinks = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "link", true);
        return;
      }
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
        
      std::vector<std::thread> threads;
      for (unsigned index = 0, limit = num_threads; index != limit; ++index) {
        threads.emplace_back(consume);
      }
      for (auto& thread : threads) {
        thread.join();
      }
    };
    
    auto deployLookups = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "lookup", true);
        return;
      }
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
            auto op = workload[index];
            auto root = lct.findRoot(nodes[op.first]);
            if (root->label != nodes[op.second]->label)
              std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->label << " vs " << nodes[op.second]->label << std::endl;
            assert(root->label == nodes[op.second]->label);
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
      } else {
        deployLookups(currIndex, currIndex + elem.second);
      }
      currIndex += elem.second;
    } while (currIndex != workload.size());
  };

  try {
    std::cerr << "Check for correctness.." << std::endl;
    for (unsigned index = 0; index != 10; ++index) {
      checkForCorrectness();
    }
  } catch (...) {
    std::cerr << "Correctness test failed!" << std::endl;
    assert(0);
  }

  auto benchmark = [&]() -> double {
    std::vector<NodeType*> nodes(n); 
    TreeType lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new NodeType();
      nodes[index]->label = index;
    }
    
    auto deployLinks = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "link");
        return;
      }
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
        
      std::vector<std::thread> threads;
      for (unsigned index = 0, limit = num_threads; index != limit; ++index) {
        threads.emplace_back(consume);
      }
      for (auto& thread : threads) {
        thread.join();
      }
    };
    
    auto deployLookups = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "lookup");
        return;
      }
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
    
    
    std::cerr << "Start workload.." << std::endl;
    auto start = high_resolution_clock::now();
    unsigned currIndex = 0;
    do {
      auto elem = workload[currIndex];
      ++currIndex;
      
      if (elem.first == 1) {
        deployLinks(currIndex, currIndex + elem.second);
      } else {
        deployLookups(currIndex, currIndex + elem.second);
      }
      currIndex += elem.second;
    } while (currIndex != workload.size());
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };
  
  try {
    auto time = benchmark();
    std::cerr << "Benchmark: " << time << " ms" << std::endl;
    return time;
  } catch (...) {
    std::cerr << "Benchmark failed!" << std::endl;
    assert(0);
  }
  return 0;
}

template <class TreeType, class NodeType>
double cut_benchmark_lct(unsigned n, unsigned num_threads, unsigned task_factor, Workload& workload) {  
  auto sequential = [&](TreeType& lct, std::vector<NodeType*>& nodes, unsigned lb, unsigned ub, std::string type, bool verify = false) {
    if (type == "link") {
      for (unsigned index = lb; index != ub; ++index)
        lct.link(nodes[workload[index].first], nodes[workload[index].second]);
    } else if (type == "lookup") {
      for (unsigned index = lb; index != ub; ++index) {
        auto op = workload[index];
        auto root = lct.findRoot(nodes[op.first]);
        if (verify) {
          if (root->label != nodes[op.second]->label)
            std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->label << " vs " << nodes[op.second]->label << std::endl;
          assert(root->label == nodes[op.second]->label);
        }
      }
    } else if (type == "cut") {
      for (unsigned index = lb; index != ub; ++index) {
        lct.cut(nodes[workload[index].first]);
      }
    }
  };
  
  unsigned m = workload.size();
  auto checkForCorrectness = [&]() -> void {
    std::cerr << "**************** CHECK FOR CORRECTNESS ****************" << std::endl;
    std::vector<NodeType*> nodes(n); 
    TreeType lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new NodeType();
      nodes[index]->label = index;
    }
    
    auto deployCuts = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "cut", true);
        return;
      }
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
            lct.cut(nodes[workload[index].first]);
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
    
    auto deployLinks = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "link", true);
        return;
      }
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
        
      std::vector<std::thread> threads;
      for (unsigned index = 0, limit = num_threads; index != limit; ++index) {
        threads.emplace_back(consume);
      }
      for (auto& thread : threads) {
        thread.join();
      }
    };
    
    auto deployLookups = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "lookup", true);
        return;
      }
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
            auto op = workload[index];
            auto root = lct.findRoot(nodes[op.first]);
            if (root->label != nodes[op.second]->label)
              std::cerr << "op=(" << op.first << "," << op.second << ") root=" << root->label << " vs " << nodes[op.second]->label << std::endl;
            assert(root->label == nodes[op.second]->label);
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
      } else if (elem.first == 2) {
        deployCuts(currIndex, currIndex + elem.second);
      } else {
        deployLookups(currIndex, currIndex + elem.second);
      }
      currIndex += elem.second;
    } while (currIndex != workload.size());
  };

  try {
    std::cerr << "Check for correctness.." << std::endl;
    for (unsigned index = 0; index != 10; ++index) {
      checkForCorrectness();
    }
  } catch (...) {
    std::cerr << "Correctness test failed!" << std::endl;
    assert(0);
  }

  auto benchmark = [&]() -> double {
    std::vector<NodeType*> nodes(n); 
    TreeType lct(n, nodes);
    for (unsigned index = 0; index != n; ++index) {
      nodes[index] = new NodeType();
      nodes[index]->label = index;
    }
    
    auto deployCuts = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "cut");
        return;
      }
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
            lct.cut(nodes[workload[index].first]);
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
    
    auto deployLinks = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "link");
        return;
      }
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
      std::vector<std::thread> threads;
      for (unsigned index = 0, limit = num_threads; index != limit; ++index) {
        threads.emplace_back(consume);
      }
      for (auto& thread : threads) {
        thread.join();
      }
    };
    
    auto deployLookups = [&](unsigned lb, unsigned ub) {
      unsigned taskSize = (ub - lb) / (task_factor * num_threads);
      if (!taskSize) {
        sequential(lct, nodes, lb, ub, "lookup");
        return;
      }
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
    
    
    std::cerr << "Start workload.." << std::endl;
    auto start = high_resolution_clock::now();
    unsigned currIndex = 0;
    do {
      auto elem = workload[currIndex];
      ++currIndex;

      if (elem.first == 1) {
        deployLinks(currIndex, currIndex + elem.second);
      } else if (elem.first == 2) {
        deployCuts(currIndex, currIndex + elem.second);
      } else {
        deployLookups(currIndex, currIndex + elem.second);
      }
      currIndex += elem.second;
    } while (currIndex != workload.size());
    auto stop = high_resolution_clock::now();
    std::cerr << "Finished workload!" << std::endl;
    return duration_cast<milliseconds>(stop - start).count();
  };
  
  try {
    auto time = benchmark();std::cerr << "Benchmark: " << time << " ms" << std::endl;
    return time;
  } catch (...) {
    std::cerr << "Benchmark failed!" << std::endl;
    assert(0);
  }
  return 0;
}

double lookup_benchmark(std::string filename, unsigned n, unsigned num_threads, unsigned task_factor, unsigned lock_coupling) {
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

  std::cerr << "---------------- New benchmark (lock_coupling=" << lock_coupling << ") ----------------" << std::endl;
  return (!lock_coupling) ? lookup_benchmark_lct<ConcurrentLinkCutTrees, ConcurrentLinkCutTrees::CoNode>(n, num_threads, task_factor, workload)
                          : lookup_benchmark_lct<LockCouplingLinkCutTrees, LockCouplingLinkCutTrees::CoNode>(n, num_threads, task_factor, workload);
}

double cut_benchmark(std::string filename, unsigned n, unsigned num_threads, unsigned task_factor, unsigned lock_coupling) {
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

  std::cerr << "---------------- New benchmark (lock_coupling=" << lock_coupling << ") ----------------" << std::endl;
  return (!lock_coupling) ? cut_benchmark_lct<ConcurrentLinkCutTrees, ConcurrentLinkCutTrees::CoNode>(n, num_threads, task_factor, workload)
                          : cut_benchmark_lct<LockCouplingLinkCutTrees, LockCouplingLinkCutTrees::CoNode>(n, num_threads, task_factor, workload);
}

void benchmark(std::string filename, unsigned num_threads, unsigned task_factor, unsigned lock_coupling = 0) { 
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

  std::cerr << "Start benchmarking \"" << type << " (" << std::to_string(n) << ")\"" << std::endl;
  double time = 0;
  if (type == "cut") {
    time = cut_benchmark(filename, n, num_threads, task_factor, lock_coupling);
  } else if (type == "lookup") {
    time = lookup_benchmark(filename, n, num_threads, task_factor, lock_coupling);
  } else {
    std::cerr << "Not supported yet!" << std::endl;
    exit(-1);
  }

  std::ofstream log("../logs/" + type + "-p_1" + "-w_" + w + "-b_" + b + "-n_" + std::to_string(n) + "-t_" + std::to_string(num_threads) + "-f_" + std::to_string(task_factor) + "-l_" + std::to_string(lock_coupling) + ".log");
  log << time << " ms" << std::endl;
  log.close();
}

int main(int argc, char** argv) {
  if ((argc != 4) && (argc != 5)) {
    std::cerr << "Usage: " << argv[0] << " <workload:file> <num_threads:unsigned> <task_factor:unsigned> [<lock-coupling:bool>]" << std::endl;
    exit(-1);
  }
  auto lock_coupling = (argc == 5) ? atoi(argv[4]) : 0;
  benchmark(argv[1], atoi(argv[2]), atoi(argv[3]), lock_coupling);
}