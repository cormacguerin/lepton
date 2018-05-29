#include "word2vec.h"

using namespace std;

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

Word2Vec::Word2Vec()
{
	alpha = 0.025;
	sample = 1e-3;
	negative = 5;
	binary = 0, cbow = 1, debug_mode = 2, window = 5, min_count = 5, num_threads = 12, min_reduce = 1;
	vocab_size = 0, layer1_size = 100;
	train_words = 0, word_count_actual = 0, file_size = 0, classes = 0;
	word_counter = 0;
	next_random = 1;
}

Word2Vec::~Word2Vec()
{
}

void Word2Vec::init() {
	//train_words = vocab.size();
	//expTable = new std::vector<float>()
	starting_alpha = alpha;
	expTable.reserve(EXP_TABLE_SIZE);
	for (int i = 0; i < EXP_TABLE_SIZE; i++) {
	//	expTable[i] = alpha * (i / (1 + abs(i)));
    	expTable[i] = exp((i / (float)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
    	expTable[i] = expTable[i] / (expTable[i] + 1);                   // Precompute f(x) = x / (x + 1)
  	}
}

/*
__global__ void vectorKernel(int N)
{
    int idx = blockIdx.x*blockDim.x + threadIdx.x;

    if (idx<N)
        printf("Hello from thread # %i (block #: %i)\n", idx, blockIdx.x);
}
*/

void Word2Vec::initNeuron(int num_neurons, Neuron::Neuron* n, Neuron::Neuron* p) {
	for (int i = 0; i < 100; i++) {
		next_random = next_random * (unsigned long long)25214903917 + 11;
		n->syn_in[i] = (((next_random & 0xFFFF) / (float)65536) - 0.5) / num_neurons;
		n->previous_neuron = p;
	}
}

/*
 * Args: 
 * current iterator in sentence vector (sentence position)
 * sentence vector, 
 * neurons vector,
 * vocab vector,
 */
void Word2Vec::trainCBOW(
		int index,
		std::vector<std::string>::iterator i, 
		std::vector<std::string> s, 
		std::vector<Neuron::Neuron*> n, 
		std::map<std::string,int> *v,
		int r) {

	// this should be more meaningful
	//train_words = sqrt(v->size())*1000;
	train_words = 39395687;
	// cout << "train_words : " << train_words << endl;
	clock_t now = clock();
	float progress_ = word_count_actual / (float)(window * train_words + 1) * 100;
	float wts_ = word_count_actual / ((float)(now - start + 1) / (float)CLOCKS_PER_SEC * 1000);
	// cout << "Alpha: " << "Progress: " << progress_ << " Words/thread/sec: " << wts_ << endl;
	local_iter = window;
/*	
	cout << "inital alpha " << alpha << endl;
	cout << "starting_alpha " << starting_alpha << endl;
	cout << "word_count_actual " << word_count_actual << endl;
	cout << "window " << window << endl;
	cout << "train words " << train_words << endl;
*/	
	if (index == 0) {
		alpha = starting_alpha * (1 - word_counter / (float)(train_words + 1));
		if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;
	//	cout << "alpha " << alpha<< endl;
	}
	word_counter++;
	//alpha = starting_alpha * (1 - word_count_actual / (real)(train_words + 1));
	// cout << "process line " << line << endl;
	
	next_random = next_random * (unsigned long long)25214903917 + 11;
	long a, b, c, p = 0;
	// b = next_random % window;
	b = r % window;
	p = index;
	// for a random window between window and 2*window
	for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
		c = p - window + a;
		if (c >= 0 && c < p) {
			for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
				if (v->count(*i)) {
					(*nit)->weight += (*nit)->syn_in[v->at(*i)];
				} else {
					string seg = "▁";
					string segV = seg.append(*i);
					if (v->count(segV)) {
						(*nit)->weight += (*nit)->syn_in[v->at(segV)];
					}
				}
			}
		}
	}
	return;
}

void Word2Vec::negSample(
		int index,
		std::vector<std::string>::iterator i, 
		std::vector<std::string> s, 
		std::vector<Neuron::Neuron*> n, 
		std::map<std::string,int> *v) {
	long label = 1;
	int r = std::rand() + 11;
	for (int d = 0; d < negative + 1; d++) {
		if (d == 0) {
        	label = 1;
        } else {
			// so this needs to be changed to sample more from the more common words
			//std::map<std::string,int>::iterator j = v->begin();
			int r = std::rand() % (v->size() + 1);
			auto w = v->begin();
			std::advance(w, r);
        	label = 0;
        }
		//long l2 = target;
		float f = 0;
		float g = 0;
		for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
			if (v->count(*i)) {
				f += (*nit)->weight * (*nit)->syn_out_neg[v->at(*i)];
			} else {
				string seg = "▁";
				string segV = seg.append(*i);
				if (v->count(segV)) {
					f += (*nit)->weight * (*nit)->syn_out_neg[v->at(segV)];
				}
			}
		}
		if (isnan(f)) f = 0;
		if (isinf(f)) f = 1;
//		cout << "NS f " << f << endl;
//		cout << "NS alpha " << alpha << endl;
//		cout << "NS debug " << (int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2)) << endl;
		if (f > MAX_EXP) g = (label - 1) * alpha;
        else if (f < -MAX_EXP) g = (label - 0) * alpha;
        else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;
//		cout << "NS g " << g << endl;
		for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
			if (v->count(*i)) {
					(*nit)->error += g * (*nit)->syn_out_neg[v->at(*i)];
			} else {
				string seg = "▁";
				string segV = seg.append(*i);
				if (v->count(segV)) {
					(*nit)->error += g * (*nit)->syn_out_neg[v->at(segV)];
				}
			}
		}
		for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
			if (v->count(*i)) {
					(*nit)->syn_out_neg[v->at(*i)] += g * (*nit)->weight;
			} else {
				string seg = "▁";
				string segV = seg.append(*i);
				if (v->count(segV)) {
					(*nit)->syn_out_neg[v->at(segV)] += g * (*nit)->weight;
				}
			}
		}
	}
}

void Word2Vec::learnWordVectors(
		int index,
		std::vector<std::string>::iterator i, 
		std::vector<std::string> s, 
		std::vector<Neuron::Neuron*> n, 
		std::map<std::string,int> *v,
		int r) {
	long a, b, c, p = 0;
	b = r % window;
	p = index;
	for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
		c = p - window + a;
		if (c >= 0 && c < p) {
			for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
				string seg = "▁";
				string segV = seg.append(*i);
				if (v->count(*i)) {
						(*nit)->syn_in[v->at(*i)] += (*nit)->error;
				} else {
					string seg = "▁";
					string segV = seg.append(*i);
					if (v->count(segV)) {
						(*nit)->syn_in[v->at(segV)] += (*nit)->error;
					}
				}
			}
		}
	}
}

void Word2Vec::printWordVectors(
		std::vector<Neuron::Neuron*> n, 
		std::map<std::string,int> *v) {
	for(std::map<string,int>::iterator vit = v->begin(); vit != v->end(); ++vit) {
		std::string word_vector = (*vit).first;
		for(std::vector<Neuron::Neuron*>::iterator nit = n.begin(); nit != n.end(); ++nit) {
			float value = (*nit)->syn_in[(*vit).second];
			word_vector.append("," + std::to_string(value));
		}
		cout << word_vector << endl;
	}
}

