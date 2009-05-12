/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PREE_KIDS__H
#define HAPY_PREE_KIDS__H

#include <Hapy/Top.h>
#include <Hapy/Iterator.h>

namespace Hapy {

class Pree;

// user-level iterator to iterate through Pree or const Pree kids
template <class Base>
class PreeKidsIterator: public std_bidirectional_iterator<Base, unsigned int> {
	public:
		typedef typename std::iterator_traits<PreeKidsIterator>::iterator_category iterator_category;
		typedef typename std::iterator_traits<PreeKidsIterator>::difference_type difference_type;
		typedef typename std::iterator_traits<PreeKidsIterator>::value_type value_type;
		typedef typename std::iterator_traits<PreeKidsIterator>::pointer pointer;
		typedef typename std::iterator_traits<PreeKidsIterator>::reference reference;

	public:
		PreeKidsIterator(): current(0), idx(0) {}
		explicit PreeKidsIterator(Pree *start, difference_type offset):
			current(start), idx(offset) {}

		reference operator*() const {
			return *current;
		}

		pointer operator->() const { return &(operator*()); }

		PreeKidsIterator& operator++() {
			current = current->right;
			++idx;
			return *this;
		}
		PreeKidsIterator operator++(int) {
			PreeKidsIterator tmp = *this;
			++(*this);
			return tmp;
		}
		PreeKidsIterator& operator--() {
			current = current->left;
			--idx;
			return *this;
		}
		PreeKidsIterator operator--(int) {
			PreeKidsIterator tmp = *this;
			--(*this);
			return tmp;
		}

		PreeKidsIterator operator+(difference_type n) const {
			PreeKidsIterator tmp = *this;
			return tmp += n;
		}

		PreeKidsIterator& operator+=(difference_type n) {
			while (n-- > 0)
				++(*this);
			return *this;
		}

		PreeKidsIterator operator-(difference_type n) const {
			PreeKidsIterator tmp = *this;
			return tmp -= n;
		}

		PreeKidsIterator& operator-=(difference_type n) {
			while (n-- > 0)
				--(*this);
			return *this;
		}

		reference operator[](difference_type n) const {
			return *(*this + n);
		}

	public:
		pointer current;
		difference_type idx;
}; 

template <class Base>
inline
bool operator==(const PreeKidsIterator<Base>& x, 
	const PreeKidsIterator<Base>& y) {
	return x.idx == y.idx;
}

template <class Base>
inline
bool operator!=(const PreeKidsIterator<Base>& x, 
	const PreeKidsIterator<Base>& y) {
	return !(x == y);
}

template <class Base>
inline
bool operator<(const PreeKidsIterator<Base>& x, 
	const PreeKidsIterator<Base>& y) {
	return x.idx < y.idx;
}

template <class Base>
inline
typename PreeKidsIterator<Base>::difference_type operator-(const PreeKidsIterator<Base>& x,
	const PreeKidsIterator<Base>& y) {
	return x.idx - y.idx;
}

template <class Base>
inline
PreeKidsIterator<Base> operator+(typename PreeKidsIterator<Base>::difference_type n,
	const PreeKidsIterator<Base>& x) {
	return x + n;
}

} // namespace

#endif

