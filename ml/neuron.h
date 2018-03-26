/*
 * Author : Cormac Guerin 
 *
 * Name: object.h
 * A class defining simulation objects in our world.
 * 
 * For a robot for example an object would represent a limb etc.
 *
 * The position and orientation is stored in Eigen Matrices
 * The corresponding physics body is also store here
 *
 * Essentially this is a state container for the body/object
 *
 */

#ifndef _NEURON_H_
#define _NEURON_H_


#include <iostream>
#include <vector>

namespace Neuron {
	class Neuron {
		private:
		public:
			Neuron();
			~Neuron();

			std::vector<std::string> data;
			float syn_in, syn_out, syn_out_neg;
			int id;
	};
}

#endif

