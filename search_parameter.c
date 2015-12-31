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
  operator=(Parameter);
}

recsearch::cSearchParameter::~cSearchParameter(void)
{
}

recsearch::cSearchParameter &recsearch::cSearchParameter::operator=(const recsearch::cSearchParameter &Parameter)
{
  if (this == &Parameter)
     return *this;
  memcpy(_name, Parameter._name, RECSEARCH_MAX_LEN);
  memcpy(_category, Parameter._category, RECSEARCH_MAX_LEN);
  memcpy(_term, Parameter._term, RECSEARCH_MAX_LEN);
  _status = Parameter._status;
  _younger_than_days = Parameter._younger_than_days;
  _hot_key = Parameter._hot_key;
  SplitTerms();
  return *this;
}

void recsearch::cSearchParameter::SplitTerms(void)
{
  _splitted_terms.Clear();
  char *a = _term;
  char buffer[RECSEARCH_MAX_LEN];
  do {
    char *b = strchrnul(a, '|');
    if (b > a) {
       strncpy(buffer, a, b - a);
       buffer[b - a] = '\0';
       compactspace(buffer);
       if (!isempty(buffer))
          _splitted_terms.Append(strdup(buffer));
       }
    if (*b == '\0')
       break;
    a = b + 1;
    } while (true);
}

int recsearch::cSearchParameter::Compare(const cListObject &ListObject) const
{
  const cSearchParameter& rhs = (const cSearchParameter&)ListObject;
  int cmp = strcasecmp(_name, rhs._name);
  if (cmp != 0)
     return cmp;
  cmp = strcasecmp(_category, rhs._category);
  if (cmp != 0)
     return cmp;
  cmp = strcasecmp(_term, rhs._term);
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

bool recsearch::cSearchParameter::Filter(const cRecording *Recording) const
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

  if (_splitted_terms.Size() == 0) {
     if ((_status > 0) || (_younger_than_days > 0))
        return true;
     return false;
     }

  const cRecordingInfo *info = Recording->Info();
  if (info == NULL)
     return false;

  int found = 0;
  const char *text;
  const char *term;
  int look_into; // bit 1: title, bit 2: shorttext, bit 3: description
  for (int i = 0; i < _splitted_terms.Size(); i++) {
      term = _splitted_terms[i];
      look_into = 7;
      if ((strlen(term) > 2) && (term[1] == ':')) {
         if ((term[0] == 't') || (term[0] == 'T'))
            look_into = 1;
         else if ((term[0] == 's') || (term[0] == 'S'))
            look_into = 2;
         else if ((term[0] == 'd') || (term[0] == 'D'))
            look_into = 4;
         if (look_into != 7)
            term += 2;
         }

      if ((look_into & 1) != 0) {
         text = info->Title();
         if ((text != NULL) && (strcasestr(text, term) != NULL)) {
            found++;
            continue;
            }
         }

      if ((look_into & 2) != 0) {
         text = info->ShortText();
         if ((text != NULL) && (strcasestr(text, term) != NULL)) {
            found++;
            continue;
            }
         }

      if ((look_into & 4) != 0) {
         text = info->Description();
         if ((text != NULL) && (strcasestr(text, term) != NULL)) {
            found++;
            continue;
            }
         }

      return false;
      }

  return (found == _splitted_terms.Size());
}

void recsearch::cSearchParameter::Clear(void)
{
  memset(_name, 0, RECSEARCH_MAX_LEN);
  memset(_category, 0, RECSEARCH_MAX_LEN);
  memset(_term, 0, RECSEARCH_MAX_LEN);
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

  value = helper.Get("name");
  if (value != NULL)
     strncpy(_name, value, RECSEARCH_MAX_LEN - 1);

  value = helper.Get("category");
  if (value != NULL)
     strncpy(_category, value, RECSEARCH_MAX_LEN - 1);

  value = helper.Get("term");
  if (value != NULL)
     strncpy(_term, value, RECSEARCH_MAX_LEN - 1);

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
  cString esc_name("");
  if (!isempty(_name))
     esc_name = strescape(_name, "\\,");

  cString esc_category("");
  if (!isempty(_category))
     esc_category = strescape(_category, "\\,");

  cString esc_term("");
  if (!isempty(_term))
     esc_term = strescape(_term, "\\,");

  cString hot_key("");
  if (_hot_key > 0)
     hot_key = cString::sprintf(",hotkey=%d", _hot_key);

  return cString::sprintf("name=%s,category=%s,term=%s,status=%d,youngerthandays=%d%s",
                          *esc_name,
                          *esc_category,
                          *esc_term,
                          _status,
                          _younger_than_days,
                          *hot_key);
}

