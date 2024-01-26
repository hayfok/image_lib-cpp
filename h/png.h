// #pragma once


//              ===             global 

struct CRC_TABLE {
    std::vector<unsigned long> t { };
    unsigned long c { };
    CRC_TABLE() {
        for(int n { 0xFFFFFF }; n > 0; --n){
            c = (unsigned long) n;
            for(int k { }; k < 8; ++k){
                // AUTODIN II polynomial
                if (c & 1) c = 0xedb88320L ^ (c >> 1); // XOR
                else c = c >> 1;
            }
            t.push_back(c);
        }
    };


} crc_table { };

// look into ramifications 
// operational order: cast to unsigned int, bitshift right by x, bitwise OR (top -> down)
// 32 bit endian conversion
unsigned int little_to_big_endian(std::vector<char>& b){
    return (unsigned int)b[3] | (unsigned int)b[2]<<8 | (unsigned int)b[1]<<16 | (unsigned int)b[0]<<24; 
};

// "Please don't implement your own when you can use the zlib library instead." - fuck off
bool crc_32(){

    return true;
};

//              ===             global 





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
            png_width   = little_to_big_endian(endian_buff);    endian_buff.clear();    // clear buffer
            for(int i { }; i < 4; ++i){ endian_buff.push_back(file.get()); --length; }; // offsets file by 4 bytes
            png_height  = little_to_big_endian(endian_buff);    endian_buff.clear();    // clear buffer


            // all remaining ihdr bytes are size 1
            b_depth          = (char)file.get(); --length;
            color_type       = (char)file.get(); --length;
            comp_method      = (char)file.get(); --length;
            filter_method    = (char)file.get(); --length;
            interlace_method = (char)file.get(); --length;

            
            if(!crc_32()) throw;
        
        };

        std::vector<char> header { };
        unsigned int png_width { };     // little endian
        unsigned int png_height { };    // little endian
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
        PNG(){};
        PNG(std::ifstream& file) {
        
            //std::ifstream file { filepath, std::ios_base::binary | std::ios_base::in }; 
            std::pair<unsigned long, unsigned long> pair_id_size { };

            //if(!file.is_open())
	        //{ std::cerr << "Error opening: " << filepath << "\n"; throw; };

            //validate_header(file); // offsets file by 8 bytes
            
            while(!file.eof()){
                // file position guaranteed to be at beginning of chunk

                // get chunk len and chunk 4 byte identifier
                // offsets file by 8 bytes
                pair_id_size = identify_chunk(file);  

                switch(pair_id_size.first){
                    case 295:
                        ihdr.make_ihdr(file, pair_id_size.second); 
                };

                // early break while i work on this
                break;

            };
        };

        ~PNG(){};

        // user functions
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
            if(PNG_SIG != head_buff){ std::cout << "Bad PNG" << "\n"; throw; };

        };
        

        std::pair<unsigned long, unsigned long> identify_chunk(std::ifstream& file){
           
            unsigned long chunk_size { };
            for(int i { }; i < 4; ++i){ chunk_size += file.get(); }; // get 4 byte chunk size 
            // ofset is at byte 12

            unsigned long chunk_id { };
            for(int i { }; i < 4; ++i){ chunk_id += file.get(); };  // get 4 byte chunk id

            std::pair<unsigned long, unsigned long> pair_id_size{ chunk_id, chunk_size };
            return pair_id_size;
          
        };

                
};

//              ===             encoding classes





