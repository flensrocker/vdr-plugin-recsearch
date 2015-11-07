#include "search_menu.h"

#include <vdr/interface.h>
#include <vdr/menu.h>

#if APIVERSNUM < 20103
#include "menu_recordings.h"
#endif


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


// --- cMainMenuCategory -----------------------------------------------------

namespace recsearch
{
  class cMainMenuCategory : public cOsdItem
  {
  public:
    cNestedItem *_sub_cats;

    cMainMenuCategory(cNestedItem *SubCats)
    :cOsdItem(*cString::sprintf("%s >", SubCats->Text()))
    ,_sub_cats(SubCats)
    {
    };

    virtual ~cMainMenuCategory(void)
    {
    };
  };
}


// --- cMainMenuItem ---------------------------------------------------------

namespace recsearch
{
  class cMainMenuItem : public cOsdItem
  {
  public:
    cNestedItem *_item;
    cSearchParameter *_parameter;

    cMainMenuItem(cNestedItem *Item, cSearchParameter *Parameter)
    :cOsdItem(Parameter->ToText())
    ,_item(Item)
    ,_parameter(Parameter)
    {
    };

    virtual ~cMainMenuItem(void)
    {
      delete _parameter;
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
    :cOsdMenu(tr("select category"))
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
    int               _current_category;

  public:
    cSearchMenuLoad(cSearches &Searches, cSearchParameter &Parameter)
    :cOsdMenu(tr("load search template"))
    ,_searches(Searches)
    ,_parameter(Parameter)
    ,_category(NULL)
    ,_current_category(-1)
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
         if ((_current_category > 0) && (_current_category < Count())) {
            DisplayCurrent(false);
            SetCurrent(Get(_current_category));
            DisplayCurrent(true);
            }
         _current_category = -1;
         return osContinue;
         }

      eOSState state = cOsdMenu::ProcessKey(Key);

