#include "search_menu.h"

#include <vdr/menu.h>


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
  SetTitle(tr("searching..."));
  SetMenuCategory(mcRecording);
  Display();

  _parameter._host = this;
  _parameter._search_term = SearchTerm;
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
  if (state == osUnknown) {
     if (Key == kOk) {
        int i = Current();
        if ((i >= 0) && (i < _parameter._result.Count())) {
           cReplayControl::SetRecording(_parameter._result.Get(i)->FileName());
           return osReplay;
           }
        return osBack;
        }
     }
  return state;
}

void  recsearch::cSearchResult::SearchDone(void)
{
  dsyslog("recsearch: cSearchResult::SearchDone");
  SetTitle(*cString::sprintf("%s: %s", tr("search result"), *_parameter._search_term));
  if (_parameter._result.Count() == 0) {
     Add(new cOsdItem(tr("nothing found"), osUnknown, false));
     }
  else {
     for (cRecording *r = _parameter._result.First(); r; r = _parameter._result.Next(r)) {
         if (r->Info()->ShortText() == NULL)
            Add(new cOsdItem(r->Info()->Title()));
         else
            Add(new cOsdItem(*cString::sprintf("%s~%s", r->Info()->Title(), r->Info()->ShortText())));
         }
     }
  Display();
}
