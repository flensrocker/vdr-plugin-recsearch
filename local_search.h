#ifndef recsearch_local_search_h
#define recsearch_local_search_h

#include "search_provider.h"


namespace recsearch
{
  class cLocalSearch : public cSearchProvider
  {
  public:
    virtual void OnSearch(cSearchParameter *Parameter);
  };
}

#endif
