#include <math.h>
#include <assert.h>
#include <iostream>

#include "help_functions.hxx"

template<class HASH_TYPE, class CONTENT_TYPE>
ostream& operator<<(ostream& os, const HashTableEntry<HASH_TYPE, CONTENT_TYPE>& entry) {
  os << "HashTableEntry(" << entry.hash_value << ", " << entry.content << ')';
  return os;
}

// #################################################################
// #################################################################

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::print(ostream& os) {
  os << "HashTable_Simple(" << this << "), size = " << size << "\n"
     << "\tsizeof(HASH_TYPE) = " << sizeof(HASH_TYPE) << "\n"
     << "\tsizeof(CONTENT_TYPE) = " << sizeof(CONTENT_TYPE) << "\n";
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::HashTable_Simple(int log_size) :
  size(1<<log_size), max_index((1<<log_size)-1)
{
  cerr << "Creating HashTable_Simple(log_size = " << log_size << "), memory consumption = "
       << size*sizeof(HashTableEntry<HASH_TYPE, CONTENT_TYPE>) << " bytes.\n";
  table = new HashTableEntry<HASH_TYPE, CONTENT_TYPE>[size];
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::~HashTable_Simple() {
  delete[] table;
}

template<class HASH_TYPE, class CONTENT_TYPE>
CONTENT_TYPE& HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::operator[](const HASH_TYPE& hash_value) {
  return table[hash_value.get_index(max_index)].content;
}

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::update(const HASH_TYPE& hash_value,
						       const CONTENT_TYPE &content) {
  int index = hash_value.get_index(max_index);
  table[index].hash_value = hash_value;
  table[index].content = content;
}

/*
template<class HASH_TYPE, class CONTENT_TYPE>
inline void HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::remove(int index) {
  table[index].content.clear();
}
template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::remove(CONTENT_TYPE& content) {
  remove(((int)&content - (int)table)/sizeof(HashTableEntry<HASH_TYPE, CONTENT_TYPE>));
}
*/

template<class HASH_TYPE, class CONTENT_TYPE>
bool HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::find(const HASH_TYPE hash_value, CONTENT_TYPE **content) {
  int index = hash_value.get_index(max_index);
  if (hash_value == table[index].hash_value) {
    *content = &(table[index].content);
    return true;
  }
  return false;
}

template<class HASH_TYPE, class CONTENT_TYPE>
bool HashTable_Simple<HASH_TYPE, CONTENT_TYPE>::find2(const HASH_TYPE hash_value, CONTENT_TYPE **content) {
  int index = hash_value.get_index(max_index);
  *content = &(table[index].content);
  if (hash_value == table[index].hash_value) return true;

  table[index].hash_value = hash_value;
  return false;
}

// #################################################################
// #################################################################

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable<HASH_TYPE, CONTENT_TYPE>::print(ostream& os) {
  os << "HashTable:\n"
     << "  - size = " << size << " (actual size = " << actual_size << ")\n"
     << "  - last index returned = " << last_index_returned << '\n';
  if (last_index_returned != -1)
    os << "      - contains " << table[last_index_returned] << '\n';
  os << "  - (current,allowed) fill = (" << current_fill
     << ',' << allowed_fill << ")\n";
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable<HASH_TYPE, CONTENT_TYPE>::HashTable(int log_size, int allowed_fill_in_percent) :
  size(1<<log_size), actual_size((1<<log_size) + 2*(1<<(log_size>>1)) + 4),
  max_index((1<<log_size)-1), current_fill(0), last_index_returned(-1)
{
  set_allowed_fill(allowed_fill_in_percent);
  table = new HashTableEntry<HASH_TYPE, CONTENT_TYPE>[actual_size];
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable<HASH_TYPE, CONTENT_TYPE>::~HashTable() {
  delete[] table;
}

template<class HASH_TYPE, class CONTENT_TYPE>
CONTENT_TYPE HashTable<HASH_TYPE, CONTENT_TYPE>::operator[](const HASH_TYPE& hash_value) {
  int index = hash_value.get_index(max_index);

  while (table[index].content.is_valid()) {
    if (table[index].hash_value == hash_value)
      return table[last_index_returned = index].content;
    ++index;
    assert(index < actual_size);  
  }

  // Remember the index. Also update the hash value of the
  // entry in case that it is being updated.
  table[index].hash_value = hash_value;
  return table[last_index_returned = index].content;
}

template<class HASH_TYPE, class CONTENT_TYPE>
inline void HashTable<HASH_TYPE, CONTENT_TYPE>::remove(int index) {
  int hole = index;

  while (table[++index].content.is_valid()) {
    assert(index < actual_size);  
    if (table[index].hash_value.get_index(max_index) <= hole) {
      // Relocate entry
      table[hole] = table[index];
      // Now index has become the hole
      hole = index;
    }
  }

  // Clear hole
  table[hole].content.clear();
}

template<class HASH_TYPE, class CONTENT_TYPE>
inline void HashTable<HASH_TYPE, CONTENT_TYPE>::update(const CONTENT_TYPE& content) {
  if (table[last_index_returned].content.is_valid()) {
    table[last_index_returned].content = content;
  } else if (!is_full()) {
    ++current_fill;
    table[last_index_returned].content = content;
  } else {
    cerr << "Hash table is filled! (allowed_fill = "
	 << allowed_fill << ") update ignored\n";
  }
}


template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable<HASH_TYPE, CONTENT_TYPE>::remove() {
  assert(table[last_index_returned].content.is_valid());
  --current_fill;
  remove(last_index_returned);
}

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable<HASH_TYPE, CONTENT_TYPE>::clear() {
  current_fill = 0;
  last_index_returned = -1;
  for (int i=0; i<actual_size; i++) table[i].content.clear();
}

// #################################################################
// #################################################################

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Age<HASH_TYPE, CONTENT_TYPE>::print(ostream& os) {
  os << "HashTable:\n"
     << "  - size = " << size << " (actual size = " << actual_size << ")\n"
     << "  - last index returned = " << last_index_returned << '\n';
  if (last_index_returned != -1)
    os << "      - contains " << table[last_index_returned]
       << " with age " << ages[last_index_returned] << '\n';
  os << "  - (total,allowed) fill = (" << total_fill << ',' << allowed_fill << ")\n"
     << "      - (total fill also includes outdated entries)\n"
     << "  - current age = " << current_age << '\n';
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable_Age<HASH_TYPE, CONTENT_TYPE>::HashTable_Age(int log_size, int allowed_fill_in_percent) :
  size(1<<log_size), actual_size((1<<log_size) + 2*(1<<(log_size>>1)) + 4),
  max_index((1<<log_size)-1), current_age(0),
  total_fill(0), last_index_returned(-1), stat()
{
  set_allowed_fill(allowed_fill_in_percent);
  table = new HashTableEntry<HASH_TYPE, CONTENT_TYPE>[actual_size];
  ages = new uint[actual_size];
}

template<class HASH_TYPE, class CONTENT_TYPE>
HashTable_Age<HASH_TYPE, CONTENT_TYPE>::~HashTable_Age() {
  delete[] table;
  delete[] ages;
}

template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Age<HASH_TYPE, CONTENT_TYPE>::clear() {
  current_age = 0;
  total_fill = 0;
  last_index_returned = -1;
  for (int i=0; i<actual_size; i++) table[i].content.clear();
}

template<class HASH_TYPE, class CONTENT_TYPE>
inline void HashTable_Age<HASH_TYPE, CONTENT_TYPE>::remove(int index) {
  total_fill -= table[++index].content.is_valid();

  int hole = index;

  while (table[++index].content.is_valid()) {
    assert(index < actual_size);
    if (table[index].hash_value.get_index(max_index) <= hole) {
      // Relocate entry
      table[hole] = table[index];
      // Now index has become the hole
      hole = index;
    }
  }

  // Clear hole
  table[hole].content.clear();
}

template<class HASH_TYPE, class CONTENT_TYPE>
inline void HashTable_Age<HASH_TYPE, CONTENT_TYPE>::update(const CONTENT_TYPE& content) {
  stat.insert(content.depth);
  total_fill += ! table[last_index_returned].content.is_valid();
  table[last_index_returned].content = content;
  ages[last_index_returned] = ++current_age;
}

template<class HASH_TYPE, class CONTENT_TYPE>
CONTENT_TYPE HashTable_Age<HASH_TYPE, CONTENT_TYPE>::operator[](const HASH_TYPE& hash_value) {
  int index = hash_value.get_index(max_index);

  while (table[index].content.is_valid()) {
    if (table[index].hash_value == hash_value) {
      stat.found(table[index].content.depth, current_age - ages[index]);
      return table[last_index_returned = index].content;
    }
    if (outdated(ages[index]))
      remove(index);
    ++index;
    assert(index < actual_size);
  }

  // Remember the index. Also update the hash value of the
  // entry in case that it is being updated.
  table[index].hash_value = hash_value;
  return table[last_index_returned = index].content;
}


template<class HASH_TYPE, class CONTENT_TYPE>
void HashTable_Age<HASH_TYPE, CONTENT_TYPE>::remove() {
  assert(table[last_index_returned].content.is_valid());

  remove(last_index_returned);
}