      if (state == osUnknown) {
         switch (Key) {
           case kOk:
            {
              int cur = Current();
              cOsdItem *i = Get(cur);
              cSearchMenuItem *item = dynamic_cast<cSearchMenuItem*>(i);
              if (item != NULL) {
                 _parameter = *(item->_parameter);
                 return osBack;
                 }
              cSearchMenuCategory *category = dynamic_cast<cSearchMenuCategory*>(i);
              if (category != NULL) {
                 _current_category = cur;
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


// --- cSearchResult ---------------------------------------------------------

namespace recsearch
{
#if APIVERSNUM < 20103
#define CMENURECORDINGS recsearch::cMenuRecordings
#else
#define CMENURECORDINGS cMenuRecordings
#endif
  class cSearchResult : public CMENURECORDINGS
  {
  private:
    const cRecordingFilter *_filter;

  public:
    cSearchResult(const cSearchParameter *Filter)
    :CMENURECORDINGS(NULL, -1, false, Filter)
    ,_filter(Filter)
    {
      cSearches::Last.LoadSearches(); // to set filename in cConfig
      cSearches::Last.cList<cSearchParameter>::Clear();
      cSearches::Last.Add(new cSearchParameter(*Filter));
      cSearches::Last.Save();
    };

    virtual ~cSearchResult(void)
    {
      delete _filter;
    };
  };
#undef CMENURECORDINGS
}


// --- cSearchMenu -----------------------------------------------------------

recsearch::cSearchMenu::cSearchMenu(cSearchParameter *Draft)
:cOsdMenu(tr("edit search templates"), 15)
,_needs_refresh(false)
{
  SetMenuCategory(mcPlugin);

  if (Draft != NULL) {
     _data = *Draft;
     delete Draft;
     }
  else if (cSearches::Last.LoadSearches() && (cSearches::Last.Count() > 0))
     _data = *cSearches::Last.Get(0);

  Add(new cMenuEditStrItem(tr("name"), _data._name, RECSEARCH_MAX_LEN, NULL));
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
         const cSearchParameter *p = cSearches::Searches.GetHotKey(hotkey);
         if (p != NULL)
            return AddSubMenu(new cSearchResult(new cSearchParameter(*p)));
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
          if (_data.IsValid())
             return AddSubMenu(new cSearchResult(new cSearchParameter(_data)));
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


// --- cMainMenu -------------------------------------------------------------

recsearch::cMainMenu::cMainMenu(const char *BaseCat, cList<cNestedItem> *Cats)
:cOsdMenu(BaseCat ? BaseCat : tr("search recordings"))
{
  SetMenuCategory(mcPlugin);

  if (BaseCat == NULL) {
     _base_cat = NULL;
     _cats = new cList<cNestedItem>();
     cSearches::Searches.LoadSearches();
     cSearches::Searches.GetCatMenus(_cats);
     }
  else {
     _base_cat = strdup(BaseCat);
     _cats = Cats;
     }

  bool setHelp = true;
  for (cNestedItem *i = _cats->First(); i; i = _cats->Next(i)) {
      if ((i->SubItems() != NULL) && (i->SubItems()->Count() > 0)) {
         Add(new cMainMenuCategory(i));
         if (setHelp) {
            setHelp = false;
            SetHelp(NULL, tr("Button$New"), NULL, NULL);
            }
         }
      else {
         cSearchParameter *p = new cSearchParameter();
         if (!p->Parse(i->Text()))
            delete p;
         else {
            Add(new cMainMenuItem(i, p));
            if (setHelp) {
               setHelp = false;
               SetHelp(tr("Button$Edit"), tr("Button$New"), tr("Button$Delete"));
               }
            }
         }
      }
  if (setHelp)
     SetHelp(NULL, tr("Button$New"), NULL, NULL);
}

recsearch::cMainMenu::~cMainMenu(void)
{
  if (_base_cat == NULL)
     delete _cats;
  free(_base_cat);
}

eOSState recsearch::cMainMenu::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  int cur = Current();
  cOsdItem *current = Get(cur);
  cMainMenuItem *item = dynamic_cast<cMainMenuItem*>(current);
  cMainMenuCategory *category = dynamic_cast<cMainMenuCategory*>(current);
  if (!HasSubMenu()) {
     if (item != NULL)
        SetHelp(tr("Button$Edit"), tr("Button$New"), tr("Button$Delete"));
     else if (category != NULL)
        SetHelp(NULL, tr("Button$New"), NULL, NULL);
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
         const cSearchParameter *p = cSearches::Searches.GetHotKey(hotkey);
         if (p != NULL)
            return AddSubMenu(new cSearchResult(new cSearchParameter(*p)));
         break;
        }
       case kRed:
        {
         if (item != NULL)
            return AddSubMenu(new cSearchMenu(new cSearchParameter(*(item->_parameter))));
         return osContinue;;
        }
       case kGreen:
        {
         cSearchParameter *p = new cSearchParameter();
         if (_base_cat != NULL)
            p->SetCategory(_base_cat);
         return AddSubMenu(new cSearchMenu(p));
         break;
        }
       case kYellow:
        {
         if (item != NULL) {
            if (Interface->Confirm(tr("delete selected search template?"))) {
               cSearches::Searches.LoadSearches();
               cSearchParameter *p = cSearches::Searches.Contains(*(item->_parameter));
               if (p != NULL) {
                  cSearches::Searches.Del(p);
                  cSearches::Searches.Save();
                  Skins.Message(mtInfo, tr("search template was deleted from file"));
                  }
               _cats->Del(item->_item);
               Del(cur);
               Display();
               }
            }
         break;
        }
       case kOk:
        {
          if (item != NULL)
             return AddSubMenu(new cSearchResult(new cSearchParameter(*(item->_parameter))));

          if ((category != NULL) && (category->_sub_cats != NULL)) {
             cList<cNestedItem> *sub_cats = category->_sub_cats->SubItems();
             if ((sub_cats != NULL) && (sub_cats->Count() > 0)) {
                cString next_base_cat = cString::sprintf("%s%s%s", _base_cat ? _base_cat : "", _base_cat ? cSearches::CatDelim : "", category->_sub_cats->Text());
                return AddSubMenu(new cMainMenu(*next_base_cat, sub_cats));
                }
             }
          return osContinue;
        }
       default: break;
       }
     }
  return state;
}
