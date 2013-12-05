#include "menu_recordings.h"

#include <vdr/cutter.h>
#include <vdr/interface.h>
#include <vdr/menu.h>
#include <vdr/remote.h>
#include <vdr/status.h>
#include <vdr/videodir.h>

// --- cMenuPathEdit ---------------------------------------------------------

namespace recsearch
{
class cMenuPathEdit : public cOsdMenu {
private:
  cString path;
  char folder[PATH_MAX];
  char name[NAME_MAX];
  cMenuEditStrItem *folderItem;
  int pathIsInUse;
  eOSState SetFolder(void);
  eOSState Folder(void);
  eOSState ApplyChanges(void);
public:
  cMenuPathEdit(const char *Path);
  virtual eOSState ProcessKey(eKeys Key);
  };
}

recsearch::cMenuPathEdit::cMenuPathEdit(const char *Path)
:cOsdMenu(trVDR("Edit path"), 12)
{
  SetMenuCategory(mcRecording);
  path = Path;
  *folder = 0;
  *name = 0;
  const char *s = strrchr(path, FOLDERDELIMCHAR);
  if (s) {
     strn0cpy(folder, cString(path, s), sizeof(folder));
     s++;
     }
  else
     s = path;
  strn0cpy(name, s, sizeof(name));
  pathIsInUse = Recordings.PathIsInUse(path);
  cOsdItem *p;
  Add(p = folderItem = new cMenuEditStrItem(trVDR("Folder"), folder, sizeof(folder)));
  p->SetSelectable(!pathIsInUse);
  Add(p = new cMenuEditStrItem(trVDR("Name"), name, sizeof(name)));
  p->SetSelectable(!pathIsInUse);
  if (pathIsInUse) {
     Add(new cOsdItem("", osUnknown, false));
     Add(new cOsdItem(trVDR("This folder is currently in use - no changes are possible!"), osUnknown, false));
     }
  Display();
  if (!pathIsInUse)
     SetHelp(trVDR("Button$Folder"));
}

eOSState recsearch::cMenuPathEdit::SetFolder(void)
{
  if (cMenuFolder *mf = dynamic_cast<cMenuFolder *>(SubMenu())) {
     strn0cpy(folder, mf->GetFolder(), sizeof(folder));
     SetCurrent(folderItem);
     Display();
     }
  return CloseSubMenu();
}

eOSState recsearch::cMenuPathEdit::Folder(void)
{
  return AddSubMenu(new cMenuFolder(trVDR("Select folder"), &Folders, path));
}

eOSState recsearch::cMenuPathEdit::ApplyChanges(void)
{
  if (!*name)
     *name = ' '; // name must not be empty!
  cString NewPath = *folder ? cString::sprintf("%s%c%s", folder, FOLDERDELIMCHAR, name) : name;
  NewPath.CompactChars(FOLDERDELIMCHAR);
  if (strcmp(NewPath, path)) {
     int NumRecordings = Recordings.GetNumRecordingsInPath(path);
     if (NumRecordings > 1 && !Interface->Confirm(cString::sprintf(trVDR("Move entire folder containing %d recordings?"), NumRecordings)))
        return osContinue;
     if (!Recordings.MoveRecordings(path, NewPath)) {
        Skins.Message(mtError, trVDR("Error while moving folder!"));
        return osContinue;
        }
     cMenuRecordings::SetPath(NewPath); // makes sure the Recordings menu will reposition to the new path
     return osUser1;
     }
  return osBack;
}

eOSState recsearch::cMenuPathEdit::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
     if (!pathIsInUse) {
        switch (Key) {
          case kRed: return Folder();
          case kOk:  return ApplyChanges();
          default: break;
          }
        }
     else if (Key == kOk)
        return osBack;
     }
  else if (state == osEnd && HasSubMenu())
     state = SetFolder();
  return state;
}

// --- cMenuRecordingEdit ----------------------------------------------------

