/************************************************************/
/*    NAME: Adriano                                         */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.cpp                                    */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#include <iterator>
#include <cmath>
#include <iostream> 

#include "MBUtils.h"
#include "ACTable.h"
#include "Odometry.h"

using namespace std;

//---------------------------------------------------------
// Constructor()

Odometry::Odometry()
{
  m_first_reading = true;
  m_current_x = 0;
  m_current_y = 0;
  m_previous_x = 0;
  m_previous_y = 0;
  m_total_distance = 0;

  // 5.2
  m_last_nav_time = 0;
  m_nav_warning_active = false;
  m_nav_received = false;

  //5.3
  m_stale_threshold = 10.0; 

  // 5.2/5.3 Warning string
  m_warning_msg = "No NAV updates detected";
  
  //5.4
  m_odom_scale = 1.0;

  // Assignment 8 (6.6)
  m_depth_thresh = 0;
  m_total_distance_depth = 0;
  m_nav_depth = 0;



}

//---------------------------------------------------------
// Destructor

Odometry::~Odometry()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail()

bool Odometry::OnNewMail(MOOSMSG_LIST &NewMail)
{
  AppCastingMOOSApp::OnNewMail(NewMail);

  for(auto &msg : NewMail) {
    string key = msg.GetKey();

    if(key == "NAV_X") {
      m_current_x = msg.GetDouble();
      m_last_nav_time = MOOSTime();
      m_nav_received = true;

      cout << "NAV_X received: " << m_current_x << endl;
    }
    else if(key == "NAV_Y") {
      m_current_y = msg.GetDouble();
      m_last_nav_time = MOOSTime();
      m_nav_received = true;

      cout << "NAV_Y received: " << m_current_y << endl;
    }
    else if(key == "ODOM_SCALE") {
      double val = msg.GetDouble();

      if(val > 0) {
        m_odom_scale = val;
        cout << "ODOM_SCALE updated: " << m_odom_scale << endl;
      }
      else {
        reportRunWarning("Invalid ODOM_SCALE (must be > 0)");
      }
    }
    else if(key == "NAV_DEPTH") {
        m_nav_depth = msg.GetDouble();
    }
    else if(key == "APPCAST_REQ") {
      // AppCast handled by base class or ignored
    }
    else {
      reportRunWarning("Unhandled Mail: " + key);
    }
  }

  // Compute distance
  if(!m_first_reading) {
    double dx = m_current_x - m_previous_x;
    double dy = m_current_y - m_previous_y;
    double dist = sqrt(dx*dx + dy*dy);

    m_total_distance += dist;

    if((m_depth_thresh <= 0) ||
       (m_nav_depth > m_depth_thresh)) {
      m_total_distance_depth += dist;
    }

    cout << "Step distance: " << dist << endl;
    cout << "Total ODOMETRY_DIST: " << m_total_distance << endl;
  }
  else {
    m_first_reading = false;
  }

  m_previous_x = m_current_x;
  m_previous_y = m_current_y;

  return true;
}

//---------------------------------------------------------
// Procedure: OnConnectToServer()

bool Odometry::OnConnectToServer()
{
  registerVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()

bool Odometry::Iterate()
{
  AppCastingMOOSApp::Iterate();

  // Publish result
double scaled_dist = m_total_distance * m_odom_scale;
Notify("ODOMETRY_DIST", scaled_dist);

// 6.6 Assignment 8
Notify("ODOMETRY_DIST_AT_DEPTH",
       m_total_distance_depth);

 
  // 5.2 Runtime check logic
  if(m_nav_received) {

    double delta = MOOSTime() - m_last_nav_time;

    if(delta >= m_stale_threshold) {

      if(!m_nav_warning_active) {
        reportRunWarning(m_warning_msg);
        m_nav_warning_active = true;
      }

    } else {

      if(m_nav_warning_active) {
        retractRunWarning(m_warning_msg);
        m_nav_warning_active = false;
      }
    }
  }

  // AppCast report
  AppCastingMOOSApp::PostReport();
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()

bool Odometry::OnStartUp()
{
  AppCastingMOOSApp::OnStartUp();

  STRING_LIST sParams;
  m_MissionReader.EnableVerbatimQuoting(false);

  if(!m_MissionReader.GetConfiguration(GetAppName(), sParams))
    reportConfigWarning("No config block found for " + GetAppName());

  for(auto &line : sParams) {
    string orig  = line;
    string param = tolower(biteStringX(line, '='));
    string value = line;

    bool handled = false;

    if(param == "stale_threshold") {

      double val;
      if(setDoubleOnString(val, value) && val > 0) {
        m_stale_threshold = val;
        handled = true;
      } else {
        reportConfigWarning("Invalid stale_threshold: " + value);
      }
    }
    else if(param == "depth_thresh") {

      double val;

     if(setDoubleOnString(val, value) && val >= 0) {
       m_depth_thresh = val;
       handled = true;
     }
     else {
       reportConfigWarning("Invalid depth_thresh: " + value);
     }
    }
    if(!handled)
      reportUnhandledConfigWarning(orig);
  }

  registerVariables();	
  return(true);
}

//---------------------------------------------------------
// Procedure: registerVariables()

void Odometry::registerVariables()
{
  AppCastingMOOSApp::RegisterVariables();
  Register("NAV_X", 0);
  Register("NAV_Y", 0);
  Register("ODOM_SCALE", 0);
  Register("NAV_DEPTH", 0);
}

//------------------------------------------------------------
// Procedure: buildReport()

bool Odometry::buildReport() 
{
  m_msgs << "____________________________________________" << endl;
  m_msgs << "           pOdometry Report                 " << endl;
  m_msgs << "____________________________________________" << endl;

  ACTable actab(2);
  actab << "Metric | Value";
  actab.addHeaderLines();

  actab << "Current X" << m_current_x;
  actab << "Current Y" << m_current_y;
  actab << "Previous X" << m_previous_x;
  actab << "Previous Y" << m_previous_y;
  actab << "Total Distance" << m_total_distance;
  actab << "Stale Threshold" << m_stale_threshold;
  actab << "Depth Threshold" << m_depth_thresh;
  actab << "NAV Depth" << m_nav_depth;
  actab << "Distance At Depth" << m_total_distance_depth;

  m_msgs << actab.getFormattedString();

  return(true);
}

