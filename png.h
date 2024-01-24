#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <utility>


// 73 72 68 82
class IHDR : protected PNG {
    public:
        // opting to not put this logic in the constructor
        IHDR(std::ifstream& file, unsigned long chunk_size){ construct_idhr(file, chunk_size); };
        ~IHDR(){};

        std::vector<char> header { };
        unsigned long png_width { };    // little endian
        unsigned long png_height { };   // little endian
        char b_depth { };               // valid values 1, 2, 4, 8, 16
        char color_type { };            // valid values 0, 2, 3, 4, 6
        char comp_method { };           // should be 0 for standard
        char filter_method { };         // should be 0 for standard
        char interlace_method { };      // 0 == no interlace, 1 == Adam7

        void construct_idhr (std::ifstream& file, unsigned long chunk_size){
            // offset of file will be at byte 16
            // chunk size will be 8 less (size and id have been read)

            // need to convert to big endian
            // this function will increase the file offset by 4 bytes
            png_width = little_to_big_endian(file, chunk_size);
            png_height = little_to_big_endian(file, chunk_size);

            // the rest of the bytes for this header will all be one byte each
            b_depth = (int)file.get(); --chunk_size;
            color_type = (int)file.get(); --chunk_size;
            comp_method = (int)file.get(); --chunk_size;
            filter_method = (int)file.get(); --chunk_size;
            interlace_method = (int)file.get(); --chunk_size;
            
            if(chunk_size == 0) { 
                // last 4 bytes (outside of chunk size) are the CRC 
                // write a function to confirm the CRC is correct
                return;
            }
            else { std::cout << "we didn't read all of the ihdr chunks for some reason" << "\n"; };
        };
};

class PNG {
    public:
        PNG(std::string &filepath){
        
            std::ifstream file { filepath, std::ios_base::binary | std::ios_base::in }; 

            if(!file.is_open())
	        { std::cerr << "Error opening: " << filepath << "\n"; throw; };

            std::pair<unsigned long, unsigned long> chunk_pair = read_bytes(file);

            switch(chunk_pair.first){
                case 295: // ihdr chunk
                    IHDR ihdr { file, chunk_pair.second };
            }

        

        };
        PNG(){};
        ~PNG(){};

        
        std::pair<unsigned long, unsigned long> read_bytes(std::ifstream &file){

            std::vector<char> head_buff { };
            // read first 8 bytes into header buffer
            for( int i { }; i < 8; ++i ) head_buff.push_back(file.get()); 

            // validate header as valid png
            if(this->PNG_SIG != head_buff){ std::cout << "Bad PNG" << "\n"; return; };

            // read through the rest of the file
            // offset is at byte 8
            while(!file.eof()){
               
                unsigned long chunk_size { };
                for(int i { }; i < 4; ++i){ chunk_size += file.get(); }; // get 4 byte chunk size 
                // ofset is at byte 12

                unsigned long chunk_id{ };
                for(int i { }; i < 4; ++i){ chunk_id += file.get(); };  // get 4 byte chunk id
                // ofset is at byte 16

                return std::pair<unsigned long, unsigned long>(chunk_id, chunk_size);
            }

        }



        protected:

            std::vector<char> PNG_SIG  
            { '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' }; 

            // look into ramifications of this on all possible architectures 
            unsigned int little_to_big_endian(std::ifstream& file, unsigned long& chunk_size){
                std::vector<char> endian_buff { };
                if(!endian_buff.empty()) endian_buff.clear();
                
                for(int i { }; i < 4; ++i){ endian_buff.push_back(file.get()); --chunk_size; };
                return (int)endian_buff[3] | (int)endian_buff[2]<<8 | (int)endian_buff[1]<<16 | (int)endian_buff[0]<<24; 
            };
};


