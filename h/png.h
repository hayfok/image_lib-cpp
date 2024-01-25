// #include <iostream>
// #include <string>
// #include <fstream>
// #include <vector>
// #include <utility>

//              ===             global functions

// look into ramifications 
        unsigned long static little_to_big_endian(std::vector<char>& endian_buff){
            return (int)endian_buff[3] | (int)endian_buff[2]<<8 | (int)endian_buff[1]<<16 | (int)endian_buff[0]<<24; 
        };

//              ===             global functions


//              ===             forward declaring class chunks

// 73 72 68 82
class IHDR {
    public:
        IHDR(){};
        void make_ihdr(std::ifstream& file, unsigned long length){
            // offset of file will be at byte 16
            // chunk size will be 8 less (size and id have been read)

            std::vector<char> endian_buff { };

            // probably not the best way to do this
            for(int i { }; i < 4; ++i){ endian_buff.push_back(file.get()); --length; }; // offsets file by 4 bytes
            png_width   = little_to_big_endian(endian_buff);    endian_buff.clear();
            for(int i { }; i < 4; ++i){ endian_buff.push_back(file.get()); --length; }; // offsets file by 4 bytes
            png_height  = little_to_big_endian(endian_buff);    endian_buff.clear();


            // all remaining ihdr bytes are size 1
            b_depth          = (int)file.get(); --length;
            color_type       = (int)file.get(); --length;
            comp_method      = (int)file.get(); --length;
            filter_method    = (int)file.get(); --length;
            interlace_method = (int)file.get(); --length;
        }
        std::vector<char> header { };
        unsigned long png_width { };    // little endian
        unsigned long png_height { };   // little endian
        char b_depth { };               // valid values 1, 2, 4, 8, 16
        char color_type { };            // valid values 0, 2, 3, 4, 6
        char comp_method { };           // should be 0 for standard
        char filter_method { };         // should be 0 for standard
        char interlace_method { };      // 0 == no interlace, 1 == Adam7
} ihdr { };

//              ===             forward declaring class chunks


//              ===             encoding classes

class PNG {
    private:
       
        
    public:
        IHDR ihdr;

        PNG(std::string &filepath) {
        
            std::ifstream file { filepath, std::ios_base::binary | std::ios_base::in }; 
            std::pair<unsigned long, unsigned long> pair_cid_csize { };

            if(!file.is_open())
	        { std::cerr << "Error opening: " << filepath << "\n"; throw; };

            validate_header(file);                      // offsets file by 8 bytes
            pair_cid_csize = identify_chunk(file);      // offsets file by 8 bytes

            switch(pair_cid_csize.first){
                case 295:
                    ihdr.make_ihdr(file, pair_cid_csize.second);
                    // how can i create an object of type IHDR in a scope
                    // accessable to member functions here
            }
        };

        ~PNG(){};

        

        unsigned long width(){ return ihdr.png_width ; };
        unsigned long height(){ return ihdr.png_height; };
        
    protected:

        std::vector<char> PNG_SIG  
        { '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' }; 

        void validate_header(std::ifstream &file){

            std::vector<char> head_buff { };
            // read first 8 bytes into header buffer
            for( int i { }; i < 8; ++i ) head_buff.push_back(file.get()); 

            // validate header as valid png
            if(this->PNG_SIG != head_buff){ std::cout << "Bad PNG" << "\n"; throw; };

        };

        std::pair<unsigned long, unsigned long> identify_chunk(std::ifstream& file){
           
            unsigned long chunk_size { };
            for(int i { }; i < 4; ++i){ chunk_size += file.get(); }; // get 4 byte chunk size 
            // ofset is at byte 12

            unsigned long chunk_id { };
            for(int i { }; i < 4; ++i){ chunk_id += file.get(); };  // get 4 byte chunk id

            std::pair<unsigned long, unsigned long> pair_cid_csize{ chunk_id, chunk_size };
            return pair_cid_csize;
          
        };

                
};

//              ===             encoding classes





