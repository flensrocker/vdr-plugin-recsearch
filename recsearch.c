/*
 * recsearch.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "search_menu.h"

#include <vdr/plugin.h>

static const char *VERSION        = "0.0.7";
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
  return new recsearch::cSearchMenu();
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

bool cPluginRecsearch::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginRecsearch::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginRecsearch::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginRecsearch); // Don't touch this!
