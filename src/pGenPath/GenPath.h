/*****************************************************************/
/*    FILE: GenPath.h                                            */
/*****************************************************************/

#ifndef GenPath_HEADER
#define GenPath_HEADER

#include <string>

#include <vector>

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

struct GenPathPoint
{
  double x;

  double y;

  std::string id;
};

class GenPath : public AppCastingMOOSApp
{
 public:
   GenPath();
   ~GenPath();

 protected: // Standard MOOSApp functions to overload  
   bool OnNewMail(MOOSMSG_LIST &NewMail);
   bool Iterate();
   bool OnConnectToServer();
   bool OnStartUp();

 protected: // Standard AppCastingMOOSApp function to overload 
   bool buildReport();

 protected:
   void registerVariables();

 private: // Configuration variables
    
    // Process messages from shoreside
    bool handleVisitPoint(const std::string& sval);
    // Convert string into a GenPathPoint object
    bool parseVisitPoint(const std::string& sval, GenPathPoint& point) const;
    bool getField(const std::string& input,
                  const std::string& field,
                  std::string& value) const;

    // Generate the waypoint path (after points have been received)
    bool generatePath();
    void postPathUpdate();
    bool isSurveyingMode(const std::string& sval) const;
    bool shouldKeepPostingPath() const;
    // Calculate distance between two points.
    double distance(double x1, double y1, double x2, double y2) const;

 private: // State variables
    
    // Store points assigned to this vehicle
    std::vector<GenPathPoint> m_points;

    // Flags used to track the reception status of point sequence
      bool m_firstpoint_received;
      bool m_lastpoint_received;

      bool m_nav_received;
      bool m_nav_x_received;
      bool m_nav_y_received;
    // Flag to avoid generating the path more than once for the same point group
      bool m_path_generated;

    // Store the current vehicle position.
      double m_nav_x;
      double m_nav_y;

    // Debugging and AppCasting reports.
      unsigned int m_invalid_points;
      unsigned int m_points_received;
      unsigned int m_points_in_path;

      std::string m_updates_var;
      std::string m_vname;
      bool m_ready_posted;

      // Cached dynamic update for BHV_Waypoint. The helm/behavior can miss
      // a one-shot update if the path is generated while the helm is PARKed.
      std::string m_path_update;
      unsigned int m_path_posts;
      double m_last_path_post_time;
      bool m_surveying;
      bool m_deployed;
      bool m_manual_override;
      bool m_path_accepted;
      unsigned int m_path_update_burst;
      bool m_returning;
      bool m_station_keep;
      bool m_tsp_done;
    
};

#endif 