namespace recsearch
{
class cMenuRecordingEdit : public cOsdMenu {
private:
  cRecording *recording;
  cString originalFileName;
  int recordingsState;
  char folder[PATH_MAX];
  char name[NAME_MAX];
  int priority;
  int lifetime;
  cMenuEditStrItem *folderItem;
  const char *buttonFolder;
  const char *buttonAction;
  const char *buttonDeleteMarks;
  const char *actionCancel;
  const char *doCut;
  int recordingIsInUse;
  void Set(void);
  void SetHelpKeys(void);
  bool RefreshRecording(void);
  eOSState SetFolder(void);
  eOSState Folder(void);
  eOSState Action(void);
  eOSState DeleteMarks(void);
  eOSState ApplyChanges(void);
public:
  cMenuRecordingEdit(cRecording *Recording);
  virtual eOSState ProcessKey(eKeys Key);
  };
}

recsearch::cMenuRecordingEdit::cMenuRecordingEdit(cRecording *Recording)
:cOsdMenu(trVDR("Edit recording"), 12)
{
  SetMenuCategory(mcRecording);
  recording = Recording;
  originalFileName = recording->FileName();
  Recordings.StateChanged(recordingsState); // just to get the current state
  strn0cpy(folder, recording->Folder(), sizeof(folder));
  strn0cpy(name, recording->BaseName(), sizeof(name));
  priority = recording->Priority();
  lifetime = recording->Lifetime();
  folderItem = NULL;
  buttonFolder = NULL;
  buttonAction = NULL;
  buttonDeleteMarks = NULL;
  actionCancel = NULL;
  doCut = NULL;
  recordingIsInUse = ruNone;
  Set();
}

void recsearch::cMenuRecordingEdit::Set(void)
{
  int current = Current();
  Clear();
  recordingIsInUse = recording->IsInUse();
  cOsdItem *p;
  Add(p = folderItem = new cMenuEditStrItem(trVDR("Folder"), folder, sizeof(folder)));
  p->SetSelectable(!recordingIsInUse);
  Add(p = new cMenuEditStrItem(trVDR("Name"), name, sizeof(name)));
  p->SetSelectable(!recordingIsInUse);
  Add(p = new cMenuEditIntItem(trVDR("Priority"), &priority, 0, MAXPRIORITY));
  p->SetSelectable(!recordingIsInUse);
  Add(p = new cMenuEditIntItem(trVDR("Lifetime"), &lifetime, 0, MAXLIFETIME));
  p->SetSelectable(!recordingIsInUse);
  if (recordingIsInUse) {
     Add(new cOsdItem("", osUnknown, false));
     Add(new cOsdItem(trVDR("This recording is currently in use - no changes are possible!"), osUnknown, false));
     }
  SetCurrent(Get(current));
  Display();
  SetHelpKeys();
}

void recsearch::cMenuRecordingEdit::SetHelpKeys(void)
{
  buttonFolder = !recordingIsInUse ? trVDR("Button$Folder") : NULL;
  buttonAction = NULL;
  buttonDeleteMarks = NULL;
  actionCancel = NULL;
  doCut = NULL;
  if ((recordingIsInUse & ruCut) != 0)
     buttonAction = actionCancel = ((recordingIsInUse & ruPending) != 0) ? trVDR("Button$Cancel cutting") : trVDR("Button$Stop cutting");
  else if ((recordingIsInUse & ruMove) != 0)
     buttonAction = actionCancel = ((recordingIsInUse & ruPending) != 0) ? trVDR("Button$Cancel moving") : trVDR("Button$Stop moving");
  else if ((recordingIsInUse & ruCopy) != 0)
     buttonAction = actionCancel = ((recordingIsInUse & ruPending) != 0) ? trVDR("Button$Cancel copying") : trVDR("Button$Stop copying");
  else if (recording->HasMarks()) {
     buttonAction = doCut = trVDR("Button$Cut");
     buttonDeleteMarks = trVDR("Button$Delete marks");
     }
  SetHelp(buttonFolder, buttonAction, buttonDeleteMarks);
}

bool recsearch::cMenuRecordingEdit::RefreshRecording(void)
{
  if (Recordings.StateChanged(recordingsState)) {
     if ((recording = Recordings.GetByName(originalFileName)) != NULL)
        Set();
     else {
        Skins.Message(mtWarning, trVDR("Recording vanished!"));
        return false;
        }
     }
  return true;
}

eOSState recsearch::cMenuRecordingEdit::SetFolder(void)
{
  if (cMenuFolder *mf = dynamic_cast<cMenuFolder *>(SubMenu())) {
     strn0cpy(folder, mf->GetFolder(), sizeof(folder));
     SetCurrent(folderItem);
     Display();
     }
  return CloseSubMenu();
}

