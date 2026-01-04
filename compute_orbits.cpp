// Assessment 1: n-body gravitational solver

// To avoid warnings tell the compiler to use a recent standard of C++:

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "vector3d.hpp"
#include "body.hpp"

using std::cout, std::endl;

// *** Function declarations ***
// Compute the total energy and angular momentum of all bodies
void compute_energy_L(std::vector<body> &system);
// Update accelerations in system from current positions of all bodies
void update_acc(std::vector<body> &system); 
// Updated system according to the velocity Verlet method
void vel_verlet(std::vector<body> &curr_system, double dt);
// Read input data from file
void read_init(std::string input_file, std::vector<body> &system);
// Read the components of a 3d vector from a line
void read_vector3d(std::stringstream& data_line, double& x, double& y, double& z);
// Save the data to file
void save_data(std::ofstream& savefile, const std::vector<body> &system, double t);

int main(int argc, char* argv[])
{
  // Checking if number of arguments is equal to 4:
  if (argc != 6) {
    cout << "ERROR: need 4 arguments - compute_orbits <input_file> <output_file> <dt> <T> <Tsave>" << endl;
    return EXIT_FAILURE;
  }
  // Process command line inputs:
  std::string input_file = argv[1];
  std::string output_file = argv[2];
  double dt = atof(argv[3]); // Time step
  int T = atoi(argv[4]); // Total number of time steps
  int Tsave = atoi(argv[5]); // Number of steps between each save

  std::vector<body> system; // Create an empty vector container for bodies
  read_init(input_file, system); // Read bodies from input file into system
  int N = system.size(); // Number of bodies in system

  cout << "--- Orbital motion simulation ---" << endl;
  cout << " number of bodies N: " << N << endl;
  for(int p=0; p < N; p++)
  {
    cout << "- " << system[p].get_name() << endl; // Display names
  }
  cout << "       time step dt: " << dt << endl;
  cout << "  number of steps T: " << T << endl;
  cout << "   save steps Tsave: " << Tsave << endl;

  std::ofstream savefile (output_file); // Open save file
  if (!savefile.is_open()) {
    cout << "Unable to open file: " << output_file << endl; // Exit if save file has not opened
    return EXIT_FAILURE;
  }
  savefile << std::setprecision(16); // Set the precision of the output to that of a double
  // Write a header for the save file
  savefile << dt << "," << T << "," << Tsave << "," << N << endl;
  for(int p=0; p < (N-1); p++)
  {
    savefile << (p+1) << " = " << system[p].get_name() << ",";
  }
  savefile << N << " = " << system[N-1].get_name() << endl;
  // Write the column labels
  savefile << "time,energy,ang_mom";
  for(int p=0; p < N; p++)
  {
    savefile << ",r" << (p+1) << "x,r" << (p+1) << "y,r" << (p+1) << "z,v" << (p+1) << "x,v" << (p+1) << "y,v" << (p+1) << "z";
  }
  savefile << endl;

  compute_energy_L(system); // Compute initial energies and angular momentum of each body
  save_data(savefile, system, 0); // Add initial data to file 
  for(int t = 1; t <= T; t++)
  {  
    vel_verlet(system, dt); // Integrate a time-step forward
    
    if (!(t%Tsave)) // Save data
    {
      cout << "Time: " << t*dt << endl;
      compute_energy_L(system); // Compute the energies and angular momentum of each body
      save_data(savefile, system, t*dt); // Add data to the save file
    }
  }
  
  savefile.close();
  return EXIT_SUCCESS; 
}

// *** Function implementations ***

void compute_energy_L(std::vector<body> &system)
{
  for(int p = 0; p < system.size(); p++)
  {
    double mp = system[p].get_mass(); // Mass of body p
    vec vp = system[p].get_vel(); // Velocity of body p
    system[p].set_ke(0.5*mp*vp.lengthsq()); // Update the kinetic energy

    double energy = 0; 
    for(int j = 0; j < system.size(); j++)
    {
      if (j != p) // Compute forces on p from all other bodies
      {
        double mj = system[j].get_mass(); // Mass of body j
        double rj_rp = system[j].distance(system[p]); // Disance between j and p
        energy += -(mj*mp)/rj_rp; // Add gravitational potential energy of body p due to body j
      }
    }
    system[p].set_gpe(energy); // Update the gravitational potential energy for body p
    vec L = system[p].angular_momentum(vec(0,0,0)); // Angular momentum of body p relative to the origin
    system[p].set_L(L); // Update the angular momentum for body p
  }
}

