#include "local_search.h"


void recsearch::cLocalSearch::OnSearch(cSearchParameter *Parameter)
{
  cRecording *rec;
  const cRecordingInfo *info;
  const char *text;
  
  //Recordings.Lock();
  for (cRecording *r = Recordings.First(); Running() && r; r = Recordings.Next(r)) {
      if (r->FileName() == NULL)
         continue;

      rec = new cRecording(r->FileName());
      info = rec->Info();
      if (info == NULL)
         continue;

      for (int i = 0; Running() && (i < Parameter->_search_terms.Size()); i++) {
          text = info->Title();
          if ((text != NULL) && (strcasestr(text, Parameter->_search_terms[i]) != NULL)) {
             Parameter->AddResult(rec);
             continue;
             }

          text = info->ShortText();
          if ((text != NULL) && (strcasestr(text, Parameter->_search_terms[i]) != NULL)) {
             Parameter->AddResult(rec);
             continue;
             }

          text = info->Description();
          if ((text != NULL) && (strcasestr(text, Parameter->_search_terms[i]) != NULL)) {
             Parameter->AddResult(rec);
             continue;
             }
          }
      delete rec;
      }
  //Recordings.Unlock();
}
