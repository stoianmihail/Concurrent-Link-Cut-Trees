#include <assert.h>
#include <mutex>

// `Concurrent Link-Cut Trees` - Mihail Stoian, 2021.
// The implementation is adapted from: https://github.com/indy256/codelibrary/blob/master/java/structures/LinkCutTree.java.
class ConcurrentLinkCutTrees {
public:
  class CoNode {
    friend class ConcurrentLinkCutTrees;
    // The left child.
    CoNode* left = nullptr;
    // The right child.
    CoNode* right = nullptr;
    // The parent.
    CoNode* parent = nullptr;
    // The latch.
    std::mutex latch;
    
    bool isRoot() {
      // The first check is referring to the general root.
      // The second check is referring to a local root.
      // Thus, `parent` is overloaded by the parent pointer in the actual tree.
      // It represents at the same the parent within a splay tree and the parent pointer in the actual tree.
      return (parent == nullptr) || ((parent->right != this) && (parent->left != this));
    }
  public:
    // The label.
    unsigned label;
  };

  // The π-array.
  std::vector<unsigned> pi_;
  // The nodes.
  std::vector<CoNode*>& nodes_;
  
  ConcurrentLinkCutTrees(unsigned n, std::vector<CoNode*>& nodes) : nodes_(nodes)
  // The constructor.
  {
    pi_.resize(n);
    for (unsigned index = 0; index != n; ++index)
      pi_[index] = index;
  }

  // Rotates edge (`x`, `x.parent`)
  //        g            g
  //       /            /
  //      p            x
  //     / \    ->    / \
  //    x  p.r      x.l  p
  //   / \              / \
  // x.l x.r          x.r p.r
  void rotate(CoNode* x) {
  // Rotate.
    CoNode* p = x->parent;
    CoNode* g = p->parent;
    bool isPRoot = p->isRoot();
    bool isXRightChild = (x == p->right);
    
    // Create 3 edges: (x.r(l), p), (p, x) and (x, g)
    if (isXRightChild) {
      if (x->left)
        x->left->parent = p;
      p->right = x->left;
    } else {
      if (x->right)
        x->right->parent = p;
      p->left = x->right;
    }
    
    p->parent = x;
    if (!isXRightChild)
      x->right = p;
    else
      x->left = p;
    x->parent = g;
    if (!isPRoot) {
      if (p == g->right)
        g->right = x;
      else
        g->left = x;
    }
  }
  
  // Brings `x` to the root, balancing the tree.
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
  void splay(CoNode* x) {
  // Splay.
    while (!x->isRoot()) { 
      CoNode* p = x->parent;
      CoNode* g = p->parent;
      if (!p->isRoot()) {
        rotate(((x == p->right) == (p == g->right)) ? p /* zig-zig case */ : x /* zig-zag case */);
      }
      rotate(x);
    }
  }
  
  void unlinkInPiArray(unsigned c) {
  // Unlink the preferred path of `c`, whose representative is `c` itself.
    pi_[c] = c;
  }
  
  void linkInPiArray(unsigned c, unsigned p) {
  // Link the preferred path of `c`, whose representative is `c` itself, to node `p`.
    pi_[c] = p;
  }

  unsigned getRepr(CoNode* node) {
  // Fetch the representative of the preferred path of `node`.
    unsigned x = node->label;
    while (x != pi_[x]) {
      auto prev = x;
      x = pi_[x];
      
      // It could be the case that it rapidly changes.
      // This can happen when we *split* the splay trees.
      if (x == prev) return x;
    }
    return x;
  }
  
  std::vector<unsigned> pathExpose(CoNode* x) {
    CoNode* last = nullptr;
    std::vector<unsigned> trace;
    for (CoNode* y = x; y; last = y, y = y->parent) {
      unsigned repr = getRepr(y);
      restart: {
        nodes_[repr]->latch.lock();
        auto newRepr = getRepr(y);
        if (repr != newRepr) {
          nodes_[repr]->latch.unlock();
          repr = newRepr;
          goto restart;
        }
      };
      
      // Splay `y`.
      splay(y);

      // Does it have a lower path?
      if (y->right) {
        // Find its representant - O(log2(n))-operation.
        auto tmp = y->right;
        while (tmp->left)
          tmp = tmp->left;
        
        // Cut the link.
        auto reprOfPath = tmp->label;
        
        // At this point we cut the splay subtree.
        // So, at a later point, a thread is able to lock it.
        // Thus, the order of instructions matters!
        y->right = nullptr;
        unlinkInPiArray(reprOfPath);
      }
      
      // Redirect the preferred path.
      // The splay tree of `last` is already locked.
      // Thus, this is a safe operation. It could also be done after `link`.
      y->right = last;
      if (last) {
        assert(!trace.empty());
        linkInPiArray(trace.back(), y->label);
      }
      
      trace.push_back(repr);
    }
    
    // Finally, splay `x`.
    splay(x);
    
    // And return the trace.
    return std::move(trace);
  }
  
  void unlockTrace(std::vector<unsigned>& trace) {
  // Unlock the trace.
    for (unsigned index = 0, limit = trace.size(); index != limit; ++index)
      nodes_[trace[limit - index - 1]]->latch.unlock();
  }
  
  void link(CoNode* x, CoNode* y) {
    auto trace = pathExpose(x);
    
    // `x` must be a root node.
    assert(!x->left);
    x->parent = y;

    // And unlock the trace.
    unlockTrace(trace);
  }
  
  void cut(CoNode* x) {
    auto trace = pathExpose(x);
    
    // Delete `x` from its parent.
    assert(!!x->left);
    x->left->parent = nullptr;
    x->left = nullptr;
    unlinkInPiArray(x->label);
    
    // And unlock the trace.
    unlockTrace(trace);
  }

  CoNode* findRoot(CoNode* x) {
    auto trace = pathExpose(x);
    
    // Find the root.
    while (x->left) x = x->left;
    
    // Amortized cost.
    splay(x);
    
    // And unlock the trace.
    unlockTrace(trace);
    return x;
  }
};
