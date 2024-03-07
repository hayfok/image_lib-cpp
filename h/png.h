/* notes & documentation
*
* deflate algo              - https://www.ietf.org/rfc/rfc1951.txt
* png white paper           - https://www.w3.org/TR/2003/REC-PNG-20031110/
* png home page             - http://www.libpng.org/pub/png/
* inflate algo example      - https://github.com/madler/zlib/blob/master/contrib/puff/puff.c
* inflate annotated code    - http://zlib.net/zlib_how.html
*/

#include "zlib.h"
#include "zconf.h"
#include <vector>
#include <fstream>
/* recommended per the zlib documentation
*
* todo: look into why this is important before implimenting
*
*/
// #if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
// #  include <fcntl.h>
// #  include <io.h>
// #  define SET_BINARY_MODE(file) _setmode(_fileno(file), O_BINARY)
// #else
// #  define SET_BINARY_MODE(file)
// #endif


struct CRC_TABLE 
{
    std::vector<unsigned long> t;
    unsigned long c;

    CRC_TABLE() 
    {
        for(int n { 256 }; n > 0; --n)
        {
            c = (unsigned long) n;
            for(int k { }; k < 8; ++k)
            {
                // AUTODIN II polynomial
                // 0xedb88320L
                (c & 1) ? c = 0x04C11DBL ^ (c >> 1) : c = c >> 1;
            };
            t.push_back(c);
        };
    };

} crc_table;

// for debugging purposes, not intended to be used
void print_byte(char &c){ std::cout << c << "\n"; };

// 32 bit crc validation
// all bytes after the length up to the crc bytes
void crc_32(std::ifstream &file)
{
    // doing nothing for now, need to impliment this
    for(int i { 4 }; i > 0; --i){ file.get(); };
};



/* endian swapper
*
* accepts a byte vector and returns a 4 byte numeric representation of the big endian representation.
* this function does not yet consider the endianness of the system it's running on and is hacky for now
*
*/
unsigned long little_to_big_endian(std::vector<int>& endian_buff)
{
    return (int)endian_buff[3] | (int)endian_buff[2]<<8 | (int)endian_buff[1]<<16 | (int)endian_buff[0]<<24;
};



class Chunks 
{

    public:
    // IHDR values
        unsigned long png_width { };             // little endian
        unsigned long png_height { };            // little endian
        unsigned char b_depth { };               // valid values 1, 2, 4, 8, 16
        unsigned char color_type { };            // valid values 0, 2, 3, 4, 6
        unsigned char comp_method { };           // should be 0 for standard
        unsigned char filter_method { };         // should be 0 for standard
        unsigned char interlace_method { };      // 0 == no interlace, 1 == Adam7
    // sRGB values
        // 0 == Perceptual
        // 1 == Relative Colorimetric
        // 2 == Saturation
        // 3 == Absolute Colorimetric
        char rendering_intent { };
    // gAMA values
        unsigned long gamma { };
    // pHYs values
        unsigned long pixel_per_unit_x { };
        unsigned long pixel_per_unit_y { };

        // 0 == unknown
        // 1 == meter
        unsigned char unit_specificer { };
    // IDAT values
        std::vector<char> compresed_data { };
        std::vector<char> uncompresed_data { };

