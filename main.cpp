#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include "./h/png.h"


int main(){
	
    std::string filepath = "./images/red.png";

	PNG image ( filepath );
   
    unsigned long h = image.height();
    unsigned long w = image.width();

    std::cout << w << "\n" << h << "\n";

	return 0;
};