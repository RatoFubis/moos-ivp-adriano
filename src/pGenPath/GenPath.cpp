/*****************************************************************/
/*    FILE: GenPath.cpp                                          */
/*****************************************************************/

#include <iterator>
#include <sstream>
#include <cmath>
#include <limits>

#include "MBUtils.h"
#include "ACTable.h"

#include "XYSegList.h"

#include "GenPath.h"

using namespace std;

//---------------------------------------------------------
// Constructor

GenPath::GenPath()
{
  // State initialization.
  m_firstpoint_received = false;
  m_lastpoint_received  = false;
  m_nav_received        = false;
  m_nav_x_received      = false;
  m_nav_y_received      = false;
  m_path_generated      = false;

  // Default vehicle position.
  m_nav_x = 0;
  m_nav_y = 0;

  // Counters for debugging and AppCasting
  m_invalid_points  = 0;
  m_points_received = 0;
  m_points_in_path  = 0;

  // Update the Waypoint behavior.
  m_updates_var = "WPT_UPDATE";
  m_vname = "";
  m_ready_posted = false;

  m_path_posts = 0;
  m_last_path_post_time = 0;
  m_surveying = false;
  m_deployed = false;
  m_manual_override = true;
  m_path_accepted = false;
  m_path_update_burst = 0;
  m_returning = false;
  m_station_keep = false;
  m_tsp_done = false;
}

//---------------------------------------------------------
// Destructor

GenPath::~GenPath()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool GenPath::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  MOOSMSG_LIST::iterator p;
  for(p = NewMail.begin(); p != NewMail.end(); p++) {
    CMOOSMsg &msg = *p;

    string key = msg.GetKey();

    if(key == "VISIT_POINT") {
      handleVisitPoint(msg.GetString());
    }

    else if(key == "NAV_X") {
      m_nav_x = msg.GetDouble();
      m_nav_x_received = true;
      m_nav_received = m_nav_x_received && m_nav_y_received;
    }

    else if(key == "NAV_Y") {
      m_nav_y = msg.GetDouble();
      m_nav_y_received = true;
      m_nav_received = m_nav_x_received && m_nav_y_received;
    }

    // The waypoint behavior is not guaranteed to be active when the path is
    // first generated. In this mission the points often arrive while the helm
    // is still PARKed by MOOS_MANUAL_OVERRIDE. Re-post the cached update when
    // deployment/surveying begins, and whenever the helm reports NothingToDo.
    else if(key == "MODE") {
      bool now_surveying = isSurveyingMode(msg.GetString());
      if(now_surveying && !m_surveying)
        m_path_update_burst = 20;
      m_surveying = now_surveying;
      if(m_path_generated && now_surveying)
        postPathUpdate();
    }

    else if(key == "DEPLOY") {
      string sval = tolower(stripBlankEnds(msg.GetString()));
      bool was_deployed = m_deployed;
      m_deployed = (sval == "true" || sval == "1");

      // A redeploy after RETURN must restart the completed/previously accepted
      // TSP waypoint behavior. Otherwise waypt_tsp may remain completed or
      // empty while pGenPath incorrectly believes the old update is accepted.
      if(m_path_generated && m_deployed) {
        if(!was_deployed || m_returning || m_station_keep || m_tsp_done || m_path_accepted) {
          m_path_accepted = false;
          m_returning = false;
          m_station_keep = false;
          m_tsp_done = false;
          Notify("RETURN", "false");
          Notify("STATION_KEEP", "false");
          Notify("TSP_DONE", "false");
        }
        m_path_update_burst = 40;
        postPathUpdate();
      }
    }

    else if(key == "RETURN") {
      string sval = tolower(stripBlankEnds(msg.GetString()));
      bool was_returning = m_returning;
      m_returning = (sval == "true" || sval == "1");

      // When return is cancelled by a new deploy, resend the path as if this
      // were a fresh mission leg.
      if(was_returning && !m_returning && m_path_generated && m_deployed) {
        m_path_accepted = false;
        m_tsp_done = false;
        Notify("TSP_DONE", "false");
        m_path_update_burst = 40;
        postPathUpdate();
      }
    }

    else if(key == "STATION_KEEP") {
      string sval = tolower(stripBlankEnds(msg.GetString()));
      m_station_keep = (sval == "true" || sval == "1");
      if(m_station_keep)
        m_path_accepted = false;
    }

    else if(key == "TSP_DONE") {
      string sval = tolower(stripBlankEnds(msg.GetString()));
      m_tsp_done = (sval == "true" || sval == "1");
      if(m_tsp_done)
        m_path_accepted = false;
    }

    else if(key == "MOOS_MANUAL_OVERRIDE") {
      string sval = tolower(stripBlankEnds(msg.GetString()));
      m_manual_override = (sval == "true" || sval == "1");
      if(m_path_generated && !m_manual_override) {
        m_path_update_burst = 20;
        postPathUpdate();
      }
    }

    else if(key == "IVPHELM_ALLSTOP") {
      string sval = tolower(msg.GetString());
      if(m_path_generated && sval.find("nothingtodo") != string::npos) {
        m_path_update_burst = 20;
        postPathUpdate();
      }
    }

    else if(key == "BHV_WARNING" || key == "HELM_WARNING") {
      string sval = tolower(msg.GetString());
      if(m_path_generated && sval.find("no waypts") != string::npos) {
        m_path_update_burst = 20;
        postPathUpdate();
      }
    }

    else if(key == "DESIRED_SPEED") {
      // Only a non-zero speed while the TSP behavior is active should count
      // as acceptance of the dynamic path. Return/refuel/station-keeping also
      // produce speed commands and must not suppress later redeploy updates.
      if(m_surveying && !m_returning && !m_station_keep && !m_tsp_done &&
         msg.GetDouble() > 0.05)
        m_path_accepted = true;
    }

    else if(key != "APPCAST_REQ") {
      reportRunWarning("Unhandled Mail: " + key);
    }
  }

  return true;
}

