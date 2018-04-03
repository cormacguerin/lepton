#include <iostream>
#include "dtl/dtl.hpp"

using namespace std;

void dtltest(char* a, char* b) {
	typedef char elem;
	typedef char ses;
	typedef std::string sequence;
	sequence A(a);
	sequence B(b);
	dtl::Diff< elem, sequence > d(A, B);
	d.compose();
	d.composeUnifiedHunks();
	d.printUnifiedFormat();
	cout << "editDistance:" << d.getEditDistance() << endl;

	// Longest Common Subsequence
    vector< elem > lcs_v = d.getLcsVec();
	dtl::Ses< ses > s = d.getSes();
	sequence lcs_s(lcs_v.begin(), lcs_v.end());
	cout << "LCS:" << lcs_s << endl;
	cout << "SES" << endl;

	//d.composeUnifiedHunks();
	string s1(A);
	string s2 = d.patch(s1);
	cout << "s " << s << endl;
	cout << "s1 " << s1 << endl;
	cout << "s2 " << s2 << endl;
}

int main(int argc, char** argv)
{
	dtltest(argv[1], argv[2]);
}

