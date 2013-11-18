#ifndef search_menu_h
#define search_menu_h

#include <vdr/osdbase.h>


namespace recsearch
{
  #define RECSEARCH_MAX_LEN 100

  class cSearchMenu : public cOsdMenu
  {
  private:
    char  search_term[RECSEARCH_MAX_LEN];

  public:
    cSearchMenu(void);
    virtual ~cSearchMenu(void);

    virtual eOSState ProcessKey(eKeys Key);
  };

  class cSearchResult : public cOsdMenu
  {
  private:

  public:
    cSearchResult(const char *SearchTerm);
    virtual ~cSearchResult(void);

    virtual eOSState ProcessKey(eKeys Key);
  };
}

#endif

