#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>

using namespace std;

int main(){
	string file, sline;
	stringstream line;
	int count=0, a, b;
	double big=0, small=0;

	cout<<"Cascades file path?"<<endl;
	cin>>file;

	ifstream fd(file.c_str());
	if(fd.is_open()){
		while(getline(fd, sline)){
			line.clear();
			line.str(sline);
			line.ignore(128, ' ');
			line.ignore(128, ' ');
			line.ignore(128, ' ');
			line.ignore(128, ' ');
			line>>a;
			line>>b;
			if(a < b){
				small+=a;
				big+=b;
				count+=1;
			}else{
				small+=b;
				big+=a;
				count+=1;
			}
		}
		fd.close();
		cout<<"Average rect dims: "<<endl;
		printf ("Min: %g Max: %g \n", small/(double)count, big/(double)count);
	}else{
		cout<<"ERROR: Can't open for read"<<endl<<endl;
		return -1;
	}
}