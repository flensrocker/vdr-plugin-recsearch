#include "search_menu.h"

#include "menu_recordings.h"

#include <vdr/interface.h>
#include <vdr/menuitems.h>


// --- cSearchMenuCategory ---------------------------------------------------

namespace recsearch
{
  class cSearchMenuCategory : public cOsdItem
  {
  public:
    const char *_category;

    cSearchMenuCategory(const char *Category)
    :cOsdItem(Category)
    ,_category(Category)
    {
    };

    virtual ~cSearchMenuCategory(void)
    {
    };
  };
}


// --- cSearchMenuItem -------------------------------------------------------

namespace recsearch
{
  class cSearchMenuItem : public cOsdItem
  {
  public:
    cSearchParameter *_parameter;

    cSearchMenuItem(cSearchParameter *Parameter)
    :cOsdItem(Parameter->ToText())
    ,_parameter(Parameter)
    {
    };

    virtual ~cSearchMenuItem(void)
    {
    };
  };
}


// --- cSearchMenuCategories -------------------------------------------------

namespace recsearch
{
  class cSearchMenuCategories : public cOsdMenu
  {
  private:
    cSearches        &_searches;
    cSearchParameter &_parameter;
    cStringList       _categories;

  public:
    cSearchMenuCategories(cSearches &Searches, cSearchParameter &Parameter)
    :cOsdMenu(tr("select category"), 12)
    ,_searches(Searches)
    ,_parameter(Parameter)
    {
      _searches.GetCategories(_categories);
      Add(new cSearchMenuCategory(tr("<no category>")));
      for (int i = 0; i < _categories.Size(); i++)
          Add(new cSearchMenuCategory(_categories.At(i)));
    };

    virtual ~cSearchMenuCategories(void)
    {
    };

    virtual eOSState ProcessKey(eKeys Key)
    {
      eOSState state = cOsdMenu::ProcessKey(Key);

      if (state == osUnknown) {
         switch (Key) {
           case kOk:
            {
              const char *category = NULL;
              int i = Current() - 1;
              if ((i >= 0) && (i < _categories.Size()))
                 category = _categories.At(i);
              _parameter.SetCategory(category);
              return osBack;
            }
           default:
              return osContinue;
           }
         }

      return state;
    };
  };
}


// --- cSearchMenuLoad -------------------------------------------------------

namespace recsearch
{
  class cSearchMenuLoad : public cOsdMenu
  {
  private:
    cSearches        &_searches;
    cSearchParameter &_parameter;
    cStringList       _categories;
    const char       *_category;

  public:
    cSearchMenuLoad(cSearches &Searches, cSearchParameter &Parameter)
    :cOsdMenu(tr("load search template"), 12)
    ,_searches(Searches)
    ,_parameter(Parameter)
    ,_category(NULL)
    {
      SetCategory(NULL);
    };

    virtual ~cSearchMenuLoad(void)
    {
    };

    void SetCategory(const char *Category)
    {
      Clear();
      if (Category == NULL) {
         _searches.GetCategories(_categories);
         for (int i = 0; i < _categories.Size(); i++)
             Add(new cSearchMenuCategory(_categories.At(i)));
         for (cSearchParameter *p = _searches.First(); p; p = _searches.Next(p)) {
             if (isempty(p->Category()))
                Add(new cSearchMenuItem(p));
             }
         }
      else {
         for (cSearchParameter *p = _searches.First(); p; p = _searches.Next(p)) {
             if (strcmp(p->Category(), Category) == 0)
                Add(new cSearchMenuItem(p));
             }
         }
      Display();
      _category = Category;
    };

    virtual eOSState ProcessKey(eKeys Key)
    {
      if ((Key == kBack) && (_category != NULL)) {
         SetCategory(NULL);
         return osContinue;
         }

      eOSState state = cOsdMenu::ProcessKey(Key);

      if (state == osUnknown) {
         switch (Key) {
           case kOk:
            {
              cOsdItem *i = Get(Current());
              cSearchMenuItem *item = dynamic_cast<cSearchMenuItem*>(i);
              if (item != NULL) {
                 _parameter = *(item->_parameter);
                 return osBack;
                 }
              cSearchMenuCategory *category = dynamic_cast<cSearchMenuCategory*>(i);
              if (category != NULL) {
                 SetCategory(category->_category);
                 return osContinue;
                 }
              break;
            }
           default:
              return osContinue;
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

  if (cSearches::Last.LoadSearches() && (cSearches::Last.Count() > 0))
     _data = *cSearches::Last.Get(0);

  Add(new cMenuEditStrItem(tr("search term"), _data._term, RECSEARCH_MAX_LEN, NULL));
  Add(new cMenuEditStraItem(tr("status"), &_data._status, 3, cSearchParameter::_status_text));
  Add(new cMenuEditIntItem(tr("younger than days"), &_data._younger_than_days, 0, INT_MAX, tr("whatever")));
  Add(new cMenuEditIntItem(tr("hot key"), &_data._hot_key, 0, 9, tr("no hot key")));
  Add(_category = new cMenuEditStrItem(tr("category"), _data._category, RECSEARCH_MAX_LEN, NULL));

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

  cOsdItem *current = Get(Current());
  if (!HasSubMenu()) {
     if (current == _category)
        SetHelp(tr("Button$Save"), tr("Button$Find"), tr("Button$Delete"), tr("Button$Select"));
     else
        SetHelp(tr("Button$Save"), tr("Button$Find"), tr("Button$Delete"), tr("Button$Load"));
     }

  if (state == osUnknown) {
     switch (Key) {
       case k1:
       case k2:
       case k3:
       case k4:
       case k5:
       case k6:
       case k7:
       case k8:
       case k9:
        {
         int hotkey = Key - k0;
         cSearches::Searches.LoadSearches();
         cSearchParameter *p = cSearches::Searches.GetHotKey(hotkey);
         if (p != NULL)
            return AddSubMenu(new cMenuRecordings(NULL, -1, false, new cSearchParameter(*p)));
         break;
        }
       case kRed:
        {
         cSearches::Searches.LoadSearches();
         if (cSearches::Searches.Contains(_data) == NULL) {
            cSearches::Searches.Add(new cSearchParameter(_data));
            cSearches::Searches.Save();
            }
         Skins.Message(mtInfo, tr("search template has been saved"));
         break;
        }
       case kGreen:
       case kOk:
        {
          if (_data.IsValid()) {
             cSearches::Last.cList<cSearchParameter>::Clear();
             cSearches::Last.Add(new cSearchParameter(_data));
             cSearches::Last.Save();
             return AddSubMenu(new cMenuRecordings(NULL, -1, false, new cSearchParameter(_data)));
             }
          Skins.Message(mtError, tr("enter something I can search for"));
          return osContinue;
        }
       case kYellow:
        {
         if (Interface->Confirm(tr("delete shown search template?"))) {
            cSearches::Searches.LoadSearches();
            cSearchParameter *p = cSearches::Searches.Contains(_data);
            if (p != NULL) {
               cSearches::Searches.Del(p);
               cSearches::Searches.Save();
               Skins.Message(mtInfo, tr("search template was deleted from file"));
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
            Skins.Message(mtInfo, tr("you have to save a search template first"));
            return osContinue;
            }
         _needs_refresh = true;
         if (current == _category)
            return AddSubMenu(new cSearchMenuCategories(cSearches::Searches, _data));
         return AddSubMenu(new cSearchMenuLoad(cSearches::Searches, _data));
        }
       default: break;
       }
     }
  return state;
}
