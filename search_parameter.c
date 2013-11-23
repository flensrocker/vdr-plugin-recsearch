#include "search_parameter.h"


class cParameterHelper
{
private:
  cStringList  _list;

public:
  // instantiate with an option string like "key1=value1,key2=value2,key2=value3,key3=value4,..."
  // comma and backslashes in values must be escaped with a backslash
  cParameterHelper(const char *Options)
  {
    if (Options != NULL) {
       char *s = strdup(Options);
       size_t len = strlen(Options);
       size_t pos1 = 0;
       // skip initial commas
       while ((pos1 < len) && (s[pos1] == ','))
             pos1++;
       size_t pos2 = pos1;
       while (pos2 < len) {
             // de-escape backslashed characters
             if ((s[pos2] == '\\') && (pos2 < (len - 1))) {
                memmove(s + pos2, s + pos2 + 1, len - pos2 - 1);
                len--;
                s[len] = 0;
                pos2++;
                continue;
                }
             if ((s[pos2] == ',') && (s[pos2 - 1] != '\\')) {
                s[pos2] = 0;
                _list.Append(strdup(s + pos1));
                pos1 = pos2 + 1;
                }
             pos2++;
             }
       if (pos2 > pos1)
          _list.Append(strdup(s + pos1));
       free(s);
       }
  };

  // if Number is greater than zero, returns the n'th appearance of the parameter
  const char *Get(const char *Name, int Number = 0)
  {
    if (Name == NULL)
       return NULL;
    size_t name_len = strlen(Name);
    if (name_len == 0)
       return NULL;

    for (int i = 0; i < _list.Size(); i++) {
        const char *text = _list[i];
        if (text == NULL)
           continue;
        size_t len = strlen(text);
        if (len > name_len) {
           if ((strncmp(text, Name, name_len) == 0) && (text[name_len] == '=')) {
              Number--;
              if (Number < 0)
                 return text + name_len + 1;
              }
           }
        }
    return NULL;
  };

  // get the number of appearances of parameter "Name"
  int Count(const char *Name)
  {
    if (Name == NULL)
       return 0;
    size_t name_len = strlen(Name);
    if (name_len == 0)
       return 0;

    int count = 0;
    for (int i = 0; i < _list.Size(); i++) {
        const char *text = _list[i];
        if (text == NULL)
           continue;
        size_t len = strlen(text);
        if (len > name_len) {
           if ((strncmp(text, Name, name_len) == 0) && (text[name_len] == '='))
              count++;
           }
        }
    return count;
  };
};


const char *recsearch::cSearchParameter::_status_text[3] = {
                                                            tr("all"),
                                                            tr("only new"),
                                                            tr("only edited")
                                                           };

recsearch::cSearchParameter::cSearchParameter(void)
{
  Clear();
}

recsearch::cSearchParameter::cSearchParameter(const cSearchParameter &Parameter)
{
  memcpy(_term, Parameter._term, RECSEARCH_TERM_MAX_LEN);
  _status = Parameter._status;
}

recsearch::cSearchParameter::~cSearchParameter(void)
{
}

int recsearch::cSearchParameter::Compare(const cListObject &ListObject) const
{
  const cSearchParameter& rhs = (const cSearchParameter&)ListObject;
  int cmp = strcasecmp(_term, rhs._term);
  if (cmp != 0)
     return cmp;
  if (_status < rhs._status)
     return -1;
  if (_status > rhs._status)
     return 1;
  return 0;
}

bool recsearch::cSearchParameter::Filter(const cRecording *Recording)
{
  if (Recording == NULL)
     return false;

  if (Recording->FileName() == NULL)
     return false;

  if ((_status == 1) && !Recording->IsNew())
     return false;

  if ((_status == 2) && !Recording->IsEdited())
     return false;

  if (isempty(_term)) {
     if (_status != 0)
        return true;
     return false;
     }

  const cRecordingInfo *info = Recording->Info();
  if (info == NULL)
     return false;

  const char *text = info->Title();
  if ((text != NULL) && (strcasestr(text, _term) != NULL))
     return true;

  text = info->ShortText();
  if ((text != NULL) && (strcasestr(text, _term) != NULL))
     return true;

  text = info->Description();
  if ((text != NULL) && (strcasestr(text, _term) != NULL))
     return true;

  return false;
}

void recsearch::cSearchParameter::Clear(void)
{
  memset(_term, 0, RECSEARCH_TERM_MAX_LEN);
  _status = 0;
}

bool recsearch::cSearchParameter::IsValid(void) const
{
  if (_status < 0)
     return false;
  if ((_status > 0) && (_status < 3))
     return true;
  compactspace(_term);
  return !isempty(_term);
}

bool recsearch::cSearchParameter::Parse(const char *s)
{
  const char *value;
  cParameterHelper helper(s);

  Clear();

  value = helper.Get("term");
  if (value != NULL)
     strncpy(_term, value, RECSEARCH_TERM_MAX_LEN - 1);

  value = helper.Get("status");
  if ((value != NULL) && isnumber(value))
     _status = atoi(value);

  return IsValid();
}

bool recsearch::cSearchParameter::Save(FILE *f)
{
  if (!IsValid())
     return false;
  return fprintf(f, "%s\n", *ToString()) > 0;
}

cString recsearch::cSearchParameter::ToString(void) const
{
  cString esc_term("");
  if (!isempty(_term))
     esc_term = strescape(_term, "\\,");
  return cString::sprintf("term=%s,status=%d",
                          *esc_term,
                          _status);
}

cString recsearch::cSearchParameter::ToText(void) const
{
  if (!IsValid())
     return tr("invalid");
  return cString::sprintf("%s=%s, %s=%s",
                       tr("search term"), _term,
                       tr("status"), _status_text[_status]);
}


recsearch::cSearches recsearch::cSearches::Searches;

recsearch::cSearchParameter *recsearch::cSearches::Contains(const cSearchParameter &Parameter) const
{
  for (cSearchParameter *p = First(); p; p = Next(p)) {
      if (p->Compare(Parameter) == 0)
         return p;
      }
  return NULL;
}
