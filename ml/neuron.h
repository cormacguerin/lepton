/*
 * Author : Cormac Guerin 
 *
 * Name: neuron.h
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

            Neuron* previous_neuron;
			std::vector<std::string> *data;
            void init(std::vector<std::string> *data);
            float weight;
			int id;
			float *syn_in, *syn_out, *syn_out_neg;
	};
}

#endif

