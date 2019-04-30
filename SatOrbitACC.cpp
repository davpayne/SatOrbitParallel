// For calculating satellite orbital collisions/links

#include <stdio.h> // printf
#include <math.h> // fmod
#include <cmath> // sin cos acos
#include <accelmath.h>
#define PI 3.14159265358979323846

// Define a data structure for the two-line element (TLE), a standard form of satellite position/trajectory
// TLEs are available from "https://www.celestrak.com/NORAD/elements/". These are pulled from US government sources
typedef struct param_TLE{
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

bool collision_risk(param_TLE sat1, param_TLE sat2);

//double altitude_calc(double mean_motion);

int load_sat_data(param_TLE **sat_array, int number_of_sats, int number_of_t_steps);
// void *sat_array_in

int main(){

  for(int number_of_time_steps = 10; number_of_time_steps < 100000; number_of_time_steps*=2){
    // int number_of_time_steps = 8000; // How far in the future do you want to propogate 

 int number_of_satellites = 10; // How many satellites are you analyzing

  // OpenACC initialize
  #pragma acc init

  int time_step_size; // How big are the time steps, in seconds
  time_step_size = 1; 
  
  // Initialize an array with a row for each satellite and a column for each time step
  param_TLE** sats_over_time;

  sats_over_time = new param_TLE*[number_of_satellites];

  for(int i=0; i<number_of_satellites; i++){
    sats_over_time[i] = new param_TLE[number_of_time_steps];
  }
  //param_TLE sats_over_time[number_of_satellites][number_of_time_steps];
  //  bool collision_risk_over_time[number_of_time_steps][number_of_satellites][number_of_satellites];

  // Use function to load satellite data into TLE array made above
  load_sat_data(sats_over_time, number_of_satellites, number_of_time_steps);

  // Display size of array. # of rows = number of satellites and # of columns = number of time steps 
  //  printf("Number of Satellites:%d | Number of Time Steps:%d\n", sizeof(sats_over_time)/sizeof(sats_over_time[0]), sizeof(sats_over_time[0])/sizeof(sats_over_time[0][0]));
  printf("Number of Satellites:%d | Number of Time Steps: %d\n", number_of_satellites, number_of_time_steps);

  //  printf("Sanity check: %d\n", sats_over_time[0][0].sat_num);

  
  // Go through each time step. -1 as the first time step is filled with the initial conditions

  double motion_deg; // Mean motion converted from revolutions per day to degrees per day
  double true_anomaly_rads; // What is the actual angle for the elliptical orbit. For circular orbits, mean_anomaly=true_anomaly
  double mean_anomaly_radians; // Needs to be in radians for sin
  double eccentric_anomaly_radians;

#pragma acc data copy(sats_over_time[0:number_of_satellites][0:number_of_time_steps]) 

  for (int time_loops= 0; time_loops < (number_of_time_steps-1); time_loops++){
      // Parallelize for each satellite as those are independent
      #pragma acc parallel loop 

    for(int sat_loops= 0; sat_loops<number_of_satellites; sat_loops++){
	param_TLE current_sat_TLE = sats_over_time[sat_loops][time_loops];
	param_TLE next_time_sat_TLE = sats_over_time[sat_loops][time_loops+1];

	// Want to parallelize each calculation as each only depends on current
	// Without thrust or any unexpected force, several variables don't change    
	next_time_sat_TLE.inclination = current_sat_TLE.inclination;
	
	next_time_sat_TLE.raan = current_sat_TLE.raan;

	next_time_sat_TLE.perigee = current_sat_TLE.perigee;

	next_time_sat_TLE.mean_motion = current_sat_TLE.mean_motion - current_sat_TLE.drag*time_step_size;

  // Simplified to circular
  // new_anomaly=(mean_motion*(time_step_size) + current_anomaly)modulo(360)
  // modulo(360) as mean_anomaly bounded by 0 and 360 since it's just tracking how far along an ellipse you are
  // note: time_step_size can be used as given or caclculated by (end_time-start_time)
	  motion_deg = current_sat_TLE.mean_motion*360; // converts mean_motion from revolutions per day to degrees per day
	 
	next_time_sat_TLE.mean_anomaly = fmod(motion_deg*(time_step_size) + current_sat_TLE.mean_anomaly, 360);

  // Simplified for true anomaly. Getting the accurate true anomaly from mean anomaly requires a numerical method, no analytical solution exists. All values in radians
  // true_anomaly = mean_anomaly + 2*eccentricity*sin(mean_anomaly) + 1.25 * (eccentricity^2) * sin(2*mean_anomaly)
	mean_anomaly_radians = current_sat_TLE.mean_anomaly*PI/180;
	true_anomaly_rads = mean_anomaly_radians + 2*current_sat_TLE.eccentricity*sin(mean_anomaly_radians) + 1.25 * (current_sat_TLE.eccentricity*current_sat_TLE.eccentricity)*sin(2*mean_anomaly_radians);
  
  // cos-1((eccentricity+cos(true_anomaly_rads))/(1+eccentricity*cos(true_anomaly_rads)))

  eccentric_anomaly_radians = acos((current_sat_TLE.eccentricity+cos(true_anomaly_rads))/(1.0 + current_sat_TLE.eccentricity*cos(true_anomaly_rads)));


   // e = (eccentric anomaly - mean anomaly)/sin(eccentric anomaly)

	next_time_sat_TLE.eccentricity = (eccentric_anomaly_radians - mean_anomaly_radians)/sin(eccentric_anomaly_radians);
	
	  next_time_sat_TLE.drag = current_sat_TLE.drag;
	
	next_time_sat_TLE.sat_num = current_sat_TLE.sat_num; // This value doesn't change
	
	//	altitude_calc(current_sat_TLE.mean_motion);

	// Update
	sats_over_time[sat_loops][time_loops+1] = next_time_sat_TLE;
      }
    }

  // Calculate collision risks

  bool collision;
  int collision_risk_counter = 0;

#pragma acc data copy(sats_over_time[0:number_of_satellites][0:number_of_time_steps]) copy(collision_risk_counter) 
// Go through each time step. Start at 1 as the first time step (0) is filled with the initial conditions
  for (int t_loops=1; t_loops<number_of_time_steps; t_loops++){
      // Parallelize for each satellite as those are independent
      //#pragma acc parallel loop 

    // -1 as last satellite in list will already be checked against everything else
    for(int sat_loops=0; sat_loops<(number_of_satellites-1); sat_loops++){
#pragma acc parallel loop reduction(+:collision_risk_counter)
      for(int compare_loops = sat_loops+1; compare_loops < number_of_satellites ; compare_loops++){
	collision = collision_risk(sats_over_time[sat_loops][t_loops], sats_over_time[compare_loops][t_loops]);
	if(collision){
	  // printf("Collision risk at time step: %d between satellites: %d and %d\n", time_loops, sats_over_time[sat_loops][0].sat_num, sats_over_time[compare_loops][0].sat_num);
	  collision_risk_counter += 1;
	  //	  collision_risk_over_time[time_loops][sat_loops][compare_loops] = true;
	} else {
	  //collision_risk_over_time[time_loops][sat_loops][compare_loops] = false;
	  collision_risk_counter = collision_risk_counter;
	}
      }
    } 
  }
  
  for(int i = 1; i < number_of_satellites; i++){
    delete[] sats_over_time[i];
  }
  delete[] sats_over_time;
  
  //free(collision_risk_over_time);

  printf("Number of collison risks identified: %d\n", collision_risk_counter);  

  }
  return 0;
}

/*
double altitude_calc(double mean_motion){
  // accepts mean motion in revolutions per day
  double altitude; // km
  double orbital_period_s;
  double G = 6.674*pow(10, -11); // m^3/(kg*s^2)
  double M = 5.972*pow(10, 24); // in kg
  
  orbital_period_s = 24*60*60/mean_motion; // 24 hours in a day, 60 minutes/hour, 60 seconds/min
  printf("Orbital period: %f\n", orbital_period_s);
  // approximating altitude as semi-major axis. Roughly right for circular orbits (aka low eccentricity)
  // altitude (a) = cube_root(G*M*(orbital_period/2*pi)^2)
  altitude = cbrt(G*M*pow(orbital_period_s,2))/1000;
  printf("Altitude: %f\n", altitude);
  return altitude;
  }*/

bool collision_risk(param_TLE sat1, param_TLE sat2){
  bool motion_check;
  bool anomaly_check;
  double degrees_to_rads = PI/180;
  double sat1_pos;
  double sat2_pos;

  if (sat1.mean_motion/sat2.mean_motion > 0.98 & sat1.mean_motion/sat2.mean_motion < 1.02){
    motion_check = true;
  } else {
    motion_check = false;
  }

  sat1_pos = sat1.mean_anomaly*degrees_to_rads+sat1.perigee*degrees_to_rads;
  sat2_pos = sat2.mean_anomaly*degrees_to_rads+sat2.perigee*degrees_to_rads;

  if (sat1_pos/sat2_pos > 0.98 & sat1_pos/sat2_pos < 1.02) {
    anomaly_check = true;
  } else {
    anomaly_check = false;
  }

  if (motion_check == true & anomaly_check == true) {
    return true;
  } else {
    return false;
  }
}

//void load_sat_data(param_TLE sat_array[number_of_satellites][number_of_time_steps]){
int load_sat_data(param_TLE **sat_array, int number_of_sats, int number_of_t_steps){
  // void *sat_array_in
  //param_TLE (*sat_array)[number_of_t_steps] = (param_TLE (*)[number_of_t_steps]) sat_array_in;

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
  cosmos1191_TLE.drag = 0.001; 
  
  sat_array[0][0] = cosmos1191_TLE;
  //(*sat_array)[0][0] = cosmos1191_TLE;
  
  /* Cosmos 1217 Pulled from https://github.com/Bill-Gray/sat_code/blob/master/test.tle */
  //1 12032U 80085A   01309.42683181  .00000182  00000-0  10000-3 0  3499
  //2 12032  65.2329  86.7607 7086222 172.0967 212.4632  2.00879501101699
  param_TLE cosmos1217_TLE;
  cosmos1217_TLE.sat_num = 12032;
  cosmos1217_TLE.inclination = 65.2329;
  cosmos1217_TLE.raan = 86.7607;
  cosmos1217_TLE.eccentricity = 0.7086222; // decimal point assummed normally. Added it in for testing
  cosmos1217_TLE.perigee = 172.0967;
  cosmos1217_TLE.mean_anomaly = 212.4632;
  cosmos1217_TLE.mean_motion = 02.00879501101699;
  cosmos1217_TLE.drag = 0.001; 
  sat_array[1][0] = cosmos1217_TLE;
  /* RADIX                   
1 43550U 98067NY  19119.83042788  .00019844  00000-0  21212-3 0  9995
2 43550  51.6355 230.5317 0005263 325.7679  34.2976 15.63882166 45354 */
  param_TLE radix_TLE;
  radix_TLE.sat_num = 43550;
  radix_TLE.inclination = 51.6355;
  radix_TLE.raan = 230.5317;
  radix_TLE.eccentricity = 0.0005263;
  radix_TLE.perigee = 325.7679;
  radix_TLE.mean_anomaly = 34.2976;
  radix_TLE.mean_motion = 15.63882166;
  radix_TLE.drag = 0.0021212;
  sat_array[2][0] = radix_TLE;
  /* ENDUROSAT ONE           
1 43551U 98067NZ  19119.79230236  .00013276  00000-0  15781-3 0  9995
2 43551  51.6377 232.5581 0006278 319.9247  40.1283 15.61486373 45273 */
  param_TLE enduro_TLE;
  enduro_TLE.sat_num = 43551;
  enduro_TLE.inclination = 51.6377;
  enduro_TLE.raan = 232.5581;
  enduro_TLE.eccentricity = 0.0006278;
  enduro_TLE.perigee = 319.9247;
  enduro_TLE.mean_anomaly = 40.1283;
  enduro_TLE.mean_motion = 15.61486373;
  enduro_TLE.drag = 0.0015781;
  sat_array[3][0] = enduro_TLE;

  /* DOVE 2                  
1 39132U 13015C   19119.87184211  .00000193  00000-0  24555-4 0  9999
2 39132  64.8767 273.0797 0015258 324.2075  97.7677 15.07263588331215 */
  param_TLE dove_TLE;
  dove_TLE.sat_num = 39132;
  dove_TLE.inclination = 64.8767;
  dove_TLE.raan = 273.0797;
  dove_TLE.eccentricity = 0.0015258;
  dove_TLE.perigee = 324.2075;
  dove_TLE.mean_anomaly = 97.7677;
  dove_TLE.mean_motion = 15.07263588331215;
  dove_TLE.drag = 0.00024555;
  sat_array[4][0] = dove_TLE;

  /* MAKERSAT 0              
1 43016U 17073D   19119.69141917  .00000755  00000-0  64779-4 0  9995
2 43016  97.7227  43.0462 0256925 321.3025  37.0019 14.78670319 77891 */
  param_TLE maker_TLE;
  maker_TLE.sat_num = 43016;
  maker_TLE.inclination = 97.227;
  maker_TLE.raan = 43.0462;
  maker_TLE.eccentricity = 0.0256925;
  maker_TLE.perigee = 321.3025;
  maker_TLE.mean_anomaly = 37.0019;
  maker_TLE.mean_motion = 14.78670319;
  maker_TLE.drag = 0.00064779;
  sat_array[5][0] = maker_TLE;

  /*DELLINGR (RBLE)         
1 43021U 98067NJ  19119.76769583  .00007607  00000-0  90255-4 0  9997
2 43021  51.6381 224.1686 0002159 285.7676  74.3080 15.62305522 81881 */
  param_TLE dellingr_TLE;
  dellingr_TLE.sat_num = 43021;
  dellingr_TLE.inclination = 51.6381;
  dellingr_TLE.raan = 224.1686;
  dellingr_TLE.eccentricity = 0.0002159;
  dellingr_TLE.perigee = 285.7676;
  dellingr_TLE.mean_anomaly = 74.3080;
  dellingr_TLE.mean_motion = 15.62305522;
  dellingr_TLE.drag = 0.00090255;
  sat_array[6][0] = dellingr_TLE;

  /* AEROCUBE 7B (OCSD B)    
1 43042U 17071F   19119.11702706  .00003375  00000-0  90548-4 0  9992
2 43042  51.6386 300.1248 0004291 338.2856  21.7946 15.40887783 78288 */
  param_TLE aero_TLE;
  aero_TLE.sat_num = 43042;
  aero_TLE.inclination = 51.6386;
  aero_TLE.raan = 300.1248;
  aero_TLE.eccentricity = 0.0004291;
  aero_TLE.perigee = 338.2856;
  aero_TLE.mean_anomaly = 21.7946;
  aero_TLE.mean_motion = 15.40887783;
  aero_TLE.drag = 0.00090548;
  sat_array[7][0] = aero_TLE;

  /* ARKYD 6A                
1 43130U 18004V   19119.77176876  .00000901  00000-0  41121-4 0  9997
2 43130  97.4917 187.5580 0011488  73.2090 287.0406 15.23152643 71891 */
  param_TLE arkyd_TLE;
  arkyd_TLE.sat_num = 43130;
  arkyd_TLE.inclination = 97.4917;
  arkyd_TLE.raan = 187.5580;
  arkyd_TLE.eccentricity = 0.0011488;
  arkyd_TLE.perigee = 73.2090;
  arkyd_TLE.mean_anomaly = 287.0406;
  arkyd_TLE.mean_motion = 15.23152643;
  arkyd_TLE.drag = 0.00041121;
  sat_array[8][0] = arkyd_TLE;

  /* PICSAT                  
1 43132U 18004X   19119.85221018  .00001169  00000-0  52057-4 0  9994
2 43132  97.4919 187.7607 0011389  73.4073 286.8413 15.23433697 71919 */
  param_TLE picsat_TLE;
  picsat_TLE.sat_num = 43132;
  picsat_TLE.inclination = 97.4919;
  picsat_TLE.raan = 187.7607;
  picsat_TLE.eccentricity = 0.0011389;
  picsat_TLE.perigee = 73.4073;
  picsat_TLE.mean_anomaly = 286.8413;
  picsat_TLE.mean_motion = 15.23433697;
  picsat_TLE.drag = 0.00052057;
  sat_array[9][0] = picsat_TLE;

  return 0;
}