cString recsearch::cSearchParameter::ToText(void) const
{
  if (!IsValid())
     return tr("invalid");

  if (!isempty(_name))
     return cString(_name);

  cString term_status("");
  if (_term[0])
     term_status = cString::sprintf("%s, ", _term);
  term_status = cString::sprintf("%s%s=%s", *term_status, tr("status"), _status_text[_status]);

  cString younger("");
  if (_younger_than_days > 0) // TRANSLATORS: note the leading comma and the %d for the number of days
     younger = cString::sprintf(tr(", younger than %d days"), _younger_than_days);

  cString hotkey("");
  if (_hot_key > 0) // TRANSLATORS: note the leading comma and the %d for the number of the hot key
     hotkey = cString::sprintf(tr(", hot key %d"), _hot_key);

  return cString::sprintf("%s%s%s", *term_status, *younger, *hotkey);
}

void recsearch::cSearchParameter::SetCategory(const char *Category)
{
  memset(_category, 0, RECSEARCH_MAX_LEN);
  if (Category != NULL)
     strncpy(_category, Category, RECSEARCH_MAX_LEN - 1);
}

void recsearch::cSearchParameter::SetName(const char *Name)
{
  memset(_name, 0, RECSEARCH_MAX_LEN);
  if (Name != NULL)
     strncpy(_name, Name, RECSEARCH_MAX_LEN - 1);
}


recsearch::cSearches recsearch::cSearches::Last;
recsearch::cSearches recsearch::cSearches::Searches;
const char *recsearch::cSearches::CatDelim = "~";

const recsearch::cSearchParameter *recsearch::cSearches::Contains(const cSearchParameter &Parameter) const
{
  for (const cSearchParameter *p = First(); p; p = Next(p)) {
      if (p->Compare(Parameter) == 0)
         return p;
      }
  return NULL;
}

const recsearch::cSearchParameter *recsearch::cSearches::GetHotKey(int HotKey) const
{
  if (HotKey > 0) {
     for (const cSearchParameter *p = First(); p; p = Next(p)) {
         if (p->HotKey() == HotKey)
            return p;
         }
     }
  return NULL;
}

void recsearch::cSearches::GetCategories(cStringList &Categories) const
{
  Categories.Clear();
  for (const cSearchParameter *p = First(); p; p = Next(p)) {
      const char *c = p->Category();
      if (!isempty(c) && (Categories.Find(c) < 0))
         Categories.Append(strdup(c));
      }
  Categories.Sort();
}

static cNestedItem *find_nested_item(cList<cNestedItem> *List, const char *Text)
{
  for (cNestedItem *i = List->First(); i; i = List->Next(i)) {
      if (((Text == NULL) && (i->Text() == NULL))
       || ((Text != NULL) && (i->Text() != NULL) && (strcmp(Text, i->Text()) == 0)))
         return i;
      }
  return NULL;
}

void recsearch::cSearches::GetCatMenus(cList<cNestedItem> *CatMenus) const
{
  CatMenus->Clear();
  for (const cSearchParameter *p = First(); p; p = Next(p)) {
      char *c = strdup(p->Category());
      if (isempty(c))
         CatMenus->Add(new cNestedItem(*p->ToString()));
      else {
         cList<cNestedItem> *list = CatMenus;
         cNestedItem *item = NULL;
         char *strtok_next;
         for (char *t = strtok_r(c, CatDelim, &strtok_next); t; t = strtok_r(NULL, CatDelim, &strtok_next)) {
             item = find_nested_item(list, t);
             if (item == NULL) {
                item = new cNestedItem(t, true);
                cNestedItem *s = list->First();
                while (s && (s->SubItems() != NULL) && (strcasecmp(s->Text(), c) < 0))
                      s = list->Next(s);
                list->Ins(item, s);
                }
             list = item->SubItems();
             }
         if (item != NULL)
            item->AddSubItem(new cNestedItem(*p->ToString()));
         }
      free(c);
      }
}

bool recsearch::cSearches::LoadSearches(void)
{
  if ((*_filename == NULL) || isempty(*_filename))
     return false;
  return Load(*_filename, false, false);
}
