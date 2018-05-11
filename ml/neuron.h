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
#include <map>

namespace Neuron {
	class Neuron {
		private:
		public:
			Neuron();
			~Neuron();

            Neuron* previous_neuron;
			std::map<std::string,int> *data;
            void init(std::map<std::string,int> *data);
			std::vector<float> syn_in;
			std::vector<float> syn_out;
			std::vector<float> syn_out_neg;
			float error;
            float weight;
			int id;
			//float *syn_in, *syn_out, *syn_out_neg;
	};
}

#endif

