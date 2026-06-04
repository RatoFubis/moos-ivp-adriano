/*****************************************************************/
/*    FILE: GenPath_Info.cpp                                     */
/*****************************************************************/

#include <cstdlib>
#include <iostream>
#include "ColorParse.h"
#include "ReleaseInfo.h"
#include "GenPath_Info.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:");
  blk("------------------------------------");
  blk("  pGenPath receives VISIT_POINT messages on a vehicle,");
  blk("  builds a greedy waypoint tour, and publishes dynamic");
  blk("  waypoint behavior updates to a configured MOOS variable.");
}

void showHelpAndExit()
{
  blk("Usage: pGenPath file.moos [OPTIONS]");
  blk("");
  showSynopsis();
  blk("");
  blk("Options:");
  mag("  --alias", "=<ProcessName>");
  blk("      Launch pGenPath with the given process name.");
  mag("  --example, -e");
  blk("      Display example MOOS configuration block.");
  mag("  --interface, -i");
  blk("      Display MOOS publications and subscriptions.");
  mag("  --help, -h");
  blk("      Display this help message.");
  mag("  --version, -v");
  blk("      Display release version.");
  exit(0);
}

void showExampleConfigAndExit()
{
  blk("ProcessConfig = pGenPath");
  blk("{");
  blk("  AppTick      = 4");
  blk("  CommsTick    = 4");
  blk("");
  blk("  vname        = $(VNAME)");
  blk("  updates_var  = WPT_UPDATE");
  blk("  visit_radius = 3");
  blk("}");
  exit(0);
}

void showInterfaceAndExit()
{
  blk("SUBSCRIPTIONS:");
  blk("------------------------------------");
  blk("  VISIT_POINT");
  blk("  NAV_X");
  blk("  NAV_Y");
  blk("");
  blk("PUBLICATIONS:");
  blk("------------------------------------");
  blk("  GENPATH_READY_<VNAME>");
  blk("  configured updates_var, e.g., WPT_UPDATE");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("pGenPath", "gpl");
  exit(0);
}
