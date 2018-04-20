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

float cosineDistance(std::string a, std::string b) {
  std::vector<float> va = word_vectors.at(a);
  std::vector<float> vb = word_vectors.at(b);

  float dot_product = std::inner_product(std::begin(va), std::end(va), std::begin(vb), 0.0);

  for (std::vector<float>::iterator vai = va.begin(); vai != va.end(); ++vai) {
  }

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
  char* vector_file = argv[1];
  char* word_a = argv[2];
  char* word_b = argv[3];
  readVectors(vector_file);
  cosineDistance(word_a, word_b);
}

