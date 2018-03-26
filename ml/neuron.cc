#include <iostream>
#include <fstream>
#include <string>
#include "neuron.h"
#include <math.h>

using namespace std;

namespace Neuron {

Neuron::Neuron()
{
		syn_in = 0;
		syn_out = 0;
		syn_out_neg = 0;
}

Neuron::~Neuron()
{
}

}

