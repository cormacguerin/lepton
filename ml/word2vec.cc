#include <iostream>
#include "word2vec.h"

using namespace std;

Word2Vec::Word2Vec()
{
      alpha = 0.025;
      sample = 1e-3;
      binary = 0, cbow = 1, debug_mode = 2, window = 5, min_count = 5, num_threads = 12, min_reduce = 1;
      vocab_max_size = 1000, vocab_size = 0, layer1_size = 100;
      train_words = 0, word_count_actual = 0, file_size = 0, classes = 0;
	  next_random = 1;
}

Word2Vec::~Word2Vec()
{
}

void Word2Vec::init() {
  //train_words = vocab.size();
}

void Word2Vec::initNeuron(int num_neurons, Neuron::Neuron* n, Neuron::Neuron* p) {
        for (int i = 0; i < 100; i++) {
	        next_random = next_random * (unsigned long long)25214903917 + 11;
                n->syn_in[i] = (((next_random & 0xFFFF) / (float)65536) - 0.5) / num_neurons;
                n->previous_neuron = p;
//                cout << "a n->syn_in[i] " << n->syn_in[i]  << endl;
//                cout << "a p->syn_in[i] " << p->syn_in[i]  << endl;
        }
}


void Word2Vec::trainCBOW(std::vector<std::string> content, Neuron::Neuron* n) {
	next_random = next_random * (unsigned long long)25214903917 + 11;
	long a, b, c, p = 0;
        b = next_random % window;
        std::string last_word;
	for(std::vector<std::string>::iterator it = content.begin(); it != content.end(); ++it) {
		for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
                        c = p - window + a;
                        if (c >= 0 && c < p) {
                                // this needs to be in vocab index.
                                // neu1[c] += syn0[c + last_word * layer1_size];
                                // we need to change this so the vocab index for the syn
                                // n->weight += n->syn_out[last_word];
                                int index_of_word_c = c;
                                n->weight += n->syn_in[index_of_word_c];
                        }
		}
                p++;
	}
	cout << n->weight << endl;
  clock_t now = clock();
  float progress_ = word_count_actual / (float)(window * train_words + 1) * 100;
  float wts_ = word_count_actual / ((float)(now - start + 1) / (float)CLOCKS_PER_SEC * 1000);
  // cout << "Alpha: " << "Progress: " << progress_ << " Words/thread/sec: " << wts_ << endl;
  local_iter = window;
  alpha = starting_alpha * (1 - word_count_actual / (float)(window * train_words + 1));
  if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;
  // cout << "process line " << line << endl;
  return;
}
