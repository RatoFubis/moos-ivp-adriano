/*****************************************************************/
/*    FILE: PointAssign.h                                        */
/*****************************************************************/

#ifndef PointAssign_HEADER
#define PointAssign_HEADER

#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCastingMOOSApp.h"

#include <string>
#include <vector>
#include <set>

class PointAssign : public AppCastingMOOSApp
{
public:
  PointAssign();
  ~PointAssign() {}

protected:
  bool OnNewMail(MOOSMSG_LIST &NewMail);
  bool Iterate();
  bool OnConnectToServer();
  bool OnStartUp();

  void registerVariables();
  bool buildReport();

private:
  struct VisitPoint {
    double x;
    double y;
    std::string id;
    std::string raw;
    bool valid;

    VisitPoint() : x(0), y(0), valid(false) {}
  };

  bool handleVisitPoint(const std::string& sval);
  bool parseVisitPoint(const std::string& sval, VisitPoint& point) const;

  void distributePoints();
  void distributeAlternating();
  void distributeByRegion();

  void postVehiclePoint(const std::string& vname,
                        const std::string& point_spec);

  void postViewPoint(double x,
                     double y,
                     const std::string& label,
                     const std::string& color);

  std::string toupperStr(std::string s) const;
  std::string colorForVehicle(unsigned int ix) const;

private:
  //-------------------------------------------------------
  // Configuration
  std::vector<std::string> m_vnames;
  bool m_assign_by_region;
  bool m_wait_for_ready;
  bool m_unpause_timer;

  double m_region_xmin;
  double m_region_xmax;
  double m_region_ymin;
  double m_region_ymax;

  //-------------------------------------------------------
  // State
  bool m_received_first;
  bool m_received_last;
  bool m_distributed;

  unsigned int m_point_count;
  unsigned int m_invalid_points;
  unsigned int m_total_received;

  std::vector<VisitPoint> m_points;
  std::set<std::string> m_ready_vnames;
};

#endif
