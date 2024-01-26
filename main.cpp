#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
// #include "./h/png.h"
#include "./h/image.h"

int main(){
	
    std::string filepath = "./images/red.png";

	//PNG png( filepath );
    Image image( filepath );

   
    // unsigned long h = png.height();
    // unsigned long w = png.width();

    // std::cout << w << "\n" << h << "\n";

    //std::vector<unsigned int> v = image.crc_table;

    image.print_type();

	return 0;
};