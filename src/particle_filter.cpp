/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	default_random_engine gen;
	normal_distribution<double> dist_x{x, std[0]};
	normal_distribution<double> dist_y{y, std[1]};
	normal_distribution<double> dist_theta{theta, std[2]};

	num_particles = 10;
	for (int i = 0; i < num_particles; i++) {
		particles.push_back(Particle{
			i,
			dist_x(gen),
			dist_y(gen),
			dist_theta(gen),
			1.0
		});
	}
	is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
	default_random_engine gen;
	normal_distribution<double> dist_x{0, std_pos[0]};
	normal_distribution<double> dist_y{0, std_pos[1]};
	normal_distribution<double> dist_theta{0, std_pos[2]};

	for (auto& particle : particles) {
		double x, y, theta;
		double x_0 = particle.x;
		double y_0 = particle.y;
		double t_0 = particle.theta;
		if (yaw_rate != 0) {
			x = x_0 + velocity / yaw_rate * (sin(t_0 + yaw_rate*delta_t) - sin(t_0));
			y = y_0 + velocity / yaw_rate * (cos(t_0) - cos(t_0 + yaw_rate*delta_t));
			theta = t_0 + yaw_rate*delta_t;
		} else {
			x = x_0 + velocity * delta_t * cos(t_0);
			y = y_0 + velocity * delta_t * sin(t_0);
			theta = t_0;
		}
		particle.x = x + dist_x(gen);
		particle.y = y + dist_y(gen);
		particle.theta = theta + dist_theta(gen);
	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
	double sig_x = std_landmark[0];
	double sig_y = std_landmark[1];
	double gauss_norm = 1 / (2 * M_PI * sig_x * sig_y);
	double var_x = 2 * sig_x * sig_x;
	double var_y = 2 * sig_y * sig_y;
	weights.clear();
	for (auto& part : particles) {
		double weight = 1.0;
		for (const auto& observation : observations) {
			double r = sqrt(pow(observation.x,2.0) + pow(observation.y,2.0));
			double phi = atan2(observation.y, observation.x);
			// convert from vehicle's coordinate to map coordinate.
			LandmarkObs obs{0, r*cos(phi+part.theta)+part.x, r*sin(phi+part.theta)+part.y};
			double min_dx = sensor_range * sensor_range;
			double min_dy = sensor_range * sensor_range;
			// find nearest landmark
			for (const auto& landmark : map_landmarks.landmark_list) {
				double dx = landmark.x_f - obs.x;
				dx *= dx;
				double dy = landmark.y_f - obs.y;
				dy *= dy;
				if (dx + dy < min_dx + min_dy && sqrt(dx + dy) <= sensor_range) {
					min_dx = dx;
					min_dy = dy;
				}
			}
			double exponent = min_dx / var_x + min_dy / var_y;
			weight *= gauss_norm * exp(-exponent);
		}
		part.weight = weight;
		weights.push_back(weight);
	}
	max_weight = *max_element(begin(weights), end(weights));
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	int index = rand() % num_particles;
	double beta = 0.0;
	std::vector<Particle> new_particles;
	// resampling wheel
	for (int i = 0; i < num_particles; i++) {
		beta += rand() / (double)RAND_MAX * 2.0 * max_weight;
		while (beta > particles[index].weight) {
			beta -= particles[index].weight;
			index = (index + 1) % num_particles;
		}
		new_particles.push_back(particles[index]);
	}
	particles = new_particles;
}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations= associations;
 	particle.sense_x = sense_x;
 	particle.sense_y = sense_y;

 	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
