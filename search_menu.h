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
    static cSearchParameter *_last;
    static cSearchParameter *GetFilter(const cSearchParameter &Parameter);

    cSearchParameter _data;
    bool  _needs_refresh;
    cOsdItem *_category;

  public:
    cSearchMenu(void);
    virtual ~cSearchMenu(void);

    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
  };
}

#endif