//---------------------------------------------------------
// Procedure: handleVisitPoint()

bool GenPath::handleVisitPoint(const string& sval)
{
  // Beginning of points
  if(sval == "firstpoint") {
    m_points.clear();

    m_firstpoint_received = true;
    m_lastpoint_received  = false;
    m_path_generated      = false;
    m_path_accepted       = false;
    m_path_update         = "";
    m_path_update_burst   = 0;

    m_invalid_points  = 0;
    m_points_received = 0;
    m_points_in_path  = 0;

    return(true);
  }

  // End of points, path can generate
  if(sval == "lastpoint") {
    m_lastpoint_received = true;
    return(generatePath());
  }

  if(!m_firstpoint_received) {
    reportRunWarning("Received VISIT_POINT before firstpoint.");
    return(false);
  }

  GenPathPoint point;

  if(!parseVisitPoint(sval, point)) {
    m_invalid_points++;
    reportRunWarning("Invalid VISIT_POINT received: " + sval);
    return(false);
  }

  m_points.push_back(point);
  m_points_received++;

  return(true);
}

//---------------------------------------------------------
// Procedure: parseVisitPoint()

bool GenPath::parseVisitPoint(const string& sval, GenPathPoint& point) const
{
  // Extract x, y, and id
  string x_str;
  string y_str;
  string id_str;

  if(!getField(sval, "x", x_str))
    return(false);

  if(!getField(sval, "y", y_str))
    return(false);

  if(!getField(sval, "id", id_str))
    id_str = uintToString(m_points_received + 1);

  stringstream x_stream(x_str);
  stringstream y_stream(y_str);

  x_stream >> point.x;
  y_stream >> point.y;

  if(x_stream.fail() || y_stream.fail())
    return(false);

  point.id = id_str;
  return(true);
}

//---------------------------------------------------------
// Procedure: getField()

bool GenPath::getField(const string& input,
                       const string& field,
                       string& value) const
{
  // Find relevant fields in string ("x=", "y=", or "id=")
  string pattern = field + "=";

  size_t start = input.find(pattern);

  if(start == string::npos)
    return(false);

  start += pattern.size();

  size_t end = input.find(',', start);

  if(end == string::npos)
    value = input.substr(start);
  else
    value = input.substr(start, end - start);

  value = stripBlankEnds(value);

  return(value != "");
}

