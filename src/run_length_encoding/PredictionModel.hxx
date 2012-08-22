#ifndef _PREDICTION_MODEL_HXX_
#define _PREDICTION_MODEL_HXX_

#include <map>
#include <string>
#include <vector>
#include <cmath>

#include "bit_stream.hxx"

using namespace std;

template <class TYPE, class ElementStreamer = DefaultElementStreamer<TYPE> >
class PredictionModel {
public:
  virtual void nextElement(TYPE element) {}

  virtual vector<pair<TYPE, float> > & getDistribution() = 0;

  virtual float getEntropy();
protected:
  ElementStreamer element_streamer;
};

template <class TYPE, class ElementStreamer = DefaultElementStreamer<TYPE> >
class ZerothOrderModel : public PredictionModel<TYPE, ElementStreamer> {
public:
  ZerothOrderModel(string filename);

  vector<pair<TYPE, float> > & getDistribution();
  vector<pair<TYPE, int> > getElemCounts();

  float getEntropy();

  int getNumElements() { return count; }
protected:
  // hash_map<TYPE, int> gah;

  map<TYPE, int, ElementStreamer> elem_count;
  int count;

  bool distribution_initialized;
  vector<pair<TYPE, float> > distribution;
};

#include "PredictionModel_templ.hxx"

#endif
