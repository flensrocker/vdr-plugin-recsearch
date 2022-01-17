#ifndef recsearch_search_parameter_h
#define recsearch_search_parameter_h

#include <vdr/menu.h>

#if APIVERSNUM < 20103
#include "recording_filter.h"
#endif


namespace recsearch
{
  #define RECSEARCH_MAX_LEN 100

  bool filter_matches(const char *text, const char *term);

  class cSearchParameter : public cListObject, public cRecordingFilter
  {
  friend class cSearchMenu;

  private:
    static const char *_status_text[3];

    char _name[RECSEARCH_MAX_LEN];
    char _category[RECSEARCH_MAX_LEN];
    mutable char _term[RECSEARCH_MAX_LEN];
    int _status;            // 0 = all, 1 = only new, 2 = only edited
    int _younger_than_days; // only used if > 0
    int _hot_key;           // 1 to 9, 0 = no key

    cStringList _splitted_terms;

    void SplitTerms(void);

  public:
    cSearchParameter(void);
    cSearchParameter(const cSearchParameter &Parameter);
    virtual ~cSearchParameter(void);

    cSearchParameter &operator=(const cSearchParameter &Parameter);

    virtual int Compare(const cListObject &ListObject) const;
    virtual bool Filter(const cRecording *Recording) const;

    void Clear(void);
    bool IsValid(void) const;
    bool Parse(const char *s);
    bool Save(FILE *f);
    cString ToString(void) const; // counterpart to Parse
    cString ToText(void) const;   // userfriendly, localized on-screen representation

    void SetCategory(const char *Category);
    void SetName(const char *Name);

    const char *Name(void) const { return _name; };
    const char *Category(void) const { return _category; };
    const char *Term(void) const { return _term; };
    int Status(void) const { return _status; };
    int HotKey(void) const { return _hot_key; };
  };

  class cSearches : public cConfig<cSearchParameter>
  {
  private:
    cString  _filename;

  public:
    static cSearches Last;
    static cSearches Searches;
    static const char *CatDelim;

    void SetFilename(const cString &Filename) { _filename = Filename; };
    const cSearchParameter *Contains(const cSearchParameter &Parameter) const;
    const cSearchParameter *GetHotKey(int HotKey) const;
    void GetCategories(cStringList &Categories) const;
    void GetCatMenus(cList<cNestedItem> *CatMenus) const;
    bool LoadSearches(void);
  };
}

#endif
