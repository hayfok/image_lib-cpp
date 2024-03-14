#include <iostream>
#include <string>
#include "./h/png.h"


int main(){
	
    std::string filepath        = "./images/minion_loss.png";
    std::string new_filepath    = "./images/orange_copy.png";

	PNG image ( filepath );
   
    image.print_ihdr();
    //image.print_pixels();
    image.write_file( new_filepath );
    

	return 0;
};