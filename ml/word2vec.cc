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
}

Word2Vec::~Word2Vec()
{
}

Word2Vec w2v;

void Word2Vec::init() {
  //train_words = vocab.size();
}

void Word2Vec::processline(std::vector<std::string> line) {
  clock_t now = clock();
  float progress_ = word_count_actual / (float)(window * train_words + 1) * 100;
  float wts_ = word_count_actual / ((float)(now - start + 1) / (float)CLOCKS_PER_SEC * 1000);
  cout << "Alpha: " << "Progress: " << progress_ << " Words/thread/sec: " << wts_ << endl;
  local_iter = window;
  alpha = starting_alpha * (1 - word_count_actual / (float)(window * train_words + 1));
  if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;
  // cout << "process line " << line << endl;
  return;
}

