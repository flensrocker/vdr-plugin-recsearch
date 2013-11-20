#ifndef recsearch_search_menu_h
#define recsearch_search_menu_h

#include <vdr/osdbase.h>

#include "search_provider.h"


namespace recsearch
{
  #define RECSEARCH_MAX_LEN 100

  class cSearchMenu : public cOsdMenu
  {
  private:
    char        _search_term[RECSEARCH_MAX_LEN];
    int         _status; // 0 = all, 1 = only new, 2 = only edited
    const char *_status_item[3];

    cSearchParameter _parameter;

  public:
    cSearchMenu(void);
    virtual ~cSearchMenu(void);

    virtual eOSState ProcessKey(eKeys Key);
  };

  class cSearchResult : public cOsdMenu, public cSearchHost
  {
  private:
    cSearchParameter *_parameter;

    cRecording *GetSelectedRecording(void);
    eOSState Play(void);
    eOSState Rewind(void);
    eOSState Info(void);

  public:
    cSearchResult(cSearchParameter *Parameter);
    virtual ~cSearchResult(void);

    virtual eOSState ProcessKey(eKeys Key);
    virtual void  SearchDone(void);
  };
}

#endif
