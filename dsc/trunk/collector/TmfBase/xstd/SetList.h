 /* Made at The Measurement Factory     ::     measurement-factory.com */

#ifndef TMF_BASE__XSTD_SET_LIST__H
#define TMF_BASE__XSTD_SET_LIST__H

#include <hash_set>
#include <list>

template <class Item>
class SetList {
	public:
		inline bool insert(const Item &item);

	public:
		hash_set<Item> set;
		list<Item> list;
};

template <class Item>
inline
bool SetList<Item>::insert(const Item &item) {
	if (set.find(item) != set.end())
		return false;

	list.push_back(item);
	set.insert(item);
	return true;
}

#endif
