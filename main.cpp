#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>


class Image 
{
	protected:
		std::vector<char> PNG_SIG  
        { 
			'\x89', 
			'\x50',
			'\x4e', 
			'\x47', 
			'\x0d', 
			'\x0a', 
			'\x1a', 
			'\x0a' 
		}; 
		std::vector<char> JPEG_SIG 
        { 
			'\xff', 
			'\xd8', 
			'\xff', 
			'\xe0', 
			'\x00', 
			'\x10', 
			'\x4a', 
			'\x46' 
		};
    
	public:

        Image()
        { };

        Image(std::string &file_path)
        {
            std::ifstream file { file_path, std::ios_base::binary | std::ios_base::in }; 

            if(!file.is_open())
	        { std::cerr << "Error opening: " << file_path << "\n"; throw; };

            create_internal_object(file);

        };

        void PrintFormat()
        {
            switch(*image_state)
            {
                case 1: { std::cout << "PNG" << "\n";  break; }
                case 2: { std::cout << "JPEG" << "\n"; break; }
            }
        }

        ~Image(){ std::cout << "An Image Object Destructed \n"; };

    private:

        // update public member function PrintFormat when adding or removing enums
        enum ImageState 
        {
            state_None,
            state_Png,
            state_Jpeg
        };

        ImageState* image_state { };

        void create_internal_object(std::ifstream &file)
        {

            std::vector<char> sig_buff (8);

            for(int i { }; i < sig_buff.size(); ++i)
            { sig_buff[i] = file.get(); };

            if(sig_buff == PNG_SIG) 
                { 
                PNG png { file };
                *image_state = state_Png;
                };
            
        };
        
    
   

};

class PNG :  protected Image {
    friend class Image;
    protected:
        PNG(){};
        PNG(std::ifstream &file){};
        ~PNG(){};
    public:
        
};


int main(){
	
    std::string file_path = "./images/red.png";

	Image image ( file_path );

    image.PrintFormat();
	
	return 0;
};