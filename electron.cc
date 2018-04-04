#include <iostream>
#include "dtl/dtl.hpp"
#include "dtl/variables.hpp"

using namespace std;
using dtl::Diff;


void dtltest(char* a, char* b) {
  typedef char elem;
  typedef char ses;
  typedef std::string sequence;
  typedef std::ostream stream;
  typedef std::vector<std::string> uniHunkVec;

  typedef char elem;
  typedef string sequence;

  sequence A(a);
  sequence B(b);

  Diff<elem, sequence > d(A, B);
  d.compose();
  d.composeUnifiedHunks();
  dtl::stemdiff s = d.getUnifiedFormat();
  cout << "add " << s.add << endl;
  cout << "common " << s.common << endl;
  cout << "del " << s.del << endl;

  // editDistance
  cout << "editDistance:" << d.getEditDistance() << endl;

  // Longest Common Subsequence
  vector<elem> lcs_v = d.getLcsVec();
  sequence     lcs_s(lcs_v.begin(), lcs_v.end());
  cout << "LCS:" << lcs_s << endl;

  //d.composeUnifiedHunks();
  string s1(A);
  string s2 = d.patch(s1);
}

int main(int argc, char** argv)
{
  dtltest(argv[1], argv[2]);
}

