/* Hapy is a public domain software. See Hapy README file for the details. */

#ifndef HAPY_PREE_KIDS__H
#define HAPY_PREE_KIDS__H

#include <Hapy/config.h>
#include <list>
#include <vector>

namespace Hapy {

class Pree;
typedef std::vector<Pree*> PreeKids;

// user-level iterator to hide internal pointer-based pree node representation
template <class Iterator>
class DerefIterator {
	public:
		typedef typename std::iterator_traits<Iterator>::iterator_category iterator_category;
		typedef Pree &value_type;
		typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
		typedef Pree *pointer;
		typedef Pree &reference;

		typedef Iterator iterator_type;
		typedef DerefIterator<Iterator> Self;

	public:
		DerefIterator() {}
		explicit DerefIterator(iterator_type x) : current(x) {}

		DerefIterator(const Self& x) : current(x.current) {}
			
		iterator_type base() const { return current; }
		reference operator*() const {
			return *(*current);
		}

		pointer operator->() const { return &(operator*()); }

		Self& operator++() {
			++current;
			return *this;
		}
		Self operator++(int) {
			Self tmp = *this;
			++(*this);
			return tmp;
		}
		Self& operator--() {
			--current;
			return *this;
		}
		Self operator--(int) {
			Self tmp = *this;
			--(*this);
			return tmp;
		}

		Self operator+(difference_type n) const {
			return Self(current + n);
		}

		Self& operator+=(difference_type n) {
			current += n;
			return *this;
		}

		Self operator-(difference_type n) const {
			return Self(current - n);
		}

		Self& operator-=(difference_type n) {
			current -= n;
			return *this;
		}

		reference operator[](difference_type n) const {
			return *(*this + n);
		}

	protected:
		Iterator current;
}; 
 
template <class Iterator>
inline
bool operator==(const DerefIterator<Iterator>& x, 
	const DerefIterator<Iterator>& y) {
	return x.base() == y.base();
}

template <class Iterator>
inline
bool operator!=(const DerefIterator<Iterator>& x, 
	const DerefIterator<Iterator>& y) {
	return !(x.base() == y.base());
}

template <class Iterator>
inline
bool operator<(const DerefIterator<Iterator>& x, 
	const DerefIterator<Iterator>& y) {
	return x.base() < y.base();
}

template <class Iterator>
inline
typename DerefIterator<Iterator>::difference_type operator-(const DerefIterator<Iterator>& x, 
	const DerefIterator<Iterator>& y) {
	return x.base() - y.base();
}

template <class Iterator>
inline
DerefIterator<Iterator> operator+(typename DerefIterator<Iterator>::difference_type n,
	const DerefIterator<Iterator>& x) {
	return DerefIterator<Iterator>(x.base() + n);
}


// a farm for parsing tree nodes
class PreeFarm {
	public:
		static Pree *Get();
		static void Put(Pree *p);
		static void Clear();

	public:
		typedef int Level;
		static Level TheInLevel;
		static Level TheOutLevel;

	private:
		typedef Pree* Store;
		static Store TheStore;
};


} // namespace

#endif

