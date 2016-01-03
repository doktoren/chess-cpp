template <class TYPE, class ElementStreamer>
ZerothOrderModel<TYPE, ElementStreamer>::ZerothOrderModel(__attribute__((unused)) string filename) :
  count(0), distribution_initialized(false)
{
  assert(0); // I don't want to fix this code now.

  /*
  ibstream<file_ibstream> in(filename);
  
  TYPE element;
  while (element_streamer.readElement(&in, element)) {
    elem_count[element] = elem_count[element] + 1;
    ++count;
  }

  in.close();
  */
}

template <class TYPE, class ElementStreamer>
vector<pair<TYPE, float> > & ZerothOrderModel<TYPE, ElementStreamer>::getDistribution() {
  if (!distribution_initialized) {
    distribution.resize(elem_count.size(), pair<TYPE, float>());
    float normalizing_factor = 1.0/count;
    typedef typename map<TYPE, int, ElementStreamer>::const_iterator CI;
    int i=0;
    for (CI ci=elem_count.begin(); ci!=elem_count.end(); ci++)
      distribution[i++] = pair<TYPE, float>((*ci).first, (*ci).second * normalizing_factor);
  }
  return distribution;
}

template <class TYPE, class ElementStreamer>
float ZerothOrderModel<TYPE, ElementStreamer>:: getEntropy() {
  getDistribution();
  float entropy = 0;
  typedef typename vector<pair<TYPE, float> >::const_iterator CI;
  for (CI ci = distribution.begin(); ci!=distribution.end(); ci++)
    entropy += (*ci).second * (-log((*ci).second));
  return entropy / log(2.0);
}

template <class TYPE, class ElementStreamer>
vector<pair<TYPE, int> > ZerothOrderModel<TYPE, ElementStreamer>::getElemCounts() {
  vector<pair<TYPE, int> > result(elem_count.size());
  typedef typename map<TYPE, int, ElementStreamer>::const_iterator CI;
  int i=0;
  for (CI ci=elem_count.begin(); ci!=elem_count.end(); ci++)
    result[i++] = pair<TYPE, int>((*ci).first, (*ci).second);
  return result;
}
