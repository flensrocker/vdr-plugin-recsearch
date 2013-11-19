#include "search_menu.h"

#include <vdr/menuitems.h>


recsearch::cSearchMenu::cSearchMenu(void)
 : cOsdMenu(tr("search recordings"), 12)
{
  memset(_search_term, 0, RECSEARCH_MAX_LEN);
  SetMenuCategory(mcPlugin);
  Add(new cMenuEditStrItem(tr("search term"), _search_term, RECSEARCH_MAX_LEN, NULL));
}

recsearch::cSearchMenu::~cSearchMenu(void)
{
  dsyslog("recsearch: ~cSearchMenu");
}

eOSState recsearch::cSearchMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:
        {
          compactspace(_search_term);
          if (_search_term[0] != 0) {
             isyslog("recsearch: search for %s", _search_term);
             return AddSubMenu(new cSearchResult(_search_term));
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
  SetTitle(*cString::sprintf("%s: %s", tr("search result"), SearchTerm));
  SetMenuCategory(mcRecording);
  _info = new cOsdItem(tr("searching..."), osUnknown, false);
  Add(_info);

  _parameter._host = this;
  _parameter._search_terms.Append(strdup(SearchTerm));
  cSearchProvider::StartSearch(&_parameter);
}

recsearch::cSearchResult::~cSearchResult(void)
{
  dsyslog("recsearch: ~cSearchResult");
  cSearchProvider::StopSearch();
}

eOSState recsearch::cSearchResult::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  return state;
}

void  recsearch::cSearchResult::SearchDone(void)
{
  Del(0);
  _info = NULL;
  for (cRecording *r = _parameter._result.First(); r; r = _parameter._result.Next(r)) {
      Add(new cOsdItem(r->Info()->Title()));
      }
  Display();
}
