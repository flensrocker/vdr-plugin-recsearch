/*
 * recsearch.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "search_menu.h"

#include <vdr/plugin.h>

static const char *VERSION        = "0.3.2";
static const char *DESCRIPTION    = tr("search your recordings");
static const char *MAINMENUENTRY  = tr("search recordings");

class cPluginRecsearch : public cPlugin {
private:
  // Add any member variables or functions you may need here.
public:
  cPluginRecsearch(void);
  virtual ~cPluginRecsearch();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginRecsearch::cPluginRecsearch(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginRecsearch::~cPluginRecsearch()
{
  // Clean up after yourself!
}

const char *cPluginRecsearch::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginRecsearch::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginRecsearch::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  cString config_directory = cPlugin::ConfigDirectory("recsearch");
  recsearch::cSearches::Last.SetFilename(cString::sprintf("%s/last.conf", *config_directory));
  recsearch::cSearches::Searches.SetFilename(cString::sprintf("%s/searches.conf", *config_directory));
  return true;
}

bool cPluginRecsearch::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginRecsearch::Stop(void)
{
  // Stop any background activities the plugin is performing.
}

void cPluginRecsearch::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginRecsearch::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginRecsearch::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginRecsearch::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginRecsearch::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  recsearch::cSearches::Searches.LoadSearches();
  if (recsearch::cSearches::Searches.Count() == 0)
     return new recsearch::cSearchMenu();
  return new recsearch::cMainMenu();
}

cMenuSetupPage *cPluginRecsearch::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginRecsearch::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

static bool extract_tag(const char *text, const char *term, const char *term_prefix, cNestedItem *tag)
{
  if ((text == NULL) || (term == NULL) || (tag == NULL) || (tag->SubItems() == NULL))
     return false;

  const char *result = NULL;
  int t_len = 0;
  do {
       result = strcasestr(text, term);
       if (result == NULL)
          return false;

       if ((result > text) && (text[result - text - 1] != '\n')) {
          text = result + strlen(term);
          continue;
          }

       result += strlen(term);
       text = result;
       int r_len = strlen(result);
       if (r_len <= 0)
          return false;

       t_len = 0;
       while ((t_len < r_len) && (result[t_len] != '\n'))
             t_len++;

     } while (t_len <= 0);

  char *tag_text = new char[t_len + 1];
  memcpy(tag_text, result, t_len);
  tag_text[t_len] = 0;
  const char *name = skipspace(tag_text);
  bool found = false;
  for (cNestedItem *t = tag->SubItems()->First(); t; t = tag->SubItems()->Next(t)) {
      if ((t->Text() != NULL) && (t->SubItems() != NULL) && (strcmp(t->Text(), name) == 0)) {
         found = true;
         cString tag_term = cString::sprintf("%s%s%s", term_prefix, term, tag_text);
         bool item_found = false;
         for (cNestedItem *i = t->SubItems()->First(); i; i = t->SubItems()->Next(i)) {
             if ((i->Text() != NULL) && (strcmp(i->Text(), *tag_term) == 0)) {
                item_found = true;
                break;
                }
             }
         if (!item_found)
            t->AddSubItem(new cNestedItem(*tag_term));
         break;
         }
      }
  if (!found) {
     cNestedItem *item = new cNestedItem(name, true);
     item->AddSubItem(new cNestedItem(*cString::sprintf("%s%s%s", term_prefix, term, tag_text)));
     tag->AddSubItem(item);
     }
  delete [] tag_text;
  return true;
}

static void scan_tags(cNestedItemList &tags, const char *title, const char *shorttext, const char *description)
{
  const char *term;
  int look_into; // bit 1: title, bit 2: shorttext, bit 3: description
  for (cNestedItem *tag = tags.First(); tag; tag = tags.Next(tag)) {
      term = tag->Text();
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

      if ((look_into & 1) != 0)
         extract_tag(title, term, (look_into == 1) ? "t:" : "", tag);

      if ((look_into & 2) != 0)
         extract_tag(shorttext, term, (look_into == 2) ? "s:" : "", tag);

      if ((look_into & 4) != 0)
         extract_tag(description, term, (look_into == 4) ? "d:" : "", tag);
      }
}

static void scan_recordings(cNestedItemList &tags)
{
  cThreadLock RecordingsLock(&Recordings);
  const cRecordingInfo *info;
  for (cRecording *recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
      info = recording->Info();
      if (info == NULL)
         continue;

      scan_tags(tags, info->Title(), info->ShortText(), info->Description());
      }
}

static void scan_events(cNestedItemList &tags)
{
  cSchedulesLock lock;
  const cSchedules *ss = cSchedules::Schedules(lock);
  if (ss) {
     for (const cSchedule *s = ss->First(); s; s = ss->Next(s)) {
         const cList<cEvent> *es = s->Events();
         for (cEvent *e = es->First(); e; e = es->Next(e)) {
             scan_tags(tags, e->Title(), e->ShortText(), e->Description());
             }
         }
     }
}

bool cPluginRecsearch::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginRecsearch::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char *HelpPages[] = {
    "SCAN <tag>\n"
    "    All recording infos are scanned for the given tag.\n"
    "    The info files must contain lines with '<tag>: <value>'.\n"
    "    The colon is added to the tag, it can be prefixed with\n"
    "    't:', 's:', 'd:' to search only in title, short text or\n"
    "    description instead of all fields",
    "ESCN <tag>\n"
    "    Like SCAN only the events are scanned.",
    NULL
    };
  return HelpPages;
}

cString cPluginRecsearch::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  if ((strcasecmp(Command, "SCAN") == 0) || (strcasecmp(Command, "ESCN") == 0)) {
     if (Option && *Option) {
        cNestedItemList tags;

        char *strtok_next;
        char *o = strdup(Option);
        for (char *t = strtok_r(o, "|", &strtok_next); t; t = strtok_r(NULL, "|", &strtok_next))
            tags.Add(new cNestedItem(*cString::sprintf("%s:", t), true));
        free(o);

        isyslog("recsearch: start scanning");
        cTimeMs stopwatch;
        if (strcasecmp(Command, "ESCN") == 0)
           scan_events(tags);
        else
           scan_recordings(tags);
        uint64_t elapsed = stopwatch.Elapsed();
        isyslog("recsearch: finished scanning in %dms", elapsed);

        cString reply = "scan results:";
        for (cNestedItem *tag = tags.First(); tag; tag = tags.Next(tag)) {
            tag->SubItems()->Sort();
            const char *term = tag->Text();
            if ((strlen(term) > 2) && (term[1] == ':')
             && ((term[0] == 't') || (term[0] == 'T')
              || (term[0] == 's') || (term[0] == 'S')
              || (term[0] == 'd') || (term[0] == 'D')))
               term += 2;
            for (cNestedItem *name = tag->SubItems()->First(); name; name = tag->SubItems()->Next(name)) {
                name->SubItems()->Sort();
                for (cNestedItem *t = name->SubItems()->First(); t; t = name->SubItems()->Next(t)) {
                    reply = cString::sprintf( "%scategory=%s,name=%s,term=%s\n", *reply, *strescape(term, "\\,"), *strescape(name->Text(), "\\,"), *strescape(t->Text(), "\\,"));
                    }
                }
            }
        ReplyCode = 250;
        return reply;
        }
     ReplyCode = 501;
     return "missing tag";
     }
  return NULL;
}

VDRPLUGINCREATOR(cPluginRecsearch); // Don't touch this!
