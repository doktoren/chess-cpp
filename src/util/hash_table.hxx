#ifndef _HASH_TABLE_
#define _HASH_TABLE_

#include <iostream>

#include "../typedefs.hxx"

using namespace std;

struct Example_HashValue {
  uint hash_value;
  bool operator==(const Example_HashValue& h2) {
    return hash_value==h2.hash_value;
  }
  // max_index will always be on the form (2^n)-1
  int get_index(uint max_index) const {
    return hash_value & max_index;
  }
};
ostream& operator<<(ostream& os, const Example_HashValue& hash_value);


struct Example_Content {
  Example_Content() : count(0) {}
  Example_Content(int count) : count(count) {}

  bool is_valid() const { return count; }
  void clear() { count = 0; }

  int count;
};
ostream& operator<<(ostream& os, const Example_Content& content);


// #################################################################

template <class HASH_TYPE, class CONTENT_TYPE>
struct HashTableEntry {
  HashTableEntry() : hash_value(), content() {}
  HashTableEntry(HASH_TYPE hash_value, CONTENT_TYPE content) :
    hash_value(hash_value), content(content) {}

  HASH_TYPE hash_value;
  CONTENT_TYPE content;
};
template<class HASH_TYPE, class CONTENT_TYPE>
ostream& operator<<(ostream& os, const HashTableEntry<HASH_TYPE, CONTENT_TYPE>& entry);

// #################################################################
// #################################################################


// dirty implementation, do not assume ANYTHING, check hash_table_impl.hxx before use
template <class HASH_TYPE, class CONTENT_TYPE>
class HashTable_Simple {
public:
  // Size is power of 2
  HashTable_Simple(int log_size);
  ~HashTable_Simple();

  //void remove(CONTENT_TYPE& info);

  // [] updates hash_values at current entry and returns ref. to content
  CONTENT_TYPE& operator[](const HASH_TYPE& hash_value);

  void update(const HASH_TYPE& hash_value, const CONTENT_TYPE &content);

  bool find(const HASH_TYPE hash_value, CONTENT_TYPE **content);

  // if find2 returns false, hash_value is updated in the table, and *content is still updated
  bool find2(const HASH_TYPE hash_value, CONTENT_TYPE **content);

  void print(ostream& os);

  // clear calls clear on every CONTENT_TYPE element
  void clear() {
    for (int i=0; i<size; i++) {
      table[i].hash_value.clear();
      table[i].content.clear();
    }
  }

//protected:

  HashTableEntry<HASH_TYPE, CONTENT_TYPE> *table;
  
  int size;
  uint max_index;

private:
  // Private to prevent copying:
  HashTable_Simple(const HashTable_Simple<HASH_TYPE, CONTENT_TYPE>&);
  HashTable_Simple& operator=(const HashTable_Simple<HASH_TYPE, CONTENT_TYPE>&);
};

// #################################################################
// #################################################################


// After the hash table get filled, all updates that do not overwrite
// will be ignored
template <class HASH_TYPE, class CONTENT_TYPE>
class HashTable {
public:
  HashTable(int log_size, int allowed_fill_in_percent = 50);
  ~HashTable();
  void set_allowed_fill(int allowed_fill_in_percent = 50) {
    allowed_fill = (size < 1000000) ?
      (size*allowed_fill_in_percent / 100) : ((size/100)*allowed_fill_in_percent);
  }

  CONTENT_TYPE operator[](const HASH_TYPE& hash_value);
  // remove and update will work on the element returned by operator[]
  void remove();
  void update(const CONTENT_TYPE& content);

  void print(ostream& os);

  bool is_full() const { return current_fill >= allowed_fill; }
  int get_current_fill() const { return current_fill; }
  int get_allowed_fill() const { return allowed_fill; }

  void clear();

protected:
  // int get_index(HASH_TYPE hash_value) { return hash_value.low & max_index; }
  inline void remove(int index);

  int size, actual_size;
  uint max_index;

  int current_fill;
  int last_index_returned;

  int allowed_fill;

  HashTableEntry<HASH_TYPE, CONTENT_TYPE> *table;

private:
  // Private to prevent copying:
  HashTable(const HashTable&);
  HashTable& operator=(const HashTable&);
};

// #################################################################
// #################################################################

class HTA_STAT {
public:
  HTA_STAT() { clear(); }
  void clear();
  void insert(int depth) { ++num_with_depth[depth]; }
  void found(int depth, uint age);
  friend ostream& operator<<(ostream& os, const HTA_STAT& stat);
private:
  int num_with_depth[16];
  int matches_found[16][8];
};
ostream& operator<<(ostream& os, const HTA_STAT& stat);

// ##########################

template <class HASH_TYPE, class CONTENT_TYPE>
class HashTable_Age {
public:
  HashTable_Age(int log_size, int allowed_fill_in_percent = 50);
  ~HashTable_Age();
  void set_allowed_fill(int allowed_fill_in_percent = 50) {
    allowed_fill = (size < 1000000) ?
      (size*allowed_fill_in_percent / 100) : ((size/100)*allowed_fill_in_percent);
  }

  CONTENT_TYPE operator[](const HASH_TYPE& hash_value);
  // remove and update will work on the element returned by operator[]
  void remove();
  void update(const CONTENT_TYPE& content);

  void clear();

  void print(ostream& os);

  void clear_stat() { stat.clear(); }
  void print_stat(ostream& os) { os << stat; }
protected:
  // int get_index(HASH_TYPE hash_value) { return hash_value.low & max_index; }
  inline void remove(int index);

  int size, actual_size;
  uint max_index;

  uint current_age;
  int total_fill; // (Only for tjecking effectiveness)
  int last_index_returned;

  uint allowed_fill;

  uint *ages;
  HashTableEntry<HASH_TYPE, CONTENT_TYPE> *table;

  bool outdated(int age) { return current_age - age >= allowed_fill; }

  HTA_STAT stat;

private:
  // Private to prevent copying:
  HashTable_Age(const HashTable_Age<HASH_TYPE, CONTENT_TYPE>&);
  HashTable_Age& operator=(const HashTable_Age<HASH_TYPE, CONTENT_TYPE>&);
};

#include "hash_table_impl.hxx"

#endif
