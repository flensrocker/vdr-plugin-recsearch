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
    bool  _needs_refresh;
    cOsdItem *_category;

  public:
    cSearchMenu(cSearchParameter *Draft = NULL);
    virtual ~cSearchMenu(void);

    virtual void Display(void);
    virtual eOSState ProcessKey(eKeys Key);
  };

  class cMainMenu : public cOsdMenu
  {
  private:
   char  *_base_cat;
   cList<cNestedItem>  *_cats;

  public:
    cMainMenu(const char *BaseCat = NULL, cList<cNestedItem> *Cats = NULL);
    virtual ~cMainMenu(void);

    virtual eOSState ProcessKey(eKeys Key);
  };
}

#endif
