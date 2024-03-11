#include <iostream>
#include <string>
#include "./h/png.h"


int main(){
	
    std::string filepath = "./images/red_png_small.png";

	PNG image ( filepath );
   
    unsigned long h = image.height();
    unsigned long w = image.width();

    std::cout << w << "\n" << h << "\n";

	return 0;
};