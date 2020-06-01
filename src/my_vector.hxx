#ifndef _MY_VECTOR_
#define _MY_VECTOR_

#include <stdlib.h>
#include <assert.h>

extern int num_in_existance;

template<class TYPE>
class MyVector {
public:
  MyVector(int initial_size = 1) : _allocated_size(initial_size), _size(0) {
    //cerr << "Creating MyVector[" << _allocated_size << "], exists " << ++num_in_existance << "\n";
    assert(initial_size > 0);
    mem = new TYPE[_allocated_size];
  }
  ~MyVector() {
    if (mem) {
      //cerr << "Deleting MyVector[" << _allocated_size << "], exists " << --num_in_existance << "\n";
      delete[] mem;
    }
  }

  // Conserves the content of the vector
  void resize(int new_allocated_size) {
    if (new_allocated_size == _allocated_size) return;

    assert(_size <= new_allocated_size);
    TYPE *new_mem = new TYPE[new_allocated_size];
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    memcpy(new_mem, mem, sizeof(TYPE)*_size);
#pragma GCC diagnostic pop
    delete[] mem;
    mem = new_mem;

    _allocated_size = new_allocated_size;
  }

  void resize_if_smaller_than(int min_allocated_size) {
    if (_allocated_size < min_allocated_size)
      resize(min_allocated_size);
  }

  TYPE& operator[](int index) {
    assert(mem);
    assert(0<=index && index<_size);
    return mem[index];
  }

  int push_back(TYPE value) {
    assert(mem);
    if (_allocated_size == _size) resize(_allocated_size << 1);
    mem[_size] = value;
    return _size++;
  }
  int add(TYPE value) { return push_back(value); }

  void remove(int index) {
    assert(mem);
    assert(0<=index && index<_size);
    mem[index] = mem[--_size];
  }

  void clear() { assert(mem); _size = 0; }
  int size() { assert(mem); return _size; }
  int allocated_size() { assert(mem); return _allocated_size; }

  void sort(int (*cmp)(const void *, const void *)) {
    assert(mem);
    qsort(mem, _size, sizeof(TYPE), cmp);
  }
private:
  int _allocated_size, _size;
  TYPE *mem;

  // Private to prevent copying:
  MyVector(const MyVector&);
  MyVector& operator=(const MyVector&);
};
#endif
