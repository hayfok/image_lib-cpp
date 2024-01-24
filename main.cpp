#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <png.h>


int main(){
	
    std::string filepath = "./images/red.png";

    //Image image ( filepath );

    //image.PrintFormat();
	PNG image ( filepath );
   
    //std::cout << image.height();
	return 0;
};