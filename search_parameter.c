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
  _younger_than_days = Parameter._younger_than_days;
  _hot_key = Parameter._hot_key;
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
  if (_status != rhs._status)
     return _status - rhs._status;
  if (_younger_than_days != rhs._younger_than_days)
     return _younger_than_days - rhs._younger_than_days;
  if (_hot_key != rhs._hot_key)
     return _hot_key - rhs._hot_key;
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

  if (_younger_than_days > 0) {
     // trim to 0:00 of today before subtraction (1 day == 86400 sec)
     time_t limit = (time(NULL) / 86400 - _younger_than_days) * 86400;
     time_t start = Recording->Start();
     if (start < limit)
        return false;
     }

  if (isempty(_term)) {
     if ((_status > 0) || (_younger_than_days > 0))
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
  _younger_than_days = 0;
  _hot_key = 0;
}

bool recsearch::cSearchParameter::IsValid(void) const
{
  if ((_hot_key < 0) || (_hot_key > 9))
     return false;
  if ((_status < 0) || (_younger_than_days < 0))
     return false;
  if (((_status > 0) && (_status < 3)) || (_younger_than_days > 0))
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

  value = helper.Get("youngerthandays");
  if ((value != NULL) && isnumber(value))
     _younger_than_days = atoi(value);

  value = helper.Get("hotkey");
  if ((value != NULL) && isnumber(value))
     _hot_key = atoi(value);

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
  cString hot_key("");
  if (_hot_key > 0)
     hot_key = cString::sprintf(",hotkey=%d", _hot_key);
  return cString::sprintf("term=%s,status=%d,youngerthandays=%d%s",
                          *esc_term,
                          _status,
                          _younger_than_days,
                          *hot_key);
}

cString recsearch::cSearchParameter::ToText(void) const
{
  if (!IsValid())
     return tr("invalid");
  cString younger("");
  if (_younger_than_days > 0) // TRANSLATORS: note the leading comma and the %d for the number of days
     younger = cString::sprintf(tr(", younger than %d days"), _younger_than_days);
  cString hotkey("");
  if (_hot_key > 0) // TRANSLATORS: note the leading comma and the %d for the number of the hot key
     hotkey = cString::sprintf(tr(", hot key %d"), _hot_key);
  return cString::sprintf("%s=%s, %s=%s%s%s",
                       tr("search term"), _term,
                       tr("status"), _status_text[_status],
                       *younger, *hotkey);
}


recsearch::cSearches recsearch::cSearches::Last;
recsearch::cSearches recsearch::cSearches::Searches;

recsearch::cSearchParameter *recsearch::cSearches::Contains(const cSearchParameter &Parameter) const
{
  for (cSearchParameter *p = First(); p; p = Next(p)) {
      if (p->Compare(Parameter) == 0)
         return p;
      }
  return NULL;
}

recsearch::cSearchParameter *recsearch::cSearches::GetHotKey(int HotKey) const
{
  if (HotKey > 0) {
     for (cSearchParameter *p = First(); p; p = Next(p)) {
         if (p->HotKey() == HotKey)
            return p;
         }
     }
  return NULL;
}

bool recsearch::cSearches::LoadSearches(void)
{
  if ((*_filename == NULL) || isempty(*_filename))
     return false;
  return Load(*_filename, false, false);
}
