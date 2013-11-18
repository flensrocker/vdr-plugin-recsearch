#include "search_menu.h"

#include <vdr/menuitems.h>


recsearch::cSearchMenu::cSearchMenu(void)
 : cOsdMenu(tr("search recordings"), 12)
{
  memset(search_term, 0, RECSEARCH_MAX_LEN);
  SetMenuCategory(mcPlugin);
  Add(new cMenuEditStrItem(tr("search term"), search_term, RECSEARCH_MAX_LEN, NULL));
}

recsearch::cSearchMenu::~cSearchMenu(void)
{
}

eOSState recsearch::cSearchMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:
        {
          compactspace(search_term);
          if (search_term[0] != 0) {
             isyslog("recsearch: search for %s", search_term);
             return AddSubMenu(new cSearchResult(search_term));
             }
          return osBack;
        }
       default: break;
       }
     }
  return state;
}


recsearch::cSearchResult::cSearchResult(const char *SearchTerm)
 : cOsdMenu(tr("search result"))
{
  cString title= cString::sprintf("%s: %s", tr("search result"), SearchTerm);
  SetTitle(*title);
  SetMenuCategory(mcRecording);
}

recsearch::cSearchResult::~cSearchResult(void)
{
}

eOSState recsearch::cSearchResult::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  return state;
}
