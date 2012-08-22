#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;

template<class TYPE>
vector<TYPE> bw_transform(const vector<TYPE> &data, int& special_pos);

template<class TYPE>
vector<TYPE> bw_inverse(const vector<TYPE> &data, int special_pos);

#include "burrows_wheeler_templ.hxx"