eOSState recsearch::cMenuRecordingEdit::Folder(void)
{
  return AddSubMenu(new cMenuFolder(trVDR("Select folder"), &Folders, recording->Name()));
}

eOSState recsearch::cMenuRecordingEdit::Action(void)
{
  if (actionCancel)
     RecordingsHandler.Del(recording->FileName());
  else if (doCut) {
     if (!RecordingsHandler.Add(ruCut, recording->FileName()))
        Skins.Message(mtError, trVDR("Error while queueing recording for cutting!"));
     }
  recordingIsInUse = recording->IsInUse();
  RefreshRecording();
  SetHelpKeys();
  return osContinue;
}

eOSState recsearch::cMenuRecordingEdit::DeleteMarks(void)
{
  if (buttonDeleteMarks && Interface->Confirm(trVDR("Delete editing marks for this recording?"))) {
     if (recording->DeleteMarks())
        SetHelpKeys();
     else
        Skins.Message(mtError, trVDR("Error while deleting editing marks!"));
     }
  return osContinue;
}

eOSState recsearch::cMenuRecordingEdit::ApplyChanges(void)
{
  bool Modified = false;
  if (priority != recording->Priority() || lifetime != recording->Lifetime()) {
     if (!recording->ChangePriorityLifetime(priority, lifetime)) {
        Skins.Message(mtError, trVDR("Error while changing priority/lifetime!"));
        return osContinue;
        }
     Modified = true;
     }
  if (!*name)
     *name = ' '; // name must not be empty!
  cString NewName = *folder ? cString::sprintf("%s%c%s", folder, FOLDERDELIMCHAR, name) : name;
  NewName.CompactChars(FOLDERDELIMCHAR);
  if (strcmp(NewName, recording->Name())) {
     if (!recording->ChangeName(NewName)) {
        Skins.Message(mtError, trVDR("Error while changing folder/name!"));
        return osContinue;
        }
     Modified = true;
     }
  if (Modified) {
     cMenuRecordings::SetRecording(recording->FileName()); // makes sure the Recordings menu will reposition to the renamed recording
     return osUser1;
     }
  return osBack;
}

eOSState recsearch::cMenuRecordingEdit::ProcessKey(eKeys Key)
{
  if (!HasSubMenu()) {
     if (!RefreshRecording())
        return osBack; // the recording has vanished, so close this menu
     }
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
     switch (Key) {
       case kRed:    return buttonFolder ? Folder() : osContinue;
       case kGreen:  return buttonAction ? Action() : osContinue;
       case kYellow: return buttonDeleteMarks ? DeleteMarks() : osContinue;
       case kOk:     return !recordingIsInUse ? ApplyChanges() : osBack;
       default: break;
       }
     }
  else if (state == osEnd && HasSubMenu())
     state = SetFolder();
  return state;
}

// --- cMenuRecording --------------------------------------------------------

namespace recsearch
{
class cMenuRecording : public cOsdMenu {
private:
  cRecording *recording;
  cString originalFileName;
  int recordingsState;
  bool withButtons;
  bool RefreshRecording(void);
public:
  cMenuRecording(cRecording *Recording, bool WithButtons = false);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};
}

recsearch::cMenuRecording::cMenuRecording(cRecording *Recording, bool WithButtons)
:cOsdMenu(trVDR("Recording info"))
{
  SetMenuCategory(mcRecordingInfo);
  recording = Recording;
  originalFileName = recording->FileName();
  Recordings.StateChanged(recordingsState); // just to get the current state
  withButtons = WithButtons;
  if (withButtons)
     SetHelp(trVDR("Button$Play"), trVDR("Button$Rewind"), NULL, trVDR("Button$Edit"));
}

bool recsearch::cMenuRecording::RefreshRecording(void)
{
  if (Recordings.StateChanged(recordingsState)) {
     if ((recording = Recordings.GetByName(originalFileName)) != NULL)
        Display();
     else {
        Skins.Message(mtWarning, trVDR("Recording vanished!"));
        return false;
        }
     }
  return true;
}

void recsearch::cMenuRecording::Display(void)
{
  if (HasSubMenu()) {
     SubMenu()->Display();
     return;
     }
  cOsdMenu::Display();
  DisplayMenu()->SetRecording(recording);
  if (recording->Info()->Description())
     cStatus::MsgOsdTextItem(recording->Info()->Description());
}

