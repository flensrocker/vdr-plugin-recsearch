#include "search_menu.h"

#include "menu_recordings.h"

#include <vdr/menuitems.h>


// --- cSearchMenu -----------------------------------------------------------

recsearch::cSearchMenu::cSearchMenu(void)
:cOsdMenu(tr("search recordings"), 12)
{
  memset(_search_term, 0, RECSEARCH_MAX_LEN);
  _status = 0;
  _status_item[0] = tr("all");
  _status_item[1] = tr("only new");
  _status_item[2] = tr("only edited");

  SetMenuCategory(mcPlugin);
  Add(new cMenuEditStrItem(tr("search term"), _search_term, RECSEARCH_MAX_LEN, NULL));
  Add(new cMenuEditStraItem(tr("status"),    &_status, 3, _status_item));
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
          compactspace(_search_term);
          if ((_search_term[0] != 0) || (_status != 0)) {
             _parameter._search_term = _search_term;
             _parameter._search_status = _status;
             return AddSubMenu(new cMenuRecordings(NULL, -1, false, &_parameter));
             }
          return osBack;
        }
       default: break;
       }
     }
  return state;
}
