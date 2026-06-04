/*****************************************************************/
/*    FILE: PointAssign.cpp                                      */
/*****************************************************************/

#include <iterator>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cctype>

#include "MBUtils.h"
#include "XYPoint.h"
#include "PointAssign.h"

using namespace std;

//---------------------------------------------------------
// Constructor

PointAssign::PointAssign()
{
  m_assign_by_region = false;
  m_wait_for_ready   = false;
  m_unpause_timer    = false;

  // Default lab region corners.
  m_region_xmin = -25;
  m_region_xmax = 200;
  m_region_ymin = -175;
  m_region_ymax = -25;

  m_received_first = false;
  m_received_last  = false;
  m_distributed    = false;

  m_point_count     = 0;
  m_invalid_points  = 0;
  m_total_received  = 0;
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool PointAssign::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    string key  = msg.GetKey();
    string sval = msg.GetString();

    if(key == "VISIT_POINT") {
      handleVisitPoint(sval);
      continue;
    }

    string prefix = "GENPATH_READY_";
    if(key.length() > prefix.length() &&
       key.substr(0, prefix.length()) == prefix) {

      string ready = tolower(stripBlankEnds(sval));
      if(ready == "true" || ready == "1") {
        string vname = tolower(key.substr(prefix.length()));
        m_ready_vnames.insert(vname);
      }
    }
  }

  return true;
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool PointAssign::OnConnectToServer()
{
  registerVariables();
  return true;
}

//---------------------------------------------------------
// Procedure: Iterate()

bool PointAssign::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // Keep uTimerScript unpaused until the first VISIT_POINT marker is seen.
  // This makes the startup order robust when pPointAssign and uTimerScript
  // connect to the MOOSDB at slightly different times.
  if(m_unpause_timer && !m_received_first)
    Notify("UTS_PAUSE", "false");

  if(!m_distributed && m_received_first && m_received_last) {
    bool ready_to_send = true;

    if(m_wait_for_ready) {
      for(unsigned int i = 0; i < m_vnames.size(); i++) {
        if(m_ready_vnames.count(m_vnames[i]) == 0)
          ready_to_send = false;
      }
    }

    if(ready_to_send)
      distributePoints();
  }

  AppCastingMOOSApp::PostReport();
  return true;
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool PointAssign::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p = sParams.begin(); p != sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;

    if(param == "vname") {
      string vname = tolower(stripBlankEnds(value));
      if(vname != "") {
        m_vnames.push_back(vname);
        handled = true;
      }
    }
    else if(param == "vnames") {
      string names = stripBlankEnds(value);
      // launch_shoreside passes names as henry:gilda. Also accept commas/spaces.
      for(unsigned int i=0; i<names.length(); i++) {
        if(names[i] == ':' || names[i] == ',')
          names[i] = ' ';
      }
      stringstream ss(names);
      string name;
      while(ss >> name) {
        name = tolower(stripBlankEnds(name));
        if(name != "")
          m_vnames.push_back(name);
      }
      handled = true;
    }
    else if(param == "assign_by_region") {
      handled = setBooleanOnString(m_assign_by_region, value);
    }
    else if(param == "wait_for_ready") {
      handled = setBooleanOnString(m_wait_for_ready, value);
    }
    else if(param == "unpause_timer") {
      handled = setBooleanOnString(m_unpause_timer, value);
    }
    else if(param == "region_xmin") {
      handled = setDoubleOnString(m_region_xmin, value);
    }
    else if(param == "region_xmax") {
      handled = setDoubleOnString(m_region_xmax, value);
    }
    else if(param == "region_ymin") {
      handled = setDoubleOnString(m_region_ymin, value);
    }
    else if(param == "region_ymax") {
      handled = setDoubleOnString(m_region_ymax, value);
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  if(m_vnames.size() == 0)
    reportConfigWarning("No vehicles configured. Add lines like: vname = henry");

  if(m_vnames.size() < 2)
    reportConfigWarning("This lab expects at least two vehicles.");

  registerVariables();

  if(m_unpause_timer) {
    // uTimerScript is configured with pause_var=POINT_ASSIGN_READY=true.
    // UTS_PAUSE=false is also posted for compatibility with timer-script controls.
    Notify("POINT_ASSIGN_READY", "true");
    Notify("UTS_PAUSE", "false");
  }

  return true;
}

