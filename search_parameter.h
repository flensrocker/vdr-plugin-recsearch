#ifndef recsearch_search_parameter_h
#define recsearch_search_parameter_h

#include "menu_recordings.h"


namespace recsearch
{
  class cSearchHost
  {
  public:
    virtual void  SearchDone(void) = 0;
  };

  class cSearchParameter : public cRecordingFilter
  {
  public:
    cString        _search_term;
    int            _search_status;

    cSearchParameter(void) {};
    virtual ~cSearchParameter(void) {};

    virtual bool Filter(const cRecording *Recording);
  };
}

#endif
