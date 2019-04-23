// For calculating satellite orbital collisions/links

#include <stdio.h> // printf
#include <math.h> // fmod
#include <cmath> // sin
#define PI 3.14159265358979323846

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
  double mean_anomaly; // How long, in degrees, it has been since the satellite was at perigee.  
  double mean_motion; // A measurement of speed, denotes how many times a day the satellite would orbit if you made
  // its speed constant. In revolutions per day
  double drag; // First Time Derivative of mean motion divied by two. Generally positive but can be negative
  // if there are weird effects such as alignment of moon and sun with satellite at a certain time to pull it up
} param_TLE;

double inclination_calc(param_TLE current, int time_step_size);
double raan_calc(param_TLE current, int time_step_size);
double eccentricity_calc(param_TLE current, int time_step_size);
double perigee_calc(param_TLE current, int time_step_size);
double anomaly_calc(param_TLE current, int time_step_size);
double motion_calc(param_TLE current, int time_step_size);
double drag_calc(param_TLE current, int time_step_size);

double true_anomaly_radians(double mean_anomaly_degrees, double eccentricity);

int main(){

  // OpenACC initialize
  #pragma acc init

  int time_step_size; // How big are the time steps, in seconds
  time_step_size = 1; 
  
  int number_of_time_steps; // How far in the future do you want to propogate 
  number_of_time_steps = 100;

  int number_of_satellites; // How many satellites are you analyzing
  number_of_satellites = 1;

  param_TLE sats_over_time[number_of_satellites][number_of_time_steps];
  
  // Takes in Two-Line Element (TLE) standard 
  param_TLE test_TLE;
  test_TLE.mean_anomaly = 2.0;
  test_TLE.mean_motion = 15.0;
  
  //  sats_over_time[0][0] = test_TLE;
  //  sats_over_time[0][0].drag = 10.0;

  // At a first approximation, assuming a stable orbit (drag=0) the only value changing is mean anomaly

  //new_anomaly = anomaly_calc(test_TLE, time_step_size); // for test

  /* Cosmos 1191. Pulled from https://github.com/Bill-Gray/sat_code/blob/master/test.tle */
  //1 11871U 80057A   01309.36911127 -.00000499 +00000-0 +10000-3 0 08380
  //2 11871 067.5731 001.8936 6344778 181.9632 173.2224 02.00993562062886
  param_TLE cosmos1191_TLE;
  cosmos1191_TLE.sat_num = 11871;
  cosmos1191_TLE.inclination = 067.5731;
  cosmos1191_TLE.raan = 001.8936;
  cosmos1191_TLE.eccentricity = 0.6344778; // decimal point assummed normally. Added it in for testing
  cosmos1191_TLE.perigee = 181.9632;
  cosmos1191_TLE.mean_anomaly = 173.2224;
  cosmos1191_TLE.mean_motion = 02.0099356206886;
  cosmos1191_TLE.drag = 0.0; // Fictitious
  
  sats_over_time[0][0] = cosmos1191_TLE;
    
  param_TLE current_sat_TLE;
  param_TLE next_time_sat_TLE;

  for (int sat_loops=0; sat_loops<number_of_satellites; sat_loops++){
    #pragma acc parallel loop  
    for(int time_loops=0; (time_loops<number_of_time_steps-1); time_loops++){
	current_sat_TLE = sats_over_time[sat_loops][time_loops];
	next_time_sat_TLE = sats_over_time[sat_loops][time_loops+1];

	next_time_sat_TLE.mean_anomaly = anomaly_calc(current_sat_TLE, time_step_size);
    
	next_time_sat_TLE.inclination = inclination_calc(current_sat_TLE, time_step_size);
	
	next_time_sat_TLE.raan = raan_calc(current_sat_TLE, time_step_size);

	next_time_sat_TLE.eccentricity = eccentricity_calc(current_sat_TLE, time_step_size);
	
	next_time_sat_TLE.perigee = perigee_calc(current_sat_TLE, time_step_size);

	next_time_sat_TLE.mean_motion = motion_calc(current_sat_TLE, time_step_size);

	next_time_sat_TLE.drag = drag_calc(current_sat_TLE, time_step_size);

	// Update
	sats_over_time[sat_loops][time_loops+1] = next_time_sat_TLE;

	// Printing
	printf("Current anomaly=%f New anomaly=%f\n", current_sat_TLE.mean_anomaly, next_time_sat_TLE.mean_anomaly);
      }
    }
  return 0;
}

double anomaly_calc(param_TLE current, int time_step_size){
  double new_mean_anomaly; // Future mean anomaly that is outputted
  double motion_deg; // Mean motion converted from revolutions per day to degrees per day
  double true_anomaly_rads; // What is the actual angle for the elliptical orbit. For circular orbits, mean_anomaly=true_anomaly

  // Simplified to circular
  // new_anomaly=(mean_motion*(time_step_size) + current_anomaly)modulo(360)
  // modulo(360) as mean_anomaly bounded by 0 and 360 since it's just tracking how far along an ellipse you are
  // note: time_step_size can be used as given or caclculated by (end_time-start_time)
  motion_deg = current.mean_motion*360; // converts mean_motion from revolutions per day to degrees per day
  new_mean_anomaly = fmod(motion_deg*(time_step_size) + current.mean_anomaly, 360);

  true_anomaly_rads = true_anomaly_radians(current.mean_anomaly, current.eccentricity);
  //  printf("MA=%f, E=%f, True anomaly=%f\n", current.mean_anomaly, current.eccentricity, true_anomaly_rads);
  
    
  return new_mean_anomaly;
}

double true_anomaly_radians(double mean_anomaly_degrees, double eccentricity){
  double true_anomaly_rads; // What is the actual angle for the elliptical orbit. For circular orbits, mean_anomaly=true_anomaly
  double mean_anomaly_radians; // Needs to be in radians for sin

  // Simplified for true anomaly. Getting the accurate true anomaly from mean anomaly requires a numerical method, no analytical solution exists. All values in radians
  // true_anomaly = mean_anomaly + 2*eccentricity*sin(mean_anomaly) + 1.25 * (eccentricity^2) * sin(2*mean_anomaly)
  mean_anomaly_radians = mean_anomaly_degrees*PI/180;
  true_anomaly_rads = mean_anomaly_radians + 2*eccentricity*sin(mean_anomaly_radians) + 1.25 * (eccentricity*eccentricity)*sin(2*mean_anomaly_radians);

  return true_anomaly_rads;
} 

double inclination_calc(param_TLE current, int time_step_size){
  double new_inclination;
  
  new_inclination = current.inclination;

  return new_inclination;
}

double raan_calc(param_TLE current, int time_step_size){
  double new_raan;
  
  new_raan = current.raan;

  return new_raan;
}

double eccentricity_calc(param_TLE current, int time_step_size){
  double new_eccentricity;
  
  new_eccentricity = current.eccentricity;

  return new_eccentricity;
}

double perigee_calc(param_TLE current, int time_step_size){
  double new_perigee;
  
  new_perigee = current.perigee;

  return new_perigee;
}

double motion_calc(param_TLE current, int time_step_size){
  double new_motion;
  
  new_motion = current.mean_motion;

  return new_motion;
}

double drag_calc(param_TLE current, int time_step_size){
  double new_drag;
  
  new_drag = current.drag;

  return new_drag;
}





