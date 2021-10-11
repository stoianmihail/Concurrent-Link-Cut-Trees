#ifndef H_UnionFind
#define H_UnionFind
//---------------------------------------------------------------------------
#include <vector>
//---------------------------------------------------------------------------
// (c) 2017 Thomas Neumann
//---------------------------------------------------------------------------
/// A union-find data structure that maps dense integers to representatives
class UnionFind {
   /// An entry
   struct Entry {
      /// The representative+1 if set, otherwise 0
      unsigned representative = 0;
      /// The rank of the set
      unsigned rank = 0;

      /// Does the entry have an representative
      bool hasRepresentative() const { return representative; }
      /// Get the representative
      unsigned getRepresentative() const { return representative-1; }
      /// Set the representative
      void setRepresentative(unsigned rep) { representative=rep+1; }
   };
   /// All entries
   std::vector<Entry> entries;

   public:
   /// Constructor
   explicit UnionFind(unsigned size);
   /// Destructor
   ~UnionFind();

   /// Union two sets. Returns the new set id
   unsigned unionSets(unsigned entry1,unsigned entry2);
   /// Find the representative for a given entry
   unsigned find(unsigned entry);
   /// Check for union.
   bool areUnion(unsigned u, unsigned v) {
     return find(u) == find(v);
   }
};
//---------------------------------------------------------------------------
#endif
