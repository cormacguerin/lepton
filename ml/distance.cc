#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <set>
#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>
#include <exception>
#include <regex>
#include <numeric>
#include <math.h>

using namespace std;

// This is a map of our words to vectors
std::map<std::string, std::vector<float>> word_vectors;

void readVectors(const char* f) {
  std::string line;
  ifstream csv(f);
  while (getline(csv, line)) {
    // we expect the first value to be the word
    bool isFirst = true;
    // a key value pair for word and vector
    std::pair<string, std::vector<float>> word_vector;

    std::stringstream ss(line);
    std::string item;
    while (getline(ss, item, ',')) {
      try {
        if (isFirst) {
          word_vector.first = item;
        } else {
          word_vector.second.push_back(::atof(item.c_str()));
        }
      } catch (exception& e) {
        cout << "Exception reading CSV vector file: " << e.what() << endl;
        exit;
      }
      isFirst = false;
    }
    word_vectors.insert(word_vector);
  }
}

float getRootOfSquaredSum(std::vector<float> v) {
  float va_sqr_sum = 0;
  for (std::vector<float>::iterator vi = v.begin(); vi != v.end(); ++vi) {
	cout << "*vi " << *vi << endl;
	va_sqr_sum += pow(*vi,2);
  }
  cout << sqrt(va_sqr_sum) << endl;
  return sqrt(va_sqr_sum);
}

float cosineDistance(std::string a, std::string b) {
  std::vector<float> va;
  std::vector<float> vb;
  if (word_vectors.count(a)) { 
    va = word_vectors.at(a);
  } else {
	cout << "unknown term " << a << endl;
  	return 0.0;
  }
  if (word_vectors.count(b)) { 
    vb = word_vectors.at(b);
  } else {
	cout << "unknown term " << b << endl;
  	return 0.0;
  }

  float dot_product = std::inner_product(std::begin(va), std::end(va), std::begin(vb), 0.0);
  float gross_a = getRootOfSquaredSum(word_vectors.at(a));
  float gross_b = getRootOfSquaredSum(word_vectors.at(b));
  float result = dot_product / (gross_a*gross_b);
  /*
  for (std::map<string, vector<float>>::iterator wvi = word_vectors.begin(); wvi != word_vectors.end(); ++wvi) {
    cout << wvi->first << endl;
    // cout << wvi->second << endl;
  }
  */
}

int main(int argc, char** argv)
{
  // seg = L'▁';
  // char* vector_file = argv[1];
  // hardcode the vector file.

  const char* word_vectors_file = "word_vectors.txt";
  const char* common_words_file = "common_words.txt";
 
  char* word_a = argv[2];
  char* word_b = argv[3];
  readVectors(word_vectors_file);
  cosineDistance(word_a, word_b);
}

