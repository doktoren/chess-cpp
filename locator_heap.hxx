#ifndef _LOCATOR_HEAP_
#define _LOCATOR_HEAP_

// Fixed size - no insert, remove operations, only update
// sorts decreasing


/*
struct FromTo {
  FromTo() {}
  FromTo(Move m) : index((m.from << 6) | m.to) {}
  ushort index;
};

class FromToHeap {
public:
  // size = 4096
  FromToHeap(int size, vector<FromTo> from_to) :
    size(size+1)
  {
    heap = new pair<short, FromTo>[size];
    for (int i=1; i<size; i++) {
      heap[i] = pair<short, FromTo>(0, from_to[i-1]);
      locators[from_to[i-1].from_to] = i;
    }
  }

  ~FromToHeap() {
    delete[] heap;
  }


  void set_zero() {
    for (int i=1; i<size; i++)
      heap[i].first = 0;
  }

  void update(FromTo from_to, short add_value) {
    heap[locators[from_to.index]].first += add_value;
    bubble_up(locators[from_to.index]);
  }
    

private:
  void bubble_up(int i) {
    while (i) {
      if (heap[i<<1].first >= heap[i].first)
	return;
      swap(heap[i<<1], heap[i]);
      swap(locators[heap[i<<1].second.index], locators[heap[i].second.index]);
      i <<= 1;
    }
  }


  void bubble_down(int i) {
    while (i < size) {
      int i2 = i+i;
      if (i2+1 < size) {
	if (heap[i2] > heap[i2+1]) {
	  if (heap[i2] > heap[i]) {
	    swap(heap[i2], heap[i]);
	    swap(locators[heap[i2].second.from_to], locators[heap[i].second.from_to]);
	    i = i2;
	  }
	} else {
	  if (heap[i2+1] > heap[i]) {
	    swap(heap[i2+1], heap[i]);
	    swap(locators[heap[i2+1].second.from_to], locators[heap[i].second.from_to]);
	    i = i2+1;
	  }
	}
      } else {
	if (i2 < size  &&  heap[i2] > heap[i]) {
	  swap(heap[i2], heap[i]);
	  swap(locators[heap[i2].second.from_to], locators[heap[i].second.from_to]);
	  i = i2;
	}
      }
    }
  }

  ushort locators[4096];
  pair<short, FromTo> *heap;
  int size;
};
*/

#endif
