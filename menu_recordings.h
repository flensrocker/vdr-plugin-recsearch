#ifndef recsearch_menu_recordings_h
#define recsearch_menu_recordings_h

// a copy of vdr's cMenuRecordings
// until it will get it's own Filter method

#include <vdr/osdbase.h>


namespace recsearch
{
class cRecordingFilter {
public:
  cRecordingFilter(void) {};
  virtual ~cRecordingFilter(void) {};
  virtual bool Filter(const cRecording *Recording) = 0;
  };

class cMenuRecordings : public cOsdMenu {
private:
  char *base;
  int level;
  int recordingsState;
  int helpKeys;
  cRecordingFilter *filter;
  void SetHelpKeys(void);
  void Set(bool Refresh = false);
  bool Open(bool OpenSubMenus = false);
  eOSState Play(void);
  eOSState Rewind(void);
  eOSState Delete(void);
  eOSState Info(void);
  eOSState Sort(void);
  eOSState Commands(eKeys Key = kNone);
protected:
  cString DirectoryName(void);
public:
  cMenuRecordings(const char *Base = NULL, int Level = 0, bool OpenSubMenus = false, cRecordingFilter *Filter = NULL);
  virtual ~cMenuRecordings();
  virtual eOSState ProcessKey(eKeys Key);
  };
}

#endif
