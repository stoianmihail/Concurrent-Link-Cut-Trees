#pragma
#include <vector>

class UnionFind {
   /// The roots.
   std::vector<unsigned> boss;

   public:
   /// The constructor.
   explicit UnionFind(unsigned size) {
      boss.resize(size);
      for (unsigned index = 0; index != size; ++index)
         boss[index] = index;
   }
   
   /// The destructor.
   ~UnionFind() {}

   /// Unify two sets.
   void unify(unsigned u, unsigned v) {
      unsigned ru = find(u), rv = find(v);

      // Already in the same set?
      if (ru == rv)
         return;

      boss[rv] = ru;
   }
   
   /// Find the root.
   unsigned find(unsigned u) {
      auto r = u;
      while (r != boss[r]) {
         r = boss[r];
      }
      while (u != boss[u]) {
         auto tmp = boss[u];
         boss[u] = r;
         u = tmp; 
      }
      return r;
   }

   /// Check for union.
   bool areConnected(unsigned u, unsigned v) {
     return find(u) == find(v);
   }
};