//---------------------------------------------------------
// Procedure: registerVariables

void PointAssign::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("VISIT_POINT", 0);

  for(unsigned int i = 0; i < m_vnames.size(); i++) {
    // Vehicle brokers in this mission bridge lowercase names, while some
    // pGenPath versions publish uppercase names. Listen for both.
    string ready_var_lower = "GENPATH_READY_" + m_vnames[i];
    string ready_var_upper = "GENPATH_READY_" + toupperStr(m_vnames[i]);
    Register(ready_var_lower, 0);
    Register(ready_var_upper, 0);
  }
}

//---------------------------------------------------------
// Procedure: handleVisitPoint

bool PointAssign::handleVisitPoint(const string& sval)
{
  string clean = tolower(stripBlankEnds(sval));

  if(clean == "firstpoint") {
    m_points.clear();
    m_received_first = true;
    m_received_last = false;
    m_distributed = false;
    m_invalid_points = 0;
    m_total_received = 0;
    return true;
  }

  if(clean == "lastpoint") {
    m_received_last = true;
    return true;
  }

  if(!m_received_first) {
    reportRunWarning("Ignoring VISIT_POINT before firstpoint: " + sval);
    return false;
  }

  VisitPoint point;
  bool ok = parseVisitPoint(sval, point);

  m_total_received++;

  if(!ok) {
    m_invalid_points++;
    reportRunWarning("Invalid VISIT_POINT: " + sval);
    return false;
  }

  m_points.push_back(point);
  return true;
}

//---------------------------------------------------------
// Procedure: parseVisitPoint
// Expected forms include:
//   x=8, y=9, id=1
//   x=-11,y=-9,id=100

bool PointAssign::parseVisitPoint(const string& sval, VisitPoint& point) const
{
  string xstr  = tokStringParse(sval, "x",  ',', '=');
  string ystr  = tokStringParse(sval, "y",  ',', '=');
  string idstr = tokStringParse(sval, "id", ',', '=');

  if(xstr == "" || ystr == "")
    return false;

  if(!isNumber(xstr) || !isNumber(ystr))
    return false;

  // Some uTimerScript versions do not expand $[TCOUNT]. If the id is
  // missing or still contains a macro, replace it with a unique local count
  // so VIEW_POINT labels do not overwrite each other in pMarineViewer.
  idstr = stripBlankEnds(idstr);
  if(idstr == "" || idstr.find("$[") != string::npos)
    idstr = uintToString(m_total_received);

  point.x = atof(xstr.c_str());
  point.y = atof(ystr.c_str());
  point.id = idstr;
  point.raw = sval;
  point.valid = true;

  return true;
}

//---------------------------------------------------------
// Procedure: distributePoints

void PointAssign::distributePoints()
{
  if(m_vnames.size() == 0) {
    reportRunWarning("Cannot distribute points: no vehicles configured.");
    return;
  }

  if(m_assign_by_region)
    distributeByRegion();
  else
    distributeAlternating();

  m_distributed = true;
}

//---------------------------------------------------------
// Procedure: distributeAlternating

void PointAssign::distributeAlternating()
{
  for(unsigned int i = 0; i < m_vnames.size(); i++)
    postVehiclePoint(m_vnames[i], "firstpoint");

  for(unsigned int i = 0; i < m_points.size(); i++) {
    unsigned int vix = i % m_vnames.size();
    string vname = m_vnames[vix];

    postVehiclePoint(vname, m_points[i].raw);

    string label = vname + "_" + m_points[i].id;
    postViewPoint(m_points[i].x,
                  m_points[i].y,
                  label,
                  colorForVehicle(vix));
  }

  for(unsigned int i = 0; i < m_vnames.size(); i++)
    postVehiclePoint(m_vnames[i], "lastpoint");
}

//---------------------------------------------------------
// Procedure: distributeByRegion
//
// The lab region corners are:
//   -25,-25
//   -25,-175
//   200,-25
//   200,-175
//
// East/west split is based on midpoint x.
// With two vehicles, vehicle 0 gets west, vehicle 1 gets east.