//---------------------------------------------------------
// Procedure: generatePath()

bool GenPath::generatePath()
{

  if(m_points.empty()) {
    reportRunWarning("No visit points received. Cannot generate path.");
    return(false);
  }

  if(!m_nav_received)
    reportRunWarning("NAV_X/NAV_Y not received yet. Using 0,0 as start point.");

  vector<GenPathPoint> remaining = m_points;
  XYSegList seglist;

  double current_x = m_nav_x;
  double current_y = m_nav_y;

  // Path generation.
  while(!remaining.empty()) {
    double best_dist = numeric_limits<double>::max();
    unsigned int best_ix = 0;

    for(unsigned int i = 0; i < remaining.size(); i++) {
      double dist = distance(current_x, current_y,
                             remaining[i].x, remaining[i].y);

      if(dist < best_dist) {
        best_dist = dist;
        best_ix = i;
      }
    }

    GenPathPoint next_point = remaining[best_ix];

    // Add the selected point to the waypoint segment list
    seglist.add_vertex(next_point.x, next_point.y);

    current_x = next_point.x;
    current_y = next_point.y;

    // Remove the selected point
    remaining.erase(remaining.begin() + best_ix);
  }

  // Cache and publish the waypoint update. The cache is important because
  // pHelmIvP may still be PARKed/manual-overridden when the path is first
  // generated, and BHV_Waypoint does not reliably retain a dynamic update
  // posted before it is active.
  m_path_update = "points = ";
  m_path_update += seglist.get_spec();

  m_points_in_path = m_points.size();
  m_path_generated = true;
  m_path_accepted = false;
  m_path_update_burst = 20;

  postPathUpdate();

  return(true);
}

//---------------------------------------------------------
// Procedure: postPathUpdate()

void GenPath::postPathUpdate()
{
  if(m_path_update == "")
    return;

  Notify(m_updates_var, m_path_update);
  m_path_posts++;
  m_last_path_post_time = MOOSTime();
}

//---------------------------------------------------------
// Procedure: isSurveyingMode()

bool GenPath::isSurveyingMode(const string& sval) const
{
  string mode = tolower(sval);
  return(mode.find("surveying") != string::npos);
}

//---------------------------------------------------------
// Procedure: shouldKeepPostingPath()

bool GenPath::shouldKeepPostingPath() const
{
  if(!m_path_generated || m_path_update == "" || m_path_accepted)
    return(false);

  // Do not fight the return or station-keep behaviors. Resume posting when a
  // deploy/RETURN=false transition asks for SURVEYING again.
  if(m_returning || m_station_keep || m_tsp_done)
    return(false);

  // Before deployment, a slow heartbeat is harmless and makes uPokeDB tests
  // and delayed helm startups robust. During/after deployment, keep posting
  // until pHelmIvP starts commanding non-zero speed while SURVEYING.
  return(true);
}

//---------------------------------------------------------
// Procedure: distance()

