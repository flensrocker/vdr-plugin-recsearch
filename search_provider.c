#include "search_provider.h"


void  recsearch::cSearchParameter::AddResult(cRecording *Recording)
{
  if (Recording == NULL)
     return;

  _mutex.Lock();
  _result.Add(Recording);
  isyslog("recsearch: found \"%s\"", Recording->Info()->Title());
  _mutex.Unlock();
}

void  recsearch::cSearchParameter::Done(void)
{
  if (_host != NULL)
     _host->SearchDone();
}


cList<recsearch::cSearchProvider>  recsearch::cSearchProvider::_providers;
recsearch::cSearchParameter*       recsearch::cSearchProvider::_parameter;

void  recsearch::cSearchProvider::StartSearch(cSearchParameter *Parameter)
{
  _parameter = Parameter;
  _parameter->_mutex.Lock();
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp))
      sp->Start();
  _parameter->_mutex.Unlock();
}

void  recsearch::cSearchProvider::StopSearch(void)
{
  _parameter->_mutex.Lock();
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp))
      sp->Cancel(-1);
  for (cSearchProvider *sp = _providers.First(); sp; sp = _providers.Next(sp))
      sp->Cancel(3);
  _parameter->_mutex.Unlock();
}

recsearch::cSearchProvider::cSearchProvider(void)
{
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

  OnSearch(_parameter);

  _parameter->_mutex.Lock();
  _parameter->_count--;
  bool done = (_parameter->_count <= 0);
  _parameter->_mutex.Unlock();
  if (done)
     _parameter->Done();
}
