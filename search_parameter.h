#ifndef recsearch_search_parameter_h
#define recsearch_search_parameter_h

#include "menu_recordings.h"


namespace recsearch
{
  #define RECSEARCH_TERM_MAX_LEN 100

  class cSearchParameter : public cListObject, public cRecordingFilter
  {
  friend class cSearchMenu;

  private:
    static const char *_status_text[3];

    mutable char _term[RECSEARCH_TERM_MAX_LEN];
    int _status; // 0 = all, 1 = only new, 2 = only edited
    int _younger_than_days; // only used if > 0

  public:
    cSearchParameter(void);
    cSearchParameter(const cSearchParameter &Parameter);
    virtual ~cSearchParameter(void);

    virtual int Compare(const cListObject &ListObject) const;
    virtual bool Filter(const cRecording *Recording);

    void Clear(void);
    bool IsValid(void) const;
    bool Parse(const char *s);
    bool Save(FILE *f);
    cString ToString(void) const;
    cString ToText(void) const;

    const char *Term(void) const { return _term; };
    int Status(void) const { return _status; };
  };

  class cSearches : public cConfig<cSearchParameter>
  {
  private:
    cString  _filename;

  public:
    static cSearches Last;
    static cSearches Searches;

    void SetFilename(const cString &Filename) { _filename = Filename; };
    cSearchParameter *Contains(const cSearchParameter &Parameter) const;
    bool LoadSearches(void);
  };
}

#endif
