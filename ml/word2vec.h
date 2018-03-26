#ifndef _WORD2VEC_H_
#define _WORD2VEC_H_

#include <string>
#include <vector>
#include <math.h>

class Word2Vec {
	private:
          float alpha, starting_alpha, sample;
          int binary, cbow, debug_mode, window, min_count, num_threads, min_reduce;
          //int *vocab_hash;
          long long vocab_max_size, vocab_size, layer1_size, local_iter;
          long long train_words, word_count_actual, iter, file_size, classes;
          clock_t start;

	public:
	Word2Vec();
	~Word2Vec();
		void init();
		void processline(std::vector<std::string> line);

};

#endif

