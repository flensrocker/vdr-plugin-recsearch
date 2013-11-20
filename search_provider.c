#include "search_provider.h"


void  recsearch::cSearchParameter::AddResult(cRecording *Recording)
{
  if ((Recording == NULL) || (Recording->FileName() == NULL))
     return;

  _mutex.Lock();
  _result.Add(new cRecording(Recording->FileName()));
  _mutex.Unlock();
}

void  recsearch::cSearchParameter::Done(void)
{
  isyslog("recsearch: search done");
  if (_host != NULL)
     _host->SearchDone();
}


cList<recsearch::cSearchProvider>  recsearch::cSearchProvider::_providers;
recsearch::cSearchParameter*       recsearch::cSearchProvider::_parameter;

void  recsearch::cSearchProvider::StartSearch(cSearchParameter *Parameter)
{
  _parameter = Parameter;
  _parameter->_mutex.Lock();
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp)) {
      isyslog("recsearch: starting %s", sp->Name());
      sp->Start();
      }
  _parameter->_mutex.Unlock();
}

void  recsearch::cSearchProvider::StopSearch(void)
{
  _parameter->_mutex.Lock();
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp)) {
      if (sp->Active()) {
         isyslog("recsearch: stopping %s", sp->Name());
         sp->Cancel(-1);
         }
      }
  _parameter->_mutex.Lock();
  cCondWait::SleepMs(1000);
  _parameter->_mutex.Unlock();
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp)) {
      if (sp->Active()) {
         isyslog("recsearch: cancelling %s", sp->Name());
         sp->Cancel(3);
         }
      }
  _parameter->_mutex.Unlock();
}

recsearch::cSearchProvider::cSearchProvider(const char *Name)
 : _name(Name)
{
  isyslog("recsearch: adding search-provider %s", this->Name());
  _providers.Add(this);
}

recsearch::cSearchProvider::~cSearchProvider(void)
{
  _providers.Del(this, false);
}

void recsearch::cSearchProvider::Action(void)
{
  _parameter->_mutex.Lock();
  _parameter->_count++;
  _parameter->_mutex.Unlock();

  isyslog("recsearch: started search-provider %s", Name());
  OnSearch(_parameter);
  isyslog("recsearch: search-provider %s ended", Name());

  _parameter->_mutex.Lock();
  _parameter->_count--;
  bool done = (_parameter->_count <= 0);
  _parameter->_mutex.Unlock();
  if (done)
     _parameter->Done();
}
