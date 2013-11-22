#include "menu_recordings.h"

#include <vdr/cutter.h>
#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include <vdr/status.h>
#include <vdr/videodir.h>


// --- cMenuRecording --------------------------------------------------------

namespace recsearch
{
class cMenuRecording : public cOsdMenu {
private:
  const cRecording *recording;
  bool withButtons;
public:
  cMenuRecording(const cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};
}

recsearch::cMenuRecording::cMenuRecording(const cRecording *Recording, bool WithButtons)
:cOsdMenu(trVDR("Recording info"))
{
  SetMenuCategory(mcRecordingInfo);
  recording = Recording;
  withButtons = WithButtons;
  if (withButtons)
     SetHelp(trVDR("Button$Play"), trVDR("Button$Rewind"));
}

void recsearch::cMenuRecording::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetRecording(recording);
  if (recording->Info()->Description())
     cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState recsearch::cMenuRecording::ProcessKey(eKeys Key)
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

// --- cMenuRecordingItem ----------------------------------------------------

namespace recsearch
{
class cMenuRecordingItem : public cOsdItem {
private:
  cRecording *recording;
  int level;
  char *name;
  int totalEntries, newEntries;
public:
  cMenuRecordingItem(cRecording *Recording, int Level);
  ~cMenuRecordingItem();
  void IncrementCounter(bool New);
  const char *Name(void) { return name; }
  cRecording *Recording(void) { return recording; }
  bool IsDirectory(void) { return name != NULL; }
  virtual void SetMenuItem(cSkinDisplayMenu *DisplayMenu, int Index, bool Current, bool Selectable);
  };
}

recsearch::cMenuRecordingItem::cMenuRecordingItem(cRecording *Recording, int Level)
{
  recording = Recording;
  level = Level;
  name = NULL;
  totalEntries = newEntries = 0;
  SetText(Recording->Title('\t', true, Level));
  if (*Text() == '\t')
     name = strdup(Text() + 2); // 'Text() + 2' to skip the two '\t'
}

recsearch::cMenuRecordingItem::~cMenuRecordingItem()
{
  free(name);
}

void recsearch::cMenuRecordingItem::IncrementCounter(bool New)
{
  totalEntries++;
  if (New)
     newEntries++;
  SetText(cString::sprintf("%d\t\t%d\t%s", totalEntries, newEntries, name));
}

void recsearch::cMenuRecordingItem::SetMenuItem(cSkinDisplayMenu *DisplayMenu, int Index, bool Current, bool Selectable)
{
  if (!DisplayMenu->SetItemRecording(recording, Index, Current, Selectable, level, totalEntries, newEntries))
     DisplayMenu->SetItem(Text(), Index, Current, Selectable);
}

// --- cMenuRecordings -------------------------------------------------------

recsearch::cMenuRecordings::cMenuRecordings(const char *Base, int Level, bool OpenSubMenus, cRecordingFilter *Filter)
:cOsdMenu(Base ? Base : trVDR("Recordings"), 9, 6, 6)
,filter(Filter)
{
  SetMenuCategory(mcRecording);
  base = Base ? strdup(Base) : NULL;
  level = Setup.RecordingDirs ? Level : -1;
  Recordings.StateChanged(recordingsState); // just to get the current state
  helpKeys = -1;
  Display(); // this keeps the higher level menus from showing up briefly when pressing 'Back' during replay
  Set();
  if (Current() < 0)
     SetCurrent(First());
  else if (OpenSubMenus && cReplayControl::LastReplayed() && Open(true))
     return;
  Display();
  SetHelpKeys();
}

recsearch::cMenuRecordings::~cMenuRecordings()
{
  helpKeys = -1;
  free(base);
}

void recsearch::cMenuRecordings::SetHelpKeys(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  int NewHelpKeys = 0;
  if (ri) {
     if (ri->IsDirectory())
        NewHelpKeys = 1;
     else {
        NewHelpKeys = 2;
        if (ri->Recording()->Info()->Title())
           NewHelpKeys = 3;
        }
     }
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(trVDR("Button$Open")); break;
       case 2:
       case 3: SetHelp(RecordingCommands.Count() ? trVDR("Commands") : trVDR("Button$Play"), trVDR("Button$Rewind"), trVDR("Button$Delete"), NewHelpKeys == 3 ? trVDR("Button$Info") : NULL);
       default: ;
       }
     helpKeys = NewHelpKeys;
     }
}

void recsearch::cMenuRecordings::Set(bool Refresh)
{
  const char *CurrentRecording = cReplayControl::LastReplayed();
  cMenuRecordingItem *LastItem = NULL;
  cThreadLock RecordingsLock(&Recordings);
  if (Refresh) {
     if (cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current()))
        CurrentRecording = ri->Recording()->FileName();
     }
  Clear();
  GetRecordingsSortMode(DirectoryName());
  Recordings.Sort();
  for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
      if ((!filter || filter->Filter(recording)) && (!base || (strstr(recording->Name(), base) == recording->Name() && recording->Name()[strlen(base)] == FOLDERDELIMCHAR))) {
         cMenuRecordingItem *Item = new cMenuRecordingItem(recording, level);
         cMenuRecordingItem *LastDir = NULL;
         if (Item->IsDirectory()) {
            // Sorting may ignore non-alphanumeric characters, so we need to explicitly handle directories in case they only differ in such characters:
            for (cMenuRecordingItem *p = LastItem; p; p = dynamic_cast<cMenuRecordingItem *>(p->Prev())) {
                if (p->Name() && strcmp(p->Name(), Item->Name()) == 0) {
                   LastDir = p;
                   break;
                   }
                }
            }
         if (*Item->Text() && !LastDir) {
            Add(Item);
            LastItem = Item;
            if (Item->IsDirectory())
               LastDir = Item;
            }
         else
            delete Item;
         if (LastItem || LastDir) {
            if (CurrentRecording && strcmp(CurrentRecording, recording->FileName()) == 0)
               SetCurrent(LastDir ? LastDir : LastItem);
            }
         if (LastDir)
            LastDir->IncrementCounter(recording->IsNew());
         }
      }
  if (Refresh)
     Display();
}