eOSState recsearch::cMenuRecording::ProcessKey(eKeys Key)
{
  if (HasSubMenu()) {
     eOSState state = cOsdMenu::ProcessKey(Key);
     if (state == osUser1)
        CloseSubMenu();
     return state;
     }
  else if (!RefreshRecording())
     return osBack; // the recording has vanished, so close this menu
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
       case kBlue:   if (withButtons)
                        return AddSubMenu(new cMenuRecordingEdit(recording));
                     break;
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
  int Level(void) { return level; }
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

cString recsearch::cMenuRecordings::path;
cString recsearch::cMenuRecordings::fileName;

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
  else if (OpenSubMenus && (cReplayControl::LastReplayed() || *path || *fileName)) {
     if (!*path || Level < strcountchr(path, FOLDERDELIMCHAR)) {
        if (Open(true))
           return;
        }
     }
  Display();
  SetHelpKeys();
}

recsearch::cMenuRecordings::~cMenuRecordings()
{
  helpKeys = -1;
  free(base);
  if (filter)
     delete filter;
}

void recsearch::cMenuRecordings::SetHelpKeys(void)
{
  cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current());
  int NewHelpKeys = 0;
  if (ri) {
     if (ri->IsDirectory())
        NewHelpKeys = 1;
     else
        NewHelpKeys = 2;
     }
  if (NewHelpKeys != helpKeys) {
     switch (NewHelpKeys) {
       case 0: SetHelp(NULL); break;
       case 1: SetHelp(trVDR("Button$Open"), NULL, NULL, trVDR("Button$Edit")); break;
       case 2: SetHelp(RecordingCommands.Count() ? trVDR("Commands") : trVDR("Button$Play"), trVDR("Button$Rewind"), trVDR("Button$Delete"), trVDR("Button$Info"));
       default: ;
       }
     helpKeys = NewHelpKeys;
     }
}

void recsearch::cMenuRecordings::Set(bool Refresh)
{
  const char *CurrentRecording = *fileName ? *fileName : cReplayControl::LastReplayed();
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
            if (*path) {
               if (strcmp(path, recording->Folder()) == 0)
                  SetCurrent(LastDir ? LastDir : LastItem);
               }
            else if (CurrentRecording && strcmp(CurrentRecording, recording->FileName()) == 0)
               SetCurrent(LastDir ? LastDir : LastItem);
            }
         if (LastDir)
            LastDir->IncrementCounter(recording->IsNew());
         }
      }
  if (Refresh)
     Display();
}

void recsearch::cMenuRecordings::SetPath(const char *Path)
{
  path = Path;
}

void recsearch::cMenuRecordings::SetRecording(const char *FileName)
{
  fileName = FileName;
}

cString recsearch::cMenuRecordings::DirectoryName(void)
{
  cString d(cVideoDirectory::Name());
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
  if (ri && ri->IsDirectory() && (!*path || strcountchr(path, FOLDERDELIMCHAR) > 0)) {
     const char *t = ri->Name();
     cString buffer;
     if (base) {
        buffer = cString::sprintf("%s%c%s", base, FOLDERDELIMCHAR, t);
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
        if (RecordingsHandler.GetUsage(FileName)) {
           if (Interface->Confirm(trVDR("Recording is being edited - really delete?"))) {
              RecordingsHandler.Del(FileName);
              recording = Recordings.GetByName(FileName); // RecordingsHandler.Del() might have deleted it if it was the edited version
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
  if (cMenuRecordingItem *ri = (cMenuRecordingItem *)Get(Current())) {
     if (ri->IsDirectory())
        return AddSubMenu(new cMenuPathEdit(cString(ri->Recording()->Name(), strchrn(ri->Recording()->Name(), FOLDERDELIMCHAR, ri->Level() + 1))));
     else
        return AddSubMenu(new cMenuRecording(ri->Recording(), true));
     }
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
  else if (state == osUser1) {
     // a recording or path was renamed, so let's refresh the menu
     CloseSubMenu(false);
     if (base)
        return state; // closes all recording menus except for the top one
     Set(); // this is the top level menu, so we refresh it...
     Open(true); // ...and open any necessary submenus to show the new name
     Display();
     path = NULL;
     fileName = NULL;
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
