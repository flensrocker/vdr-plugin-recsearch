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
    char  _search_term[RECSEARCH_MAX_LEN];

  public:
    cSearchMenu(void);
    virtual ~cSearchMenu(void);

    virtual eOSState ProcessKey(eKeys Key);
  };

  class cSearchResult : public cOsdMenu, public cSearchHost
  {
  private:
    cOsdItem   *_info;
    cSearchParameter _parameter;

  public:
    cSearchResult(const char *SearchTerm);
    virtual ~cSearchResult(void);

    virtual eOSState ProcessKey(eKeys Key);
    virtual void  SearchDone(void);
  };
}

#endif
