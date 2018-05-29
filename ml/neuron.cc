#include <iostream>
#include <fstream>
#include <string>
#include "neuron.h"
#include <math.h>

using namespace std;

namespace Neuron {

	Neuron::Neuron()
	{
	/*
	   syn_in = 0;
	   syn_out = 0;
	   syn_out_neg = 0;
   */
	}

	Neuron::~Neuron()
	{
	}

	void Neuron::init(std::map<std::string,int> *data_)
	{
		data = data_;
		error = 0;
		weight = 0;
		int size = (*data).size();
		syn_in = new float[size*2];
		syn_out = new float[size*2];
		syn_out_neg = new float[size*2];
		/*
		for (int i = 0; i < size*2; i++) {
			syn_in.push_back(0);
			syn_out.push_back(0);
			syn_out_neg.push_back(0);
		}
		*/
	}

}

