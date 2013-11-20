#include "local_search.h"


void recsearch::cLocalSearch::OnSearch(cSearchParameter *Parameter)
{
  cRecordings recordings;
  const cRecordingInfo *info;
  const char *text;

  dsyslog("recsearch/local-search: loading recordings");
  recordings.Load();  
  dsyslog("recsearch/local-search: search for recordings");
  for (cRecording *r = recordings.First(); Running() && r; r = recordings.Next(r)) {
      if (r->FileName() == NULL)
         continue;

      info = r->Info();
      if (info == NULL)
         continue;

      if ((Parameter->_search_status == 1) && !r->IsNew())
         continue;

      if ((Parameter->_search_status == 2) && !r->IsEdited())
         continue;

      text = info->Title();
      if ((text != NULL) && (strcasestr(text, Parameter->_search_term) != NULL)) {
         Parameter->AddResult(r);
         continue;
         }

      text = info->ShortText();
      if ((text != NULL) && (strcasestr(text, Parameter->_search_term) != NULL)) {
         Parameter->AddResult(r);
         continue;
         }

      if (!Running())
         break;
      text = info->Description();
      if ((text != NULL) && (strcasestr(text, Parameter->_search_term) != NULL)) {
         Parameter->AddResult(r);
         continue;
         }
      }
  dsyslog("recsearch/local-search: done");
}
