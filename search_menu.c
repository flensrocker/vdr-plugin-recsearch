#include "search_menu.h"

#include <vdr/menu.h>
#include <vdr/remote.h>
#include <vdr/status.h>


// --- cSearchMenu -----------------------------------------------------------

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


// --- cSearchResult ---------------------------------------------------------

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

cRecording *recsearch::cSearchResult::GetSelectedRecording(void)
{
  int i = Current();
  if ((i >= 0) && (i < _parameter._result.Count()))
     return _parameter._result.Get(i);
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
  dsyslog("recsearch: cSearchResult::SearchDone");
  SetTitle(*cString::sprintf("%s: %s", tr("search result"), *_parameter._search_term));
  if (_parameter._result.Count() == 0) {
     Add(new cOsdItem(tr("nothing found"), osUnknown, false));
     SetHelp(NULL);
     }
  else {
     for (cRecording *r = _parameter._result.First(); r; r = _parameter._result.Next(r)) {
         if (r->Info()->ShortText() == NULL)
            Add(new cOsdItem(r->Info()->Title()));
         else
            Add(new cOsdItem(*cString::sprintf("%s~%s", r->Info()->Title(), r->Info()->ShortText())));
         }
     SetHelp(trVDR("Button$Play"), trVDR("Button$Rewind"), NULL, trVDR("Button$Info"));
     }
  Display();
}
