#include "search_parameter.h"


const char *recsearch::cSearchParameter::_status_text[3] = {
                                                            tr("all"),
                                                            tr("only new"),
                                                            tr("only edited")
                                                           };

recsearch::cSearchParameter::cSearchParameter(void)
{
  memset(_term, 0, RECSEARCH_TERM_MAX_LEN);
  _status = 0;
}

recsearch::cSearchParameter::~cSearchParameter(void)
{
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

bool recsearch::cSearchParameter::IsValid()
{
  compactspace(_term);
  return (_status != 0) || !isempty(_term);
}

bool recsearch::cSearchParameter::Parse(const char *s)
{
  return true;
}

cString recsearch::cSearchParameter::ToString(void)
{
  cString s("");
  return s;
}
