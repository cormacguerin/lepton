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

void Neuron::init(int size)
{
  syn_in = new float[size];
  syn_out = new float[size];
  syn_out_neg = new float[size];
  for (int i = 0; i < size; i++) {
    syn_in[i] = 0;
    syn_out[i] = 0;
    syn_out_neg[i] = 0;
  }
}

}

