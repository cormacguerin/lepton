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

void Neuron::init(long size)
{
  for (int i = 0; i < size; i++) {
    syn_in[i] = 0;
    syn_out[i] = 0;
    syn_out_neg[i] = 0;
  }
}

}

