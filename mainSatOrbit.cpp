// For calculating satellite orbital collisions/links

#include <stdio.h>

// Define a data structure for the two-line element (TLE), a standard form of satellite position/trajectory
// TLEs are available from "https://www.celestrak.com/NORAD/elements/". These are pulled from US government sources
typedef struct{
  // satellite name
  int sat_num; // A unique number identifying that satellite 
  int epoch; // This is the time step of the given two-line element (TLE)
  double inclination; // How 0 is orbiting over equator, 90 is orbiting over poles
  double raan; //right ascension of the ascending node. in degrees
  double eccentricity; // Basically, how circular is it.Range- 0, perfectly circular; 0<eccentricity<1, elliptical;
  // =1, parabolic and is called the 'escape orbit'/'capture orbit'; >1 is hyperbolic
  double perigee; // This is the argument of perigee. Basically at what point is the satellite closest to the body it 
  //is orbiting. In degrees
  double mean_anomaly; // How long, in degrees, it has been since the satellite was at perigee. In degrees.  
  double mean_motion; // A measurement of speed, denotes how many times a day the satellite would orbit if you made
  // its speed constant. In revolutions per day
  double drag; // First Time Derivative of mean motion divied by two. Generally positive but can be negative
  // if there are weird effects such as alignment of moon and sun with satellite at a certain time to pull it up
} param_TLE;

double anomaly_calc(param_TLE current, int time_step_size);


int main(){
  int time_step_size; // How big are the time steps, in seconds
  time_step_size = 1; 
  // Takes in Two-Line Element (TLE) standard 
  param_TLE test_TLE;
  test_TLE.mean_anomaly = 2.0;
  test_TLE.mean_motion = 15.0;
  // At a first approximation, assuming a stable orbit (drag=0) the only value changing is mean anomaly
  double new_anomaly;
  new_anomaly = anomaly_calc(test_TLE, time_step_size);

  printf("space!!!\n");
  printf("Current anomaly=%f New anomaly=%f\n", test_TLE.mean_anomaly, new_anomaly);
  return 0;
}

double anomaly_calc(param_TLE current, int time_step_size){
  double new_anomaly;
  new_anomaly = 1.0; //placeholder file
  
  return new_anomaly;
}
