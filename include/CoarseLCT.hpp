#ifndef PLCT_HPP
#define PLCT_HPP
#include <assert.h>
#include <iostream>
#include <mutex>

#define DEBUG 0

// Inspired from: https://github.com/indy256/codelibrary/blob/master/java/structures/LinkCutTree.java
class ParallelLinkCutTree {
public:
  class Node {
    friend class ParallelLinkCutTree;
    Node* left = nullptr;
    Node* right = nullptr;
    Node* parent = nullptr;
    bool revert;
    
    bool isRoot() {
      // The first check refers to general root.
      // The second check refers to a local root.
      // Thus, `parent` is overloaded by the parent pointer in the actual tree.
      // It represents at the same the parent within a splay tree and the parent pointer in the actual tree.
      return (parent == nullptr) || ((parent->left != this) && (parent->right != this));
    }
  public:
    uint64_t value;
  };

private:
  std::mutex latch;
     
  void printBT(const std::string& prefix, const Node* node, bool isLeft) {
    if (node != nullptr)
        {
            std::cout << prefix;

            std::cout << (isLeft ? "├──" : "└──" );

            // print the value of the node
            std::cout << node->value << std::endl;

            // enter the next tree level - left and right branch
            printBT( prefix + (isLeft ? "│   " : "    "), node->left, true);
            printBT( prefix + (isLeft ? "│   " : "    "), node->right, false);
        }
    }

    void printBT(const Node* node)
    {
        printBT("", node, false);    
    }
  
  // rotates edge (x, x.parent)
  //        g            g
  //       /            /
  //      p            x
  //     / \    ->    / \
  //    x  p.r      x.l  p
  //   / \              / \
  // x.l x.r          x.r p.r
  void rotate(Node* x) {
    Node* p = x->parent;
    Node* g = p->parent;
    bool isPRoot = p->isRoot();
    bool isXLeftChild = (x == p->left);
    
    // Create 3 edges: (x.r(l), p), (p, x) and (x, g)
    if (isXLeftChild) {
      if (x->right)
        x->right->parent = p;
      p->left = x->right;
    } else {
      if (x->left)
        x->left->parent = p;
      p->right = x->left;
    }
    
    p->parent = x;
    if (!isXLeftChild)
      x->left = p;
    else
      x->right = p;
    x->parent = g;
    if (!isPRoot) {
      if (p == g->left)
        g->left = x;
      else
        g->right = x;
    }
  }
  
  // brings x to the root, balancing tree
  //
  // zig-zig case
  //        g                                  x
  //       / \               p                / \
  //      p  g.r rot(p)    /   \     rot(x) x.l  p
  //     / \      -->    x       g    -->       / \
  //    x  p.r          / \     / \           x.r  g
  //   / \            x.l x.r p.r g.r             / \
  // x.l x.r                                    p.r g.r
  //
  // zig-zag case
  //      g               g
  //     / \             / \               x
  //    p  g.r rot(x)   x  g.r rot(x)    /   \
  //   / \      -->    / \      -->    p       g
  // p.l  x           p  x.r          / \     / \
  //     / \         / \            p.l x.l x.r g.r
  //   x.l x.r     p.l x.l
  void splay(Node* x) {
#if DEBUG
    std::cerr << "\t[splay start] node=" << x->value << std::endl;
#endif
    while (!x->isRoot()) { 
      Node* p = x->parent;
      Node* g = p->parent;
#if DEBUG
      std::cerr << "\t\t[splay-inside] p=" << p->value << std::endl;
      std::cerr << "\t\t[splay-inside] parent before rotation" << std::endl;
      printBT(p);
#endif
      if (!p->isRoot()) {
        rotate(((x == p->left) == (p == g->left)) ? p /* zig-zig case */ : x /* zig-zag case */);
      }
      rotate(x);
#if DEBUG
      std::cerr << "\t\t[splay-inside] after rotation:" << std::endl;
      printBT(x);
#endif
    }
#if DEBUG
    std::cerr << "\t[splay finish]" << std::endl;
#endif
  }
  
  Node* expose(Node* x) {
    Node* last = nullptr;
#if DEBUG
    std::cerr << "[expose] x=" << x->value << std::endl;
#endif
    for (Node* y = x; y; y = y->parent) {
#if DEBUG
      std::cerr << "[expose] splay y=" << y->value << " parent=" << (y->parent ? std::to_string(y->parent->value) : "nullptr") << std::endl;
#endif
      splay(y);
      y->left = last;
      last = y;
    }
    // This splay can be also done by rotating `x` in each iteration.
    // TOode=7DO: maybe better for prl!
#if DEBUG
    std::cerr << "[expose] last splay x=" << x->value << std::endl;
#endif
    splay(x);
    return last;
  }
  
public:
  void link(Node* x, Node* y) {
    std::unique_lock lock(latch);
    auto rx = findRoot(x), ry = findRoot(y);
    //assert(rx != ry);
    if (rx == ry) {
      return;
    }
#if DEBUG
    std::cerr << "%% [link] x=" << x->value << " y=" << y->value << std::endl;
#endif
    expose(x);
    
#if DEBUG
    printBT(x);
    printBT(y);
#endif
    
    // TODO: what if x or y are erased?
    // x must be a root node.
    assert(!x->right);
    x->parent = y;
  }
  
  void cut(Node* x) {
    // Delete `v` from its parent.
    expose(x);
    assert(!!x->right);
    x->right->parent = nullptr;
    x->right = nullptr;
  }
  
  Node* findRoot(Node* x) {
#if DEBUG
    std::cerr << "&&& [findRoot] x=" << x->value << std::endl;
#endif
    // Find the root `r` of `v`.
    expose(x);
    while (x->right) x = x->right;
    
    // Amortized cost.
#if DEBUG
    std::cerr << "+++ [findRoot] final splay x=" << x->value << std::endl;
#endif
    splay(x);
    return x;
  }
  
  bool areConnected(Node* x, Node* y) {
    std::unique_lock lock(latch);
    return findRoot(x) == findRoot(y);
  }
};
#endif
