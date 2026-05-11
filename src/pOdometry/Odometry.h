/************************************************************/
/*    NAME: Adriano                                              */
/*    ORGN: MIT, Cambridge MA                               */
/*    FILE: Odometry.h                                          */
/*    DATE: December 29th, 1963                             */
/************************************************************/

#ifndef Odometry_HEADER
#define Odometry_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

class Odometry : public AppCastingMOOSApp
{
 public:
   Odometry();
   ~Odometry();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   void registerVariables();
   bool buildReport();

 private: // State variables
  bool   m_first_reading;

  double m_current_x;
  double m_current_y;

  double m_previous_x;
  double m_previous_y;

  double m_total_distance;
    
  //5.2 Run Time Checks
  double m_last_nav_time;
  bool   m_nav_warning_active;
  bool m_nav_received;  // NEW
  // Warning string
  std::string m_warning_msg;
  
  //5.3 Staleness Threshold
  double m_stale_threshold;
   
  //5.4 
  double m_odom_scale;

  // Assignment 8 (6.6)
  double m_nav_depth;
  double m_depth_thresh;
  double m_total_distance_depth;


};

#endif 
