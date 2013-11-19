#ifndef recsearch_search_provider_h
#define recsearch_search_provider_h

#include <vdr/recording.h>


namespace recsearch
{
  class cSearchHost
  {
  public:
    virtual void  SearchDone(void) = 0;
  };

  class cSearchParameter
  {
  public:
    cMutex         _mutex;
    cSearchHost   *_host;
    cStringList    _search_terms;
    cRecordings    _result;
    int            _count;

    cSearchParameter(void):_host(NULL),_count(0) {};
    virtual ~cSearchParameter(void) {};

    void  AddResult(cRecording *Recording);
    void  Done(void);
  };

  class cSearchProvider : public cListObject, public cThread
  {
  private:
    static cList<cSearchProvider>  _providers;
    static cSearchParameter*       _parameter;

  public:
    static void  StartSearch(cSearchParameter *Parameter);
    static void  StopSearch(void);

    cSearchProvider(void);
    virtual ~cSearchProvider(void);

    virtual void Action(void);

    virtual void OnSearch(cSearchParameter *Parameter) = 0;
  };
}

#endif
