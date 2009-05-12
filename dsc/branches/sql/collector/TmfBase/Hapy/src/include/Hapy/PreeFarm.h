/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PREE_FARM__H
#define HAPY_PREE_FARM__H

#include <Hapy/Top.h>

namespace Hapy {

class Pree;

// a farm for generating and recycling nodes of the parsing tree
class PreeFarm {
	public:
		static Pree *Get();
		static void Put(Pree *p);
		static void Clear();

	public:
		typedef unsigned long int Count;
		static Count TheGetCount; // number of Get()s
		static Count TheNewCount; // number of Get()s that created a node
		static Count ThePutCount; // number of Put()s

	private:
		typedef Pree* Store;
		static Store TheStore;
};


} // namespace

#endif