void PointAssign::distributeByRegion()
{
  for(unsigned int i = 0; i < m_vnames.size(); i++)
    postVehiclePoint(m_vnames[i], "firstpoint");

  double mid_x = (m_region_xmin + m_region_xmax) / 2.0;

  for(unsigned int i = 0; i < m_points.size(); i++) {
    unsigned int vix = 0;

    if(m_vnames.size() == 1) {
      vix = 0;
    }
    else if(m_vnames.size() == 2) {
      if(m_points[i].x < mid_x)
        vix = 0;
      else
        vix = 1;
    }
    else {
      double width = m_region_xmax - m_region_xmin;
      if(width <= 0)
        vix = 0;
      else {
        double frac = (m_points[i].x - m_region_xmin) / width;
        if(frac < 0)
          frac = 0;
        if(frac > 1)
          frac = 1;

        vix = (unsigned int)floor(frac * m_vnames.size());
        if(vix >= m_vnames.size())
          vix = m_vnames.size() - 1;
      }
    }

    string vname = m_vnames[vix];

    postVehiclePoint(vname, m_points[i].raw);

    string label = vname + "_" + m_points[i].id;
    postViewPoint(m_points[i].x,
                  m_points[i].y,
                  label,
                  colorForVehicle(vix));
  }

  for(unsigned int i = 0; i < m_vnames.size(); i++)
    postVehiclePoint(m_vnames[i], "lastpoint");
}

//---------------------------------------------------------
// Procedure: postVehiclePoint

void PointAssign::postVehiclePoint(const string& vname,
                                   const string& point_spec)
{
  string var = "VISIT_POINT_" + toupperStr(vname);
  Notify(var, point_spec);
}

//---------------------------------------------------------
// Procedure: postViewPoint

void PointAssign::postViewPoint(double x,
                                double y,
                                const string& label,
                                const string& color)
{
  XYPoint point(x, y);
  point.set_label(label);
  point.set_color("vertex", color);
  point.set_color("label", color);
  point.set_param("vertex_size", "6");
  point.set_param("active", "true");

  string spec = point.get_spec();
  Notify("VIEW_POINT", spec);
}

//---------------------------------------------------------
// Procedure: toupperStr

string PointAssign::toupperStr(string s) const
{
  for(unsigned int i = 0; i < s.length(); i++)
    s[i] = toupper(s[i]);
  return s;
}

//---------------------------------------------------------
// Procedure: colorForVehicle

string PointAssign::colorForVehicle(unsigned int ix) const
{
  vector<string> colors;
  colors.push_back("yellow");
  colors.push_back("cyan");
  colors.push_back("orange");
  colors.push_back("magenta");
  colors.push_back("green");
  colors.push_back("white");

  return colors[ix % colors.size()];
}

//------------------------------------------------------------
// Procedure: buildReport()

bool PointAssign::buildReport()
{
  m_msgs << "pPointAssign Status" << endl;
  m_msgs << "-------------------" << endl;
  m_msgs << "Vehicles configured: " << m_vnames.size() << endl;

  for(unsigned int i = 0; i < m_vnames.size(); i++) {
    m_msgs << "  " << i << ": " << m_vnames[i];

    if(m_ready_vnames.count(m_vnames[i]))
      m_msgs << " ready";

    m_msgs << endl;
  }

  m_msgs << endl;
  m_msgs << "Assign by region: " << boolToString(m_assign_by_region) << endl;
  m_msgs << "Wait for ready:   " << boolToString(m_wait_for_ready) << endl;
  m_msgs << "Received first:   " << boolToString(m_received_first) << endl;
  m_msgs << "Received last:    " << boolToString(m_received_last) << endl;
  m_msgs << "Distributed:      " << boolToString(m_distributed) << endl;
  m_msgs << endl;
  m_msgs << "Total valid points:    " << m_points.size() << endl;
  m_msgs << "Total received points: " << m_total_received << endl;
  m_msgs << "Invalid points:        " << m_invalid_points << endl;

  return true;
}