double GenPath::distance(double x1, double y1,
                         double x2, double y2) const
{
  // Calculate Euclidean distance between two points.
  double dx = x1 - x2;
  double dy = y1 - y2;

  return(sqrt((dx * dx) + (dy * dy)));
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool GenPath::OnConnectToServer()
{
   registerVariables();

   // Tell shoreside pPointAssign that this vehicle is ready to receive
   // assigned VISIT_POINT mail. The broker aliases GENPATH_READY to
   // GENPATH_READY_<VNAME> on the shoreside community.
   Notify("GENPATH_READY", "true");
   if(m_vname != "")
     Notify("GENPATH_READY_" + m_vname, "true");
   m_ready_posted = true;

   return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool GenPath::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // Keep advertising readiness until the first batch arrives.
  // In this mission the vehicles launch before shoreside, so a one-shot
  // GENPATH_READY post can be missed by pPointAssign.
  if(!m_firstpoint_received) {
    Notify("GENPATH_READY", "true");
    if(m_vname != "")
      Notify("GENPATH_READY_" + m_vname, "true");
    m_ready_posted = true;
  }

  // Keep WPT_UPDATE alive until pHelmIvP clearly accepts it and starts
  // producing a non-zero DESIRED_SPEED. This directly prevents the logged
  // failure: BHV_Waypoint starts after the one-shot update and warns
  // "No waypts given", causing IVPHELM_ALLSTOP=NothingToDo.
  if(shouldKeepPostingPath()) {
    double gap = (m_deployed || m_surveying || !m_manual_override) ? 0.5 : 2.0;
    if((MOOSTime() - m_last_path_post_time) > gap)
      postPathUpdate();
  }

  if(m_path_generated && !m_path_accepted && m_path_update_burst > 0 &&
     (MOOSTime() - m_last_path_post_time) > 0.25) {
    postPathUpdate();
    m_path_update_burst--;
  }

  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool GenPath::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  STRING_LIST::iterator p;
  for(p=sParams.begin(); p!=sParams.end(); p++) {
    string orig  = *p;
    string line  = *p;
    string param = tolower(biteStringX(line, '='));
    string value = stripBlankEnds(line);

    bool handled = false;

    if(param == "updates_var") {
      m_updates_var = value;
      handled = true;
    }
    else if(param == "vname") {
      m_vname = tolower(value);
      handled = true;
    }

    if(!handled)
      reportUnhandledConfigWarning(orig);
  }
  
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void GenPath::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();

  // Receive points from shoreside.
  Register("VISIT_POINT", 0);
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("MODE", 0);
  Register("DEPLOY", 0);
  Register("RETURN", 0);
  Register("STATION_KEEP", 0);
  Register("TSP_DONE", 0);
  Register("MOOS_MANUAL_OVERRIDE", 0);
  Register("IVPHELM_ALLSTOP", 0);
  Register("BHV_WARNING", 0);
  Register("HELM_WARNING", 0);
  Register("DESIRED_SPEED", 0);
}


//------------------------------------------------------------
// Procedure: buildReport()

bool GenPath::buildReport()
{
  m_msgs << "============================================" << endl;
  m_msgs << "File: GenPath.cpp                           " << endl;
  m_msgs << "============================================" << endl;

  // AppCasting report for debugging
  ACTable actab(2);
  actab << "Variable | Value";
  actab.addHeaderLines();

  actab << "Updates variable"     << m_updates_var;
  actab << "Vehicle name"         << m_vname;
  actab << "Ready posted"         << (m_ready_posted ? "true" : "false");
  actab << "Path update posts"    << uintToString(m_path_posts);
  actab << "Surveying mode"       << (m_surveying ? "true" : "false");
  actab << "Deployed"             << (m_deployed ? "true" : "false");
  actab << "Manual override"      << (m_manual_override ? "true" : "false");
  actab << "Path accepted"        << (m_path_accepted ? "true" : "false");
  actab << "Returning"            << (m_returning ? "true" : "false");
  actab << "Station keep"         << (m_station_keep ? "true" : "false");
  actab << "TSP done"             << (m_tsp_done ? "true" : "false");
  actab << "Update burst left"    << uintToString(m_path_update_burst);
  actab << "Firstpoint received"  << (m_firstpoint_received ? "true" : "false");
  actab << "Lastpoint received"   << (m_lastpoint_received ? "true" : "false");
  actab << "NAV received"         << (m_nav_received ? "true" : "false");
  actab << "Path generated"       << (m_path_generated ? "true" : "false");
  actab << "Points received"      << uintToString(m_points_received);
  actab << "Invalid points"       << uintToString(m_invalid_points);
  actab << "Points in the path"       << uintToString(m_points_in_path);
  actab << "NAV_X"                << doubleToStringX(m_nav_x, 2);
  actab << "NAV_Y"                << doubleToStringX(m_nav_y, 2);

  m_msgs << actab.getFormattedString();

  return(true);
}
