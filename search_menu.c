#include "search_menu.h"

#include "menu_recordings.h"

#include <vdr/interface.h>
#include <vdr/menuitems.h>


// --- cSearchMenuLoad -------------------------------------------------------

namespace recsearch
{
  class cSearchMenuLoad : public cOsdMenu
  {
  private:
    cSearches        &_searches;
    cSearchParameter &_parameter;

  public:
    cSearchMenuLoad(cSearches &Searches, cSearchParameter &Parameter)
    :cOsdMenu(tr("load search"), 12)
    ,_searches(Searches)
    ,_parameter(Parameter)
    {
      for (cSearchParameter *p = _searches.First(); p; p = _searches.Next(p))
          Add(new cOsdItem(p->ToText()));
    };

    virtual ~cSearchMenuLoad(void)
    {
    };

    eOSState ProcessKey(eKeys Key)
    {
      eOSState state = cOsdMenu::ProcessKey(Key);

      if (state == osUnknown) {
         switch (Key) {
           case kOk:
            {
              int i = Current();
              if ((i >= 0) && (i < _searches.Count()))
                 _parameter = *_searches.Get(i);
              return osBack;
            }
           default: break;
           }
         }

      return state;
    };
  };
}


// --- cSearchMenu -----------------------------------------------------------

recsearch::cSearchMenu::cSearchMenu(void)
:cOsdMenu(tr("search recordings"), 12)
,_needs_refresh(false)
{
  SetMenuCategory(mcPlugin);
  Add(new cMenuEditStrItem(tr("search term"), _data._term, RECSEARCH_TERM_MAX_LEN, NULL));
  Add(new cMenuEditStraItem(tr("status"), &_data._status, 3, cSearchParameter::_status_text));

  SetHelp(tr("Button$Save"), tr("Button$Find"), tr("Button$Delete"), tr("Button$Load"));
}

recsearch::cSearchMenu::~cSearchMenu(void)
{
}

void recsearch::cSearchMenu::Display(void)
{
  if (_needs_refresh) {
     // refresh all items
     _needs_refresh = false;
     for (cOsdItem *o = First(); o; o = Next(o))
         o->Set();
     }
  cOsdMenu::Display();
}

eOSState recsearch::cSearchMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:
        {
         cSearches::Searches.LoadSearches();
         cSearchParameter *p = cSearches::Searches.Contains(_data);
         if (p == NULL) {
            p = new cSearchParameter(_data);
            cSearches::Searches.Add(p);
            cSearches::Searches.Save();
            }
         Skins.Message(mtInfo, tr("search has been saved"));
         break;
        }
       case kGreen:
       case kOk:
        {
          if (_data.IsValid())
             return AddSubMenu(new cMenuRecordings(NULL, -1, false, new cSearchParameter(_data)));
          Skins.Message(mtError, tr("enter something I can search for"));
          return osContinue;
        }
       case kYellow:
        {
         if (Interface->Confirm(tr("delete shown search?"))) {
            cSearches::Searches.LoadSearches();
            cSearchParameter *p = cSearches::Searches.Contains(_data);
            if (p != NULL) {
               cSearches::Searches.Del(p);
               cSearches::Searches.Save();
               Skins.Message(mtInfo, tr("search was deleted from file"));
               }
            else {
               _data.Clear();
               _needs_refresh = true;
               Display();
               }
            }
         break;
        }
       case kBlue:
        {
         cSearches::Searches.LoadSearches();
         if (cSearches::Searches.Count() == 0) {
            Skins.Message(mtInfo, tr("you have to save a search first"));
            return osContinue;
            }
         _needs_refresh = true;
         return AddSubMenu(new cSearchMenuLoad(cSearches::Searches, _data));
        }
       default: break;
       }
     }
  return state;
}
