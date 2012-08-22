template<class TYPE>
struct MySort {
  MySort(int l, const vector<TYPE>& v) : l(l), v(v) {}

  bool operator()(const int& i1, const int& i2) const {
    for (int i=0; i<l; i++) {
      if (i1+i == l) return false;
      if (i2+i == l) return true;
      TYPE c1 = v[i1+i];
      TYPE c2 = v[i2+i];
      if (c1 != c2) return c1<c2;
    }
    return false;
  }

private:
  int l;
  const vector<TYPE> &v;
};

template<class TYPE>
vector<TYPE> bw_transform(const vector<TYPE> &data, int& special_pos) {
  int l = data.size();
  vector<int> ref(l+1);
  for (int i=0; i<l+1; i++) ref[i]=i;
  stable_sort(ref.begin(), ref.end(), MySort<TYPE>(l, data));

  vector<TYPE> result(l+1);
  for (int i=0; i<l+1; i++) {
    if (ref[i]) {
      result[i] = data[ref[i]-1];
    } else {
      special_pos = i;
      result[i] = '#';
    }
  }
  return result;
}

template<class TYPE>
struct MySort2 {
  MySort2() {};
  bool operator()(const pair<TYPE, int>& e1, const pair<TYPE, int>& e2) const {
    return e1.first < e2.first;
  }
};

template<class TYPE>
vector<TYPE> bw_inverse(const vector<TYPE> &data, int special_pos) {
  int l = data.size()-1;
  vector<pair<TYPE, int> > sorted(l+1);
  for (int i=0; i<l+1; i++)
    sorted[i] = pair<TYPE, int>(data[i], i);

  for (int i=special_pos; i<l; i++)
    sorted[i] = sorted[i+1];
  sorted.pop_back();
  stable_sort(sorted.begin(), sorted.end(), MySort2<TYPE>());
  
  //for (int i=0; i<l; i++) cout << sorted[i].first << " - " << sorted[i].second << "\n";

  vector<TYPE> result(l);
  int index = special_pos;
  for (int i=0; i<l; i++) {
    result[i] = sorted[index].first;
    index = sorted[index].second;
  }
  return result;
}
