#ifndef recsearch_search_parameter_h
#define recsearch_search_parameter_h

#include "menu_recordings.h"


namespace recsearch
{
  #define RECSEARCH_TERM_MAX_LEN 100

  class cSearchParameter : public cRecordingFilter
  {
  friend class cSearchMenu;

  private:
    char   _term[RECSEARCH_TERM_MAX_LEN];
    int    _status; // 0 = all, 1 = only new, 2 = only edited

  public:
    static const char *_status_text[3];

    cSearchParameter(void);
    virtual ~cSearchParameter(void);

    virtual bool Filter(const cRecording *Recording);

    bool IsValid();
    bool Parse(const char *s);
    cString ToString(void);

    const char *Term(void) const { return _term; };
    int Status(void) const { return _status; };
    };
}

#endif
