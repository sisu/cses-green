#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;
void end(bool success) {
	cout<<success<<'\n';
	exit(0);
}
int main(int argc, char* argv[]) {
	ifstream output(argv[1]);
	ifstream correct(argv[2]);
	string o,c;
	while(output>>o, correct>>c) {
		if (c!=o) end(0);
	}
	if (output) end(0);
	end(1);
}
