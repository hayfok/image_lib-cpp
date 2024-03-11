/* notes & documentation
*
* deflate algo              - https://www.ietf.org/rfc/rfc1951.txt
* png white paper           - https://www.w3.org/TR/2003/REC-PNG-20031110/
* png home page             - http://www.libpng.org/pub/png/
* inflate algo example      - https://github.com/madler/zlib/blob/master/contrib/puff/puff.c
* inflate annotated code    - http://zlib.net/zlib_how.html
*
*/

#include "zlib.h"
#include "zconf.h"
#include <vector>
#include <fstream>
#include <cassert>


#define IHDR 295;
#define sRGB 334;
#define gAMA 310;
#define pHYs 388;
#define IDAT 290;
#define IEND 288;

// recommended space allocation for inflate()
#define CHUNK 16384

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

// for debugging purposes, not intended to be used
void print_byte(char &c){ std::cout << c << "\n"; };

// 32 bit crc validation
// all bytes after the length up to the crc bytes
void crc_32(std::ifstream& file)
{
    // impliment the zlib crc32 here
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

        struct Pixel
        {
            unsigned char R;
            unsigned char G;
            unsigned char B;
            unsigned char A;
        };

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
        std::vector< std::vector<unsigned char> > image_matrix;
        std::vector<unsigned char> compresed_data { };
        std::vector<unsigned char> out { };
        std::vector<unsigned char> b { };
        //std::vector< std::vector<Pixel> > pixels { };



    // IHDR
    // 0x49 0x48 0x44 0x52
        int read_ihdr(std::ifstream& file, unsigned long length)
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

            return 0;
        };

    // SRGB
    // 0x73 0x52 0x47 0x42
        int read_sRGB(std::ifstream &file, unsigned long &length)
        {
            this->rendering_intent = file.get(); --length;

            if(length != 0) { std::cout << "file sync error: srgb \n"; throw; };

            crc_32(file); // offsets files by 4 bytes

            return 0;
        };

    // GAMA
    // 0x67 0x41 0x4D 0x41
        int read_gAMA(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->gamma = file.get(); --length; };

            if(length != 0) { std::cout << "file sync error: gama \n"; throw; };

            crc_32(file); // offsets files by 4 bytes

            return 0;
        };

        void sample_gamma(){
            int sample { };
            sample = sample / (2^b_depth - 1);
        };

    // PHYS
    // 0x70 0x48 0x59 0x73
        int read_pHYs(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_x = file.get(); --length; };
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_y = file.get(); --length; };

            this->unit_specificer = file.get(); --length;

            if(length != 0) { std::cout << "file sync error: phys \n"; throw; };

            crc_32(file); // offsets files by 4 bytes

            return 0;
        };

    // IDAT
    // 0x49 0x44 0x54 0x78
        int read_IDAT(std::ifstream& file, unsigned long& length)
        {
            unsigned int i;
            int err;
            //unsigned long compressed_data_length = length;

            for(i=0;i<length;++i) this->compresed_data.push_back(file.get());
        
            err = inf(compresed_data, length, out);
            if(err) return 1;


            //for(int i=0;i<b.size();++i) std::cout << (int)b[i] << " ";
            err = construct_image_matrix();
            if(err) return 1;

            for(int w=0;w<this->png_width;++w)
            {
                for(int h=0;h<this->png_height;++h)
                {
                    std::cout << 
                    (int)image_matrix[w][h] << " " <<
                    (int)image_matrix[w][h] << " " <<
                    (int)image_matrix[w][h] << " " <<
                    (int)image_matrix[w][h] << " \n ";
                }
            }

            crc_32(file);

            return 0;
        };
    
    // IEND
    // 0x49 0x45 0x4E 0x44
        int read_IEND(std::ifstream& file, unsigned long& length)
        {
            crc_32(file);
            // do nothing for now

            return 0;
        };


    
    /* zlib inflation of compressed image data
    *
    * http://zlib.net/zlib_how.html
    * 
    */
        int inf(std::vector<unsigned char>& data, unsigned long& len, std::vector<unsigned char>& out)
        {

            // unsigned char buffer[CHUNK];
            out.resize(this->png_height * this->png_width * 4 + this->png_height);
            //out.push_back('\0');
            //out.reserve(CHUNK);
           
            int codes               { }; // zlib return codes
            unsigned int have       { }; // number of bytes read
            int r, c, i, j;
            z_stream stream;
         
            stream.zalloc           = Z_NULL;
            stream.zfree            = Z_NULL;
            stream.opaque           = Z_NULL;
            stream.avail_in         = 0;
            stream.next_in          = Z_NULL;

            codes = inflateInit(&stream);
            assert(codes == Z_OK);
            int q = 0;
            unsigned long f = 0;


            do
            {
                stream.avail_in     = CHUNK;
                stream.next_in      = reinterpret_cast<unsigned char* >(data.data());

                do
                {
                    stream.avail_out    = CHUNK;
                    stream.next_out     = reinterpret_cast<unsigned char* >(out.data());

                    codes = inflate(&stream, Z_NO_FLUSH);
                    if(codes == Z_BUF_ERROR) return 0;
                    switch(codes)
                    {
                        case Z_NEED_DICT:
                            std::cout << Z_DATA_ERROR << "\n dict \n"; 
                            return 1;
                        case Z_DATA_ERROR:
                            std::cout << Z_DATA_ERROR << "\n"; 
                            return 1;
                        case Z_MEM_ERROR:
                            inflateEnd(&stream);
                            std::cout << "MEM ERR \n";
                            return 1;
                        case Z_STREAM_ERROR:
                            std::cout << "Z_STREAM_ERROR \n"; 
                            return 1;
                    };

                    have = CHUNK - stream.avail_out;

                    for(int i=0;i<have;++i){ b.push_back(out[i]); };

                    out.clear();
                  
                } while (codes != Z_STREAM_END);
            } while(stream.avail_out == 0);
        
            // for(int k =0;k<have;++k) std::cout << (unsigned int)out[k] << " ";

            return 0;
           
        };

        int construct_image_matrix()
        {
         
            unsigned int l = b.capacity();
            std::vector<unsigned char> pixels { };

            unsigned long a = 0;

            unsigned long h, w, i;

            // Pixel p;

            h = w = i = 0;
       
            for(h=0;h<this->png_height;++h)
            {
                for(w=0;w<this->png_width;++w)
                {
                    unsigned long pos = 1;
                    for(i=0;i<4;++i)
                    {
                        pixels.push_back(b[pos]);
                        ++pos;
                    };

                };
                // std::cout << ++a << "\n";
                image_matrix.push_back(pixels);
                
            };

            return 0;

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
        * the code after this warning will run with undefined behavior.
        * 
        */
            if(id != 295) std::cout << 
            "WARNING! PNG header not followed by IDHR chunk" <<
            " - We're decoding a questionably encoded PNG \n";

            Chunks.read_ihdr(file, len);

        /* main file decoder
        *
        * read all bytes from the file until the eof is reached.
        * each case is the sum of individual ascii bytes parsed from identity_chunk().
        * 
        */
            while(!file.eof())
            {
                len = id = 0;

                identify_chunk(file, len, id); // offsets file reader by 8 bytes on every call

                // it is a guarantee that the chunk is offset up to the first byte after the header
                switch(id)
                {
                    case 334: { int err = Chunks.read_sRGB(file, len); if(!err) break; throw; }
                    case 310: { int err = Chunks.read_gAMA(file, len); if(!err) break; throw; }
                    case 388: { int err = Chunks.read_pHYs(file, len); if(!err) break; throw; }
                    case 290: { int err = Chunks.read_IDAT(file, len); if(!err) break; throw; }
                    case 288: { int err = Chunks.read_IEND(file, len); if(!err) break; throw; }
                    default: std::cout << "did not catch a chunk id \n";
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
    * this function will throw if the first 8 bytes of the file differ from PNG_SIG.
    * this function is intended to only be ran once by the decoder.
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
    * the length is converted to big endian and needed for decoding.
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



