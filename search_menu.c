#include "search_menu.h"

#include <vdr/menu.h>
#include <vdr/remote.h>
#include <vdr/status.h>


// --- cSearchMenu -----------------------------------------------------------

recsearch::cSearchMenu::cSearchMenu(void)
 : cOsdMenu(tr("search recordings"), 12)
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
             isyslog("recsearch: searching for %s, status = %d", _search_term, _status);
             _parameter._search_term = _search_term;
             _parameter._search_status = _status;
             return AddSubMenu(new cSearchResult(&_parameter));
             }
          return osBack;
        }
       default: break;
       }
     }
  return state;
}


// --- cMenuRecording (copied from vdr source) -------------------------------

class cMenuRecording : public cOsdMenu {
private:
  const cRecording *recording;
  bool withButtons;
public:
  cMenuRecording(const cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};

cMenuRecording::cMenuRecording(const cRecording *Recording, bool WithButtons)
:cOsdMenu(trVDR("Recording info"))
{
  SetMenuCategory(mcRecordingInfo);
  recording = Recording;
  withButtons = WithButtons;
  if (withButtons)
     SetHelp(trVDR("Button$Play"), trVDR("Button$Rewind"));
}

void cMenuRecording::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetRecording(recording);
  if (recording->Info()->Description())
     cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState cMenuRecording::ProcessKey(eKeys Key)
{
  switch (int(Key)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft);
                  return osContinue;
    case kInfo:   return osBack;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:    if (withButtons)
                        Key = kOk; // will play the recording, even if recording commands are defined
       case kGreen:  if (!withButtons)
                        break;
                     cRemote::Put(Key, true);
                     // continue with osBack to close the info menu and process the key
       case kOk:     return osBack;
       default: break;
       }
     }
  return state;
}


// --- cSearchResultItem -----------------------------------------------------

class cSearchResultItem : public cOsdItem {
private:
  cRecording *recording;
public:
  cSearchResultItem(cRecording *Recording);
  ~cSearchResultItem();
  cRecording *Recording(void) { return recording; }
  virtual void SetMenuItem(cSkinDisplayMenu *DisplayMenu, int Index, bool Current, bool Selectable);
  };

cSearchResultItem::cSearchResultItem(cRecording *Recording)
{
  recording = Recording;
  if (Recording->Info()->ShortText())
     SetText(*cString::sprintf("%s~%s", Recording->Info()->Title(), Recording->Info()->ShortText()));
  else
     SetText(Recording->Info()->Title());
}

cSearchResultItem::~cSearchResultItem()
{
}

void cSearchResultItem::SetMenuItem(cSkinDisplayMenu *DisplayMenu, int Index, bool Current, bool Selectable)
{
  if (!DisplayMenu->SetItemRecording(recording, Index, Current, Selectable, -1, 0, 0))
     DisplayMenu->SetItem(Text(), Index, Current, Selectable);
}


// --- cSearchResult ---------------------------------------------------------

recsearch::cSearchResult::cSearchResult(cSearchParameter *Parameter)
 : cOsdMenu(tr("search result"))
 ,_parameter(Parameter)
{
  SetTitle(tr("searching..."));
  SetMenuCategory(mcRecording);
  Display();

  _parameter->_host = this;
  _parameter->_result.Clear();
  cSearchProvider::StartSearch(_parameter);
}

recsearch::cSearchResult::~cSearchResult(void)
{
  cSearchProvider::StopSearch();
}

cRecording *recsearch::cSearchResult::GetSelectedRecording(void)
{
  int i = Current();
  if ((i >= 0) && (i < _parameter->_result.Count()))
     return _parameter->_result.Get(i);
  return NULL;
}

eOSState recsearch::cSearchResult::Play(void)
{
  cRecording *r = GetSelectedRecording();
  if (r != NULL) {
     cReplayControl::SetRecording(r->FileName());
     return osReplay;
     }
  return osContinue;
}

eOSState recsearch::cSearchResult::Rewind(void)
{
  cRecording *r = GetSelectedRecording();
  if (r != NULL) {
     cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
     cResumeFile ResumeFile(r->FileName(), r->IsPesRecording());
     ResumeFile.Delete();
     return Play();
     }
  return osContinue;
}

eOSState recsearch::cSearchResult::Info(void)
{
  cRecording *r = GetSelectedRecording();
  if (r && r->Info()->Title())
     return AddSubMenu(new cMenuRecording(r, true));
  return osContinue;
}

eOSState recsearch::cSearchResult::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
      case kPlayPause:
      case kPlay:
      case kRed:
      case kOk:     return Play();
      case kGreen:  return Rewind();
      case kInfo:
      case kBlue:   return Info();
      default: break;
      }
    }
  return state;
}

void  recsearch::cSearchResult::SearchDone(void)
{
  cString title = cString::sprintf("%s: %s", tr("search result"), *_parameter->_search_term);
  if (_parameter->_search_status == 1)
     title = cString::sprintf("%s, %s", *title, tr("only new"));
  else if (_parameter->_search_status == 2)
     title = cString::sprintf("%s, %s", *title, tr("only edited"));
  SetTitle(*title);

  if (_parameter->_result.Count() == 0) {
     Add(new cOsdItem(tr("nothing found"), osUnknown, false));
     SetHelp(NULL);
     }
  else {
     for (cRecording *r = _parameter->_result.First(); r; r = _parameter->_result.Next(r))
         Add(new cSearchResultItem(r));
     SetHelp(trVDR("Button$Play"), trVDR("Button$Rewind"), NULL, trVDR("Button$Info"));
     }
  Display();
}
