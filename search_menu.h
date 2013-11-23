#ifndef recsearch_search_menu_h
#define recsearch_search_menu_h

#include <vdr/osdbase.h>

#include "search_parameter.h"


namespace recsearch
{
  #define RECSEARCH_MAX_LEN 100

  class cSearchMenu : public cOsdMenu
  {
  private:
    cSearchParameter _data;
    cSearchParameter _parameter;

  public:
    cSearchMenu(void);
    virtual ~cSearchMenu(void);

    virtual eOSState ProcessKey(eKeys Key);
  };
}

#endif
