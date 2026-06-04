/*****************************************************************/
/*    FILE: PointAssign_Info.cpp                                 */
/*****************************************************************/

#include <cstdlib>
#include <iostream>
#include "PointAssign_Info.h"
#include "ColorParse.h"
#include "ReleaseInfo.h"

using namespace std;

void showSynopsis()
{
  blk("SYNOPSIS:");
  blk("------------------------------------");
  blk("  pPointAssign receives VISIT_POINT messages on the shoreside,");
  blk("  assigns them to vehicles, and republishes them as");
  blk("  VISIT_POINT_<VNAME>. It can assign points either by alternating");
  blk("  vehicle or by east/west region.");
}

void showHelpAndExit()
{
  blk("Usage: pPointAssign file.moos [OPTIONS]");
  blk("");
  showSynopsis();
  blk("");
  blk("Options:");
  mag("  --alias", "=<ProcessName>");
  blk("      Launch pPointAssign with the given process name.");
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
  blk("ProcessConfig = pPointAssign");
  blk("{");
  blk("  AppTick        = 4");
  blk("  CommsTick      = 4");
  blk("");
  blk("  vname          = henry");
  blk("  vname          = gilda");
  blk("");
  blk("  assign_by_region = true");
  blk("  wait_for_ready   = true");
  blk("  unpause_timer    = true");
  blk("");
  blk("  region_xmin = -25");
  blk("  region_xmax = 200");
  blk("  region_ymin = -175");
  blk("  region_ymax = -25");
  blk("}");
  exit(0);
}

void showInterfaceAndExit()
{
  blk("SUBSCRIPTIONS:");
  blk("------------------------------------");
  blk("  VISIT_POINT");
  blk("  GENPATH_READY_<VNAME>");
  blk("");
  blk("PUBLICATIONS:");
  blk("------------------------------------");
  blk("  VISIT_POINT_<VNAME>");
  blk("  VIEW_POINT");
  blk("  UTS_PAUSE");
  exit(0);
}

void showReleaseInfoAndExit()
{
  showReleaseInfo("pPointAssign", "gpl");
  exit(0);
}
