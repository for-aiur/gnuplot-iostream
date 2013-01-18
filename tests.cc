/*
Copyright (c) 2009 Daniel Stahlke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <fstream>
#include <vector>
#include <math.h>

#include <boost/utility.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/and.hpp>

#include "gnuplot-iostream.h"

template <class T>
typename boost::enable_if_c<GnuplotEntry<T>::is_tuple, void>::type
foo(const T &) {
	std::cout << "tuple" << std::endl;
}

//template <class T>
//typename boost::enable_if_c<!GnuplotEntry<T>::is_tuple, void>::type
//foo(const T &) {
//	std::cout << "tuple" << std::endl;
//}

template <class T>
typename boost::enable_if_c<GnuplotEntry<typename T::value_type::value_type>::is_tuple, void>::type
foo(const T &) {
	std::cout << "vec vec tuple" << std::endl;
}

template <class T>
typename boost::enable_if_c<!GnuplotEntry<typename T::value_type::value_type>::is_tuple, void>::type
foo(const T &) {
	std::cout << "vec vec scalar" << std::endl;
}

template <typename T>
class is_container {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::value_type *, typename C::const_iterator *);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(NULL, NULL)) == sizeof(char) };
};

template <class T, size_t N>
class is_container<T[N]> {
public:
	enum { value = true };
};

template <class T>
typename boost::enable_if<is_container<T> >::type
a(const T &) {
	std::cout << "a:cont" << std::endl;
}

template <class T>
typename boost::enable_if_c<!is_container<T>::value>::type
a(const T &) {
	std::cout << "a:flat" << std::endl;
}

template <class T>
std::pair<typename T::const_iterator, typename T::const_iterator>
get_iter_pair(const T &arg) {
	return std::make_pair(arg.begin(), arg.end());
}

template <class T, size_t N>
std::pair<T *, T *>
get_iter_pair(T (&arr)[N]) {
	return std::make_pair(arr, arr+N);
}

template <class T, size_t N>
std::pair<const T *, const T *>
get_iter_pair(const T (&arr)[N]) {
	return std::make_pair(arr, arr+N);
}

template <class T>
class IterTypeGetter {
public:
	typedef typename T::const_iterator iter_type;
};

template <class T, size_t N>
class IterTypeGetter<T[N]> {
public:
	typedef T* iter_type;
};

template <class T, size_t N>
class IterTypeGetter<const T[N]> {
public:
	typedef const T* iter_type;
};

template <class T>
class ValTypeGetter {
public:
	typedef typename T::value_type val_type;
};

template <class T>
class ValTypeGetter<T*> {
public:
	typedef T val_type;
};

template <class T, class Enable=void>
class RangeVecBase : public std::vector<std::pair<T, T> > {
public:
	void inc() {
		for(size_t i=0; i<this->size(); i++) {
			++(this->at(i).first);
		}
	}

	bool is_done() {
		return this->empty() || (this->at(0).first == this->at(0).second);
	}
};

template <class T, class Enable=void>
class RangeVec : public RangeVecBase<T> { };

template <class T>
class RangeVec<T, typename boost::enable_if<is_container<typename T::value_type> >::type> : public RangeVecBase<T> {
public:
	RangeVec<typename ValTypeGetter<T>::val_type::const_iterator> deref() {
		RangeVec<typename ValTypeGetter<T>::val_type::const_iterator> ret;
		for(size_t i=0; i<this->size(); i++) {
			ret.push_back(get_iter_pair(*(this->at(i).first)));
		}
		return ret;
	}
};

//template <class T, size_t N>
//class RangeVec<T[N]> : public RangeVecBase<T[N]> {
//public:
//	RangeVec<typename IterTypeGetter<T[N]>::iter_type> deref() {
//		RangeVec<typename IterTypeGetter<T[N]>::iter_type> ret;
//		for(size_t i=0; i<this->size(); i++) {
//			ret.push_back(get_iter_pair(*(this->at(i).first)));
//		}
//		return ret;
//	}
//};

template <class T>
RangeVec<typename IterTypeGetter<typename T::value_type>::iter_type >
get_range_vec(const T &arg) {
	RangeVec<typename IterTypeGetter<typename T::value_type>::iter_type > ret;
	// FIXME - why doesn't cbegin/cend compile?
	for(typename T::const_iterator outer_it=arg.begin(); outer_it != arg.end(); ++outer_it) {
		ret.push_back(get_iter_pair(*outer_it));
	}
	return ret;
}

template <class T, size_t N>
RangeVec<typename IterTypeGetter<T>::iter_type>
get_range_vec(T (&arr)[N]) {
	RangeVec<typename IterTypeGetter<T>::iter_type> ret;
	for(size_t i=0; i<N; i++) {
		ret.push_back(get_iter_pair(arr[i]));
	}
	return ret;
}

template <class T>
void send_CL_rvec(RangeVec<T> arg) {
	if(arg.empty()) return;
	while(!arg.is_done()) {
		for(size_t i=0; i<arg.size(); i++) {
			if(i) std::cout << " ";
			GnuplotEntry<typename ValTypeGetter<T>::val_type>::send(std::cout, *arg[i].first);
		}
		arg.inc();
		std::cout << "\n";
	}
}

template <class T>
void send_CL(const T &arg) {
	send_CL_rvec(get_range_vec(arg));
}

template <class T>
void send_CBL_rvec(RangeVec<T> arg) {
	if(arg.empty()) return;
	while(!arg.is_done()) {
		//RangeVec<typename T::value_type::const_iterator> inner = arg.deref();
		RangeVec<typename IterTypeGetter<typename ValTypeGetter<T>::val_type>::iter_type> inner = arg.deref();
		send_CL_rvec(inner);
		arg.inc();
		std::cout << "\n";
	}
}

template <class T>
void send_CBL(const T &arg) {
	send_CBL_rvec(get_range_vec(arg));
}

template <class T>
void send_BL(T &arg) { // FIXME - const
	//std::vector<T> cols;
	//cols.push_back(arg);
	//send_CBL(cols);
	RangeVec<typename IterTypeGetter<T>::iter_type> cols;
	cols.push_back(get_iter_pair(arg));
	send_CBL_rvec(cols);
}

int main() {
	//Gnuplot gp("cat");

	const int NX=3, NY=4;

	double s = 0.0;
	std::pair<double, double> t;
	std::vector<double> vs;
	std::vector<std::pair<double, double> > vt;
	std::vector<std::vector<double> > cvs(2);
	std::vector<std::vector<double> > vvs(NX);
	std::vector<std::vector<std::pair<double, double> > > vvt(NX);
	std::vector<std::vector<std::vector<double> > > cvvs(2);

	cvvs[0].resize(NX);
	cvvs[1].resize(NX);

	for(int x=0; x<NX; x++) {
		vs.push_back(x);
		vt.push_back(std::make_pair(100+x, 200+x));
		cvs[0].push_back(100+x);
		cvs[1].push_back(200+x);
		for(int y=0; y<NY; y++) {
			vvs[x].push_back(x*10+y);
			vvt[x].push_back(std::make_pair(100+x*10+y, 200+x*10+y));
			cvvs[0][x].push_back(100+x*10+y);
			cvvs[1][x].push_back(200+x*10+y);
		}
	}

	std::vector<double> cols[2] = { cvs[0], cvs[1] };
	int aa[2][3] = {{1,2,3},{4,5,6}};
	//std::vector<int[3]> av(1);
	//for(int i=0; i<3; i++) av[0][i] = i;

	//gp.send(scalar_array);
	//gp.send(tuple_array);


	foo(vvs);
	foo(vvt);
	//foo(0.0);
	foo(t);
	//foo(std::vector<double>());

	a(s);
	a(vs);
	a(cols);
	a(aa);

	std::cout << "hb=" << is_container<double>::value << std::endl;
	std::cout << "hb=" << is_container<std::pair<double, double> >::value << std::endl;
	std::cout << "hb=" << is_container<std::vector<double> >::value << std::endl;

	send_CL(cvs);
	std::cout << "e" << std::endl;
	send_CBL(cvvs);
	std::cout << "e" << std::endl;
	send_CL(cols);
	std::cout << "e" << std::endl;
	send_CL(aa);
	std::cout << "e" << std::endl;
	send_BL(cvs);
	std::cout << "e" << std::endl;
	send_BL(aa);
	std::cout << "e" << std::endl;
}