void update_acc(std::vector<body> &system)
{
  for(int p = 0; p < system.size(); p++)
  {
    vec acc; // Updated acceleration
    for(int j = 0; j < system.size(); j++)
    {
      if (j != p) // Compute forces on p from all other bodies
      {
        double mj = system[j].get_mass(); // Mass of body j
        double rj_rp = system[j].distance(system[p]); // Disance between j and p
        vec e_jp = system[j].direction(system[p]); // Unit vector point from j to p
        acc += (mj/(rj_rp*rj_rp))*e_jp; // Acceleration according to Newton's law
      }
    }
    system[p].set_acc(acc); // Update the acceleration for body p
  }
}

void vel_verlet(std::vector<body> &curr_system, double dt)
{
  std::vector<body> new_system = curr_system; // Make a copy of the current system
  update_acc(curr_system); // Update the accelerations for current system
  // Update the positions:
  for(int p = 0; p < new_system.size(); p++)
  {
    vec rp = curr_system[p].get_pos(); 
    vec vp = curr_system[p].get_vel();
    vec ap = curr_system[p].get_acc(); 
    vec new_rp = rp + vp*dt + 0.5*ap*dt*dt; // Position update
    new_system[p].set_pos(new_rp);
  }
  update_acc(new_system); // Update the accelerations for new system
  // Update the velocities:
  for(int p = 0; p < new_system.size(); p++)
  { 
    vec vp = curr_system[p].get_vel();
    vec ap = curr_system[p].get_acc(); 
    vec new_ap = new_system[p].get_acc(); 
    vec new_vp = vp + 0.5*(ap + new_ap)*dt; // Velocity update
    new_system[p].set_vel(new_vp);
  }
  curr_system = new_system; // Now assign the updated system to current system
}

void read_init(std::string input_file, std::vector<body> &system)
{
  std::string line; // Declare a string to store each line
  std::string name; // String to store body name
  double m, x, y, z, vx, vy, vz; // Doubles to store vector components
  int line_cnt = 0; // Line counter

  // Declare and initialise an input file stream object
  std::ifstream data_file(input_file); 

  while (getline(data_file, line)) // Read the file line by line
  {
    line_cnt++;
    std::stringstream data_line(line); // Create a string stream from the line
    switch (line_cnt)
    {
      case 1:
        name = line;
        break;
      case 2:
        m = std::stod(line); // Convert string line into double
        break;            
      case 3:
        read_vector3d(data_line, x, y, z); // Read the 3 components of the vector on the line
        break;
      case 4:
        read_vector3d(data_line, vx, vy, vz); // Read the 3 components of the vector on the line
        break;                   
      }
      if (line_cnt==4) // Data for one body has been extracted
      {
        line_cnt = 0; // Reset line counter
        body b(name,m,vec(x,y,z),vec(vx,vy,vz)); // Package data into body
        system.push_back(b); // Add body to system
      }
    }
    // Close the file
    data_file.close();
}

void read_vector3d(std::stringstream& data_line, double& x, double& y, double& z)
{
  std::string value; // Declare a string to store values in a line
  int val_cnt = 0; // Value counter along the line
  
  while (getline(data_line, value, ','))
  {
    val_cnt++;
    switch (val_cnt)
    {
      case 1:
        x = std::stod(value); 
        break;
      case 2:
        y = std::stod(value); 
        break;
      case 3:
        z = std::stod(value);
        break;
    }
  } 
}

void save_data(std::ofstream& savefile, const std::vector<body> &system, double t)
{
    // Function for saving the simulation data to file.

    vec L; // Total angular momentum
    double E = 0.0; // Total energy
    for(int p = 0; p < system.size(); p++)
    { 
      E += system[p].get_ke() + 0.5*system[p].get_gpe();
      L += system[p].get_L();
    }
    double Lmag = L.length(); // Magnitude of total angular momentum

    // Write a header for this time-step with time, total energy and total mag of L:
    savefile << t << "," << E << "," << Lmag;

    // Loop over the bodies:
    for(int p = 0; p < system.size(); p++)
    { 
      // Output position and velocity for each body:
      savefile << "," << system[p].get_pos() << "," << system[p].get_vel();
    }
    savefile << endl;
}