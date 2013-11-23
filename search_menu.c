#include "search_menu.h"

#include "menu_recordings.h"

#include <vdr/menuitems.h>


// --- cSearchMenu -----------------------------------------------------------

recsearch::cSearchMenu::cSearchMenu(void)
:cOsdMenu(tr("search recordings"), 12)
{
  SetMenuCategory(mcPlugin);
  Add(new cMenuEditStrItem(tr("search term"), _data._term, RECSEARCH_TERM_MAX_LEN, NULL));
  Add(new cMenuEditStraItem(tr("status"), &_data._status, 3, cSearchParameter::_status_text));

  SetHelp(NULL /*tr("Button$Save")*/, tr("Button$Find"), NULL /*tr("Button$Delete")*/, NULL /*tr("Button$Load")*/);
}

recsearch::cSearchMenu::~cSearchMenu(void)
{
}

eOSState recsearch::cSearchMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kGreen:
       case kOk:
        {
          if (_data.IsValid()) {
             _parameter = _data;
             return AddSubMenu(new cMenuRecordings(NULL, -1, false, &_parameter));
             }
          return osBack;
        }
       default: break;
       }
     }
  return state;
}