    // IHDR
    // 0x49 0x48 0x44 0x52
        void make_ihdr(std::ifstream& file, unsigned long length)
        {
          
            std::vector<int> endian_buff { };
            std::vector<char> header { };

            // get the width
            for(int i = 0; i < 4; ++i)
            { 
                endian_buff.push_back(file.get()); 
                --length; 
            };
            this->png_width = little_to_big_endian(endian_buff);
            endian_buff.clear();

            // get the height
            for(int i = 0; i < 4; ++i)
            { 
                endian_buff.push_back(file.get()); 
                --length; 
            }; 
            this->png_height = little_to_big_endian(endian_buff);
            endian_buff.clear();

            // all remaining ihdr bytes are size 1
            this->b_depth          = (int)file.get(); --length;
            this->color_type       = (int)file.get(); --length;
            this->comp_method      = (int)file.get(); --length;
            this->filter_method    = (int)file.get(); --length;
            this->interlace_method = (int)file.get(); --length;

            if(length != 0) { std::cout << "file sync error: ihdr \n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

    

    // SRGB
    // 0x73 0x52 0x47 0x42
        void make_sRGB(std::ifstream &file, unsigned long &length)
        {
            this->rendering_intent = file.get(); --length;

            if(length != 0) { std::cout << "file sync error: srgb \n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

    // GAMA
    // 0x67 0x41 0x4D 0x41
        void make_gAMA(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->gamma = file.get(); --length; };

            if(length != 0) { std::cout << "file sync error: gama \n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

        void sample_gamma(){
            int sample { };
            sample = sample / (2^b_depth - 1);
        };

    // PHYS
    // 0x70 0x48 0x59 0x73
        void make_pHYs(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_x = file.get(); --length; };
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_y = file.get(); --length; };

            this->unit_specificer = file.get(); --length;

            if(length != 0) { std::cout << "file sync error: phys \n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

    // IDAT
    // 0x49 0x44 0x54 0x78
        void make_IDAT(std::ifstream& file, unsigned long& length)
        {
            unsigned long compressed_data_length = length;

            while (length != 0)
            {
               this->compresed_data.push_back(file.get());
                --length; 
            };

            inf(compresed_data, compressed_data_length, uncompresed_data);
        };
    
    // IEND
    // 0x49 0x45 0x4E 0x44
        void make_IEND(std::ifstream& file, unsigned long& length)
        {
            // do nothing for now
        };

    /* zlib inflation of compressed image data
    *
    * http://zlib.net/zlib_how.html
    * 
    */
        void inf(std::vector<char>& data, unsigned long& len, std::vector<char>& uncompressed_data)
        {
            #define CHUNK 16384

            int codes               { }; // zlib return codes
            unsigned long have      { }; // data returned
            z_stream stream;
            std::vector<char> out   { };

            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = data[0];
            stream.next_in = Z_NULL;
            codes = inflateInit(&stream);
            if(codes != Z_OK) std::cout << codes << "\n";

            while(codes != Z_STREAM_END)
            {
                codes = inflate(&stream, Z_NO_FLUSH);
                switch(codes)
                {
                    case Z_NEED_DICT:
                        std::cout << Z_DATA_ERROR << "\n dict \n"; break;
                    case Z_DATA_ERROR:
                        std::cout << Z_DATA_ERROR << "\n"; break;
                    case Z_MEM_ERROR:
                        inflateEnd(&stream);
                        std::cout << "MEM ERR \n";
                        break;
                };

                have = stream.avail_out;
                uncompresed_data.push_back(have);
            }
            
           

        };

} Chunks { };


class PNG 
{
    private:

        void read_bytes(std::string &filepath)
        {
            std::ifstream file { filepath, std::ios_base::binary | std::ios_base::in }; 
            unsigned long len, id;

            len = id = 0;
            
            if(!file.is_open()){ std::cerr << "Error opening: " << filepath << "\n"; throw; };

            validate_header(file); // offsets file reader by 8 bytes, only called once

            identify_chunk(file, len, id); // offsets file reader by 8 bytes on every call

        /* ihdr is the chunk that must follow the png header per the specification
        * 
        * we will issue a warning if this is not the case instead of throwing.
        * if this warning is valid the decoder will operate with undefined behavior
        * 
        */
            if(id != 295) std::cout << 
            "WARNING! PNG header not followed by IDHR chunk" <<
            " - We're decoding a questionably encoded PNG \n";

            Chunks.make_ihdr(file, len);

        /* main file decoder
        *
        * read all bytes from the file until the eof is reached.
        * each case is the sum of individual ascii bytes parsed from the identity_chunk() function.
        * 
        */
            while(!file.eof())
            {
                len = id = 0;

                identify_chunk(file, len, id); // offsets file reader by 8 bytes on every call

                // it is a guarantee that the chunk is offset up to the first byte after the header
                switch(id)
                {
                    case 334: Chunks.make_sRGB(file, len); break;
                    case 310: Chunks.make_gAMA(file, len); break;
                    case 388: Chunks.make_pHYs(file, len); break;
                    case 290: Chunks.make_IDAT(file, len); break;  
                    case 288: Chunks.make_IEND(file, len); break;
                    default: std::cout << "did not catch a chunk id \n"; throw;
                };
            };

            
        };
       
    public:

        PNG(std::string &filepath) { read_bytes(filepath); };

        ~PNG(){};

        unsigned long width() { return Chunks.png_width ; };
        unsigned long height(){ return Chunks.png_height; };
        
    protected:

        std::vector<char> PNG_SIG  
        { 
            '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' 
        }; 
    
    /* function to identify the png header
    *
    * this function will throw if the first 8 bytes of the file differ from the png header vec.
    * this function is intended to only be ran once by the decoder
    * 
    */
        void validate_header(std::ifstream &file)
        {
            std::vector<char> head_buff;

            for(int i { }; i < 8; ++i) head_buff.push_back(file.get());               // read first 8 bytes into header buffer
            if(this->PNG_SIG != head_buff){ std::cout << "Bad PNG" << "\n"; throw; }; // validate header as valid png
        };

    /* function to identify chunks. 
    *
    * the id variable is the sum of all individual ascii bytes.
    * the length is converted to big endian and needed for decoding
    *
    */
        void identify_chunk(std::ifstream& file, unsigned long& len, unsigned long& id)
        {
            std::vector<int> b { };
           
            for(int i = 0; i < 4; ++i) b.push_back(file.get()); // get 4 byte chunk size 

            len = little_to_big_endian(b);

            for(int i = 0; i < 4; ++i) id  += file.get();       // get 4 byte chunk id

            b.clear();
        };           
};



// 18783