cString recsearch::cMenuRecordings::DirectoryName(void)
{
  cString d(VideoDirectory);
  if (base) {
     char *s = ExchangeChars(strdup(base), true);
     d = AddDirectory(d, s);
     free(s);
     }
  return d;
}

bool recsearch::cMenuRecordings::Open(bool OpenSubMenus)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && ri->IsDirectory()) {
     const char *t = ri->Name();
     cString buffer;
     if (base) {
        buffer = cString::sprintf("%s~%s", base, t);
        t = buffer;
        }
     AddSubMenu(new cMenuRecordings(t, level + 1, OpenSubMenus));
     return true;
     }
  return false;
}

eOSState recsearch::cMenuRecordings::Play(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri) {
     if (ri->IsDirectory())
        Open();
     else {
        cReplayControl::SetRecording(ri->Recording()->FileName());
        return osReplay;
        }
     }
  return osContinue;
}

eOSState recsearch::cMenuRecordings::Rewind(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cDevice::PrimaryDevice()->StopReplay(); // must do this first to be able to rewind the currently replayed recording
     cResumeFile ResumeFile(ri->Recording()->FileName(), ri->Recording()->IsPesRecording());
     ResumeFile.Delete();
     return Play();
     }
  return osContinue;
}

eOSState recsearch::cMenuRecordings::Delete(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     if (Interface->Confirm(trVDR("Delete recording?"))) {
        cRecordControl *rc = cRecordControls::GetRecordControl(ri->Recording()->FileName());
        if (rc) {
           if (Interface->Confirm(trVDR("Timer still recording - really delete?"))) {
              cTimer *timer = rc->Timer();
              if (timer) {
                 timer->Skip();
                 cRecordControls::Process(time(NULL));
                 if (timer->IsSingleEvent()) {
                    isyslog("deleting timer %s", *timer->ToDescr());
                    Timers.Del(timer);
                    }
                 Timers.SetModified();
                 }
              }
           else
              return osContinue;
           }
        cRecording *recording = ri->Recording();
        cString FileName = recording->FileName();
        if (cCutter::Active(ri->Recording()->FileName())) {
           if (Interface->Confirm(trVDR("Recording is being edited - really delete?"))) {
              cCutter::Stop();
              recording = Recordings.GetByName(FileName); // cCutter::Stop() might have deleted it if it was the edited version
              // we continue with the code below even if recording is NULL,
              // in order to have the menu updated etc.
              }
           else
              return osContinue;
           }
        if (cReplayControl::NowReplaying() && strcmp(cReplayControl::NowReplaying(), FileName) == 0)
           cControl::Shutdown();
        if (!recording || recording->Delete()) {
           cReplayControl::ClearLastReplayed(FileName);
           Recordings.DelByName(FileName);
           cOsdMenu::Del(Current());
           SetHelpKeys();
           cVideoDiskUsage::ForceCheck();
           Display();
           if (!Count())
              return osBack;
           }
        else
           Skins.Message(mtError, trVDR("Error while deleting recording!"));
        }
     }
  return osContinue;
}

eOSState recsearch::cMenuRecordings::Info(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory() && ri->Recording()->Info()->Title())
     return AddSubMenu(new cMenuRecording(ri->Recording(), true));
  return osContinue;
}

eOSState recsearch::cMenuRecordings::Commands(eKeys Key)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  if (ri && !ri->IsDirectory()) {
     cMenuCommands *menu;
     eOSState state = AddSubMenu(menu = new cMenuCommands(trVDR("Recording commands"), &RecordingCommands, cString::sprintf("\"%s\"", *strescape(ri->Recording()->FileName(), "\\\"$"))));
     if (Key != kNone)
        state = menu->ProcessKey(Key);
     return state;
     }
  return osContinue;
}

eOSState recsearch::cMenuRecordings::Sort(void)
{
  if (HasSubMenu())
     return osContinue;
  IncRecordingsSortMode(DirectoryName());
  Set(true);
  return osContinue;
}

eOSState recsearch::cMenuRecordings::ProcessKey(eKeys Key)
{
  bool HadSubMenu = HasSubMenu();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kPlayPause:
       case kPlay:
       case kOk:     return Play();
       case kRed:    return (helpKeys > 1 && RecordingCommands.Count()) ? Commands() : Play();
       case kGreen:  return Rewind();
       case kYellow: return Delete();
       case kInfo:
       case kBlue:   return Info();
       case k0:      return Sort();
       case k1...k9: return Commands(Key);
       case kNone:   if (Recordings.StateChanged(recordingsState))
                        Set(true);
                     break;
       default: break;
       }
     }
  if (Key == kYellow && HadSubMenu && !HasSubMenu()) {
     // the last recording in a subdirectory was deleted, so let's go back up
     cOsdMenu::Del(Current());
     if (!Count())
        return osBack;
     Display();
     }
  if (!HasSubMenu()) {
     if (Key != kNone)
        SetHelpKeys();
     }
  return state;
}
