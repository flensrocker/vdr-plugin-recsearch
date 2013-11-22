#include "search_parameter.h"


bool recsearch::cSearchParameter::Filter(const cRecording *Recording)
{
  if (Recording == NULL)
     return false;

  if (Recording->FileName() == NULL)
     return false;

  if ((_search_status == 1) && !Recording->IsNew())
     return false;

  if ((_search_status == 2) && !Recording->IsEdited())
     return false;

  if (isempty(_search_term)) {
     if (_search_status != 0)
        return true;
     return false;
     }

  const cRecordingInfo *info = Recording->Info();
  if (info == NULL)
     return false;

  const char *text = info->Title();
  if ((text != NULL) && (strcasestr(text, _search_term) != NULL))
     return true;

  text = info->ShortText();
  if ((text != NULL) && (strcasestr(text, _search_term) != NULL))
     return true;

  text = info->Description();
  if ((text != NULL) && (strcasestr(text, _search_term) != NULL))
     return true;

  return false;
}
