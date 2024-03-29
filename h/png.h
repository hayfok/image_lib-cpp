/* notes & documentation
*
* deflate algo              - https://www.ietf.org/rfc/rfc1951.txt
* png white paper           - https://www.w3.org/TR/2003/REC-PNG-20031110/
* png home page             - http://www.libpng.org/pub/png/
* inflate algo example      - https://github.com/madler/zlib/blob/master/contrib/puff/puff.c
* inflate annotated code    - http://zlib.net/zlib_how.html
* amazing crc doc           - http://www.ross.net/crc/download/crc_v3.txt
*
* this code is not appropriate for production
*/

#include "zlib.h"
#include "zconf.h"
//#include "crc32.h"
#include <vector>
#include <fstream>
#include <cassert>
#include <map>
#include <stdexcept>

 // recommended space allocation for inflate() efficiency
#define CHUNK 16384

/* recommended per the zlib documentation
*
*
*/
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) _setmode(_fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif


// 32 bit crc validation
// all bytes after the length up to the crc bytes
void crc_32(std::vector<unsigned char>& buffer)
{
    buffer.clear();
};


/* endian swapper
*
* accepts a byte vector and returns a 4 byte numeric representation of the big endian representation.
* this function does not yet consider the endianness of the system it's running on and is hacky for now
*
*/
unsigned long little_to_big_endian(std::vector<unsigned char>& endian_buff)
{
    return (int)endian_buff[3] | (int)endian_buff[2]<<8 | (int)endian_buff[1]<<16 | (int)endian_buff[0]<<24;
};



class Chunks 
{

    public:

        struct Pixel
        { unsigned char R, G, B, A; };

        std::map<std::string, unsigned char> chunk_flags 
        {
            { "sRGB", 0 },
            { "PLTE", 0 },
            { "gAMA", 0 },
            { "pHYs", 0 }
        };

    // IHDR values
        unsigned int png_width              = 0;            // little endian
        unsigned int png_height             = 0;            // little endian
        unsigned char b_depth               = 0;            // valid values 1, 2, 4, 8, 16
        unsigned char color_type            = 0;            // valid values 0, 2, 3, 4, 6
        unsigned char comp_method           = 0;            // should be 0 for standard
        unsigned char filter_method         = 0;            // should be 0 for standard
        unsigned char interlace_method      = 0;            // 0 == no interlace, 1 == Adam7

        // 1 == alpha channel
        // 0 == no alpha channel
        unsigned char alpha_flag            = 0; 
    // sRGB values
        // 0 == Perceptual
        // 1 == Relative Colorimetric
        // 2 == Saturation
        // 3 == Absolute Colorimetric
        unsigned char rendering_intent      = 0;
    // gAMA values
        unsigned long gamma                 = 0;
    // pHYs values
        unsigned long pixel_per_unit_x      = 0;
        unsigned long pixel_per_unit_y      = 0;
        // 0 == unknown
        // 1 == meter
        unsigned char unit_specificer       = 0;
    //PLTE values
        std::vector<unsigned char> palate                                   { };
        std::vector< std::vector<unsigned char> > indexed_matrix            { };
    // IDAT values
        std::vector< std::vector<Pixel> > truecolor_matrix                  { };
        std::vector<unsigned char> compresed_data                           { };
        std::vector<unsigned char> out                                      { };
        std::vector<unsigned char> b                                        { };
        unsigned long len                   = 0;
     
        std::vector<unsigned char> chunk_buffer                             { };

    void chunk_buffer_writer(std::ifstream& file, unsigned long& length)
    {
            int i = 0;
            for(i;i<length + 4;++i)
            {
                this->chunk_buffer.push_back(file.get());
                
            };

    };

    // IHDR
    // 0x49 0x48 0x44 0x52
        int read_ihdr(std::ifstream& file, unsigned long& length)
        {
            int i = 8;
            length -= 8;
            chunk_buffer_writer(file, length);

            std::vector<unsigned char> endian_buff { };
            std::vector<char> header { };

            // get the width
            for(i;i<=11;++i)
            { 
                endian_buff.push_back(this->chunk_buffer[i]); 
        
            };
            this->png_width = little_to_big_endian(endian_buff);
            endian_buff.clear();

            // get the height
            for(i=12;i<=15;++i)
            { 
                endian_buff.push_back(this->chunk_buffer[i]); 
            }; 
            this->png_height = little_to_big_endian(endian_buff);
            endian_buff.clear();

            // all remaining ihdr fields are one byte each
            this->b_depth          = this->chunk_buffer[16];
            this->color_type       = this->chunk_buffer[17];
            this->comp_method      = this->chunk_buffer[18];
            this->filter_method    = this->chunk_buffer[19];
            this->interlace_method = this->chunk_buffer[20];

            this->color_type == 4 || this->color_type == 6 ? this->alpha_flag = 0 : this->alpha_flag = 1;

            crc_32(this->chunk_buffer);

            return 0;
        };

    // SRGB
    // 0x73 0x52 0x47 0x42
        int read_sRGB(std::ifstream& file, unsigned long& length)
        {
    
            this->chunk_flags["sRGB"] = 1;
            
            
            chunk_buffer_writer(file, length);

            this->rendering_intent = this->chunk_buffer[8];

            crc_32(this->chunk_buffer);

            return 0;

        };

    // PLTE
    // 0x50 0x4C 0x54 0x45
        int read_PLTE(std::ifstream& file, unsigned long& length)
        {

            int i = 8;
            this->chunk_flags["PLTE"] = 1;

            chunk_buffer_writer(file, length);

            if(!length % 3) { std::cout << "PLTE not divisible by 3. \n"; return 1; };

            this->palate.resize(length / 3);

            for(i;i<length;++i)
            {
                if(i % 3 == 0) palate.push_back(this->chunk_buffer[i]);
            };

            crc_32(this->chunk_buffer); // offsets files by 4 bytes

            return 0;
           
        };

    // GAMA
    // 0x67 0x41 0x4D 0x41
        int read_gAMA(std::ifstream& file, unsigned long& length)
        {
            int i = 8;
            chunk_flags["gAMA"] = 1;

           
            chunk_buffer_writer(file, length);

            for(i;i<12;++i)
            { 
                this->gamma += this->chunk_buffer[i];
            };

            crc_32(this->chunk_buffer); // offsets files by 4 bytes

            return 0;

        };

        void sample_gamma(){
            int sample { };
            sample = sample / (2^b_depth - 1);
        };

    // PHYS
    // 0x70 0x48 0x59 0x73
        int read_pHYs(std::ifstream& file, unsigned long& length)
        {

            int i = 8;
            chunk_flags["pHYs"] = 1;

            chunk_buffer_writer(file, length);

            for(i;i<12;++i)
            { 
                this->pixel_per_unit_x += this->chunk_buffer[i];
            };

            for(i;i<16;++i)
            { 
                this->pixel_per_unit_y += this->chunk_buffer[i];
            };

            this->unit_specificer = this->chunk_buffer[17];


            crc_32(this->chunk_buffer); // offsets files by 4 bytes

            return 0;

        };

    // IDAT
    // 0x49 0x44 0x54 0x78
        int read_IDAT(std::ifstream& file, unsigned long& length)
        {
            int i = 8;
            unsigned int err;

            unsigned long len = length;

            chunk_buffer_writer(file, length);
         
            for(i;i<length;++i)
            {
                this->compresed_data.push_back(this->chunk_buffer[i]);
            };
        
            err = inf(compresed_data, length, out);
            if(err) return 1;

            err = construct_image_matrix();
            if(err) return 1;

            crc_32(this->chunk_buffer);

            return 0;
        };
    
    // IEND
    // 0x49 0x45 0x4E 0x44
        int read_IEND(std::ifstream& file, unsigned long& length)
        {
            crc_32(this->chunk_buffer);

            return 0;
        };

    /* zlib inflation of compressed image data
    *
    * http://zlib.net/zlib_how.html
    * 
    */
        int inf(std::vector<unsigned char>& data, unsigned long& len, std::vector<unsigned char>& out)
        {

            out.resize(CHUNK);
          
            int codes, i;                // zlib return codes            
            codes = i               = 0; 
            unsigned int have       = 0; // number of bytes read
      
            z_stream stream;
         
            stream.zalloc           = Z_NULL;
            stream.zfree            = Z_NULL;
            stream.opaque           = Z_NULL;
            stream.avail_in         = 0;
            stream.next_in          = Z_NULL;

            codes = inflateInit(&stream);       
            assert(codes == Z_OK);
        
            do
            {
                stream.avail_in     = CHUNK;
                stream.next_in      = reinterpret_cast<unsigned char* >(data.data());

                do
                {
                    stream.avail_out    = CHUNK;
                    stream.next_out     = reinterpret_cast<unsigned char* >(out.data());

                    codes = inflate(&stream, Z_NO_FLUSH);
                    if(codes == Z_BUF_ERROR) { inflateEnd(&stream); return 0; } // not sure if this is a good idea or not

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

                    for(i=0;i<have;++i){ b.push_back(out[i]); };

                    out.clear();
                  
                } while (codes != Z_STREAM_END);
            } while(stream.avail_out == 0);
        
            return 0;
           
        };

    // 
        void RGBA(
            Pixel& p, 
            unsigned long& h, 
            unsigned long& w, 
            unsigned long pos, 
            unsigned int& l,
            std::vector<Pixel>& pixels
        )
        {
            for(h=0;h<this->png_height;++h)
            {
                ++pos; // skip the filter byte on each scanline
                if(pos > l) break;

                for(w=0;w<this->png_width;++w)
                {
                    p.R = b[pos]; ++pos;
                    p.G = b[pos]; ++pos;
                    p.B = b[pos]; ++pos;
                    if(!this->alpha_flag) { p.A = b[pos]; ++pos; };

                    pixels.push_back(p);
                };
                
                truecolor_matrix.push_back(pixels);
                pixels.clear();
                
            };

        };

    // 
        void PLTE_RGBA(
            unsigned long& h, 
            unsigned long& w, 
            unsigned long pos, 
            unsigned int& l, 
            std::vector<unsigned char>& plte_pixels
        )
        {
            for(h;h<this->png_height;++h)
            {
                ++pos; // skip the filter byte on each scanline
                if(pos > l) break;

                for(w;w<this->png_height;++w)
                {
                    plte_pixels.push_back(palate[pos]);
                };

                indexed_matrix.push_back(plte_pixels);
                plte_pixels.clear();
            }
        }

    // 
        int construct_image_matrix()
        {
         
            unsigned int l = b.size();

            std::vector<Pixel> pixels                   { };
            std::vector<unsigned char> plte_pixels      { };

            unsigned long h, w, pos; h = w = pos = 0;
            unsigned char err;
           
            Pixel p;


            if(!palate.empty()) 
            {
                PLTE_RGBA(h,w,pos,l, plte_pixels);
                return 0;
            }
            else
            {
                RGBA(p,h,w,pos,l,pixels);
                return 0;
            }
 
            return 0;

        };

} Chunks { };


class PNG 
{
    private:

        enum chunk_values
        {
            IEND = 288,
            IDAT = 290,
            IHDR = 295,
            PLTE = 309,
            gAMA = 310,
            sRGB = 334,
            pHYs = 388,
        };

        void read_bytes(std::string& filepath)
        {
            std::ifstream file { filepath, std::ios_base::binary | std::ios_base::in }; 
            if(!file.is_open()){ std::cerr << "Error opening: " << filepath << "\n"; throw; };

            unsigned long chunk_length, chunk_id; 
            chunk_length = chunk_id = 0;
            int err = 0;

            

            validate_header(file); 

            identify_chunk(file, chunk_length, chunk_id); 

        // file will be offset by 16 bytes at this point

        /* ihdr is the chunk that must follow the png header per the specification
        * 
        * we will issue a warning if this is not the case instead of throwing.
        * the code after this warning will run with undefined behavior.
        * 
        */
            if(chunk_id != IHDR) std::cout << 
            "WARNING! PNG header not followed by IDHR chunk" <<
            " - We're decoding a questionably encoded PNG \n";

            //Chunks.read_ihdr(file, chunk_id); 
  

        /* main file decoder
        *
        * read all bytes from the file until the eof is reached.
        * each case is the sum of individual ascii bytes parsed from identity_chunk().
        * 
        */
        
            while(!file.eof())
            {

                chunk_length = chunk_id = 0;

                identify_chunk(file, chunk_length, chunk_id); // offsets file reader by 8 bytes on every call

                // it is a guarantee that the chunk is offset up to the first byte after the header
                switch(chunk_id)
                {
                    case IHDR: { err = Chunks.read_ihdr(file, chunk_length); if(!err) break; throw; }
                    case sRGB: { err = Chunks.read_sRGB(file, chunk_length); if(!err) break; throw; }
                    case gAMA: { err = Chunks.read_gAMA(file, chunk_length); if(!err) break; throw; }
                    case PLTE: { err = Chunks.read_PLTE(file, chunk_length); if(!err) break; throw; }
                    case pHYs: { err = Chunks.read_pHYs(file, chunk_length); if(!err) break; throw; }
                    case IDAT: { err = Chunks.read_IDAT(file, chunk_length); if(!err) break; throw; } // need to consider continous idat blocks
                    case IEND: { err = Chunks.read_IEND(file, chunk_length); if(!err) break; throw; }
                    //default: std::cout << "did not catch a chunk id \n";
                };
            };

            
        };
       
    public:

        PNG(std::string &filepath) { read_bytes(filepath); };

        ~PNG(){};


        void write_file(std::string& new_filepath)
        {
            std::ofstream new_file { new_filepath, std::ios_base::binary | std::ios_base::out }; 
            if(!new_file.is_open()){ std::cerr << "Error opening: " << new_filepath << "\n"; throw; };

        // write png header
            for(int i=0;i<this->PNG_SIG.size();++i) new_file << PNG_SIG[i];
        
            new_file.close();

        }

        /* user methods
        *
        *
        *
        */
        void print_ihdr()
        {
            std::cout <<
            "length             :   " << (int)Chunks.png_width              << "\n" <<
            "width              :   " << (int)Chunks.png_height             << "\n" <<
            "bit_depth          :   " << (int)Chunks.b_depth                << "\n" <<
            "color type         :   " << (int)Chunks.color_type             << "\n" <<
            "compression method :   " << (int)Chunks.comp_method            << "\n" <<
            "filter method      :   " << (int)Chunks.filter_method          << "\n" <<
            "interlace method   :   " << (int)Chunks.interlace_method       << "\n";
        };
       

       void print_pixels()
       {

            unsigned long w, h;
            h = w = 0;

            if(!Chunks.truecolor_matrix.empty())
            {
                for(int w=0;w<Chunks.png_width;++w)
                {
                    for(int h=0;h<Chunks.png_height;++h)
                    {
                        
                        std::cout << 
                        (int)Chunks.truecolor_matrix[w][h].R << " " <<
                        (int)Chunks.truecolor_matrix[w][h].G << " " <<
                        (int)Chunks.truecolor_matrix[w][h].B << " " <<
                        (int)Chunks.truecolor_matrix[w][h].A << " \n ";
                        
                    };
                };
            };

            if(!Chunks.indexed_matrix.empty())
            {
                for(int w=0;w<Chunks.png_width;++w)
                {
                    for(int h=0;h<Chunks.png_height;++h)
                    {
                        
                        //std::cout << 
                        //(int)Chunks.indexed_matrix[w][h] << " ";
                        
                    };
                };
            };

       };

       void pixel_at(unsigned short& x, unsigned short& y)
       {

       }


        
    protected:

        std::vector<char> PNG_SIG  
        { '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' }; 
    
    /* function to identify the png header
    *
    * this function will throw if the first 8 bytes of the file differ from PNG_SIG.
    * this function is intended to only be ran once by the decoder.
    * 
    */
        void validate_header(std::ifstream& file)
        {
            std::vector<char> head_buff;

            int i = 0;

            for(i;i<8;++i)
            {
                head_buff.push_back(file.get());
            } 

            if(this->PNG_SIG != head_buff)
            { 
                std::cout << "Bad PNG" << "\n"; 
                throw; 
            };
        };

    /* function to identify chunks. 
    *
    * the id variable is the sum of all individual ascii bytes.
    * the length is converted to big endian and needed for decoding.
    *
    */
        void identify_chunk(std::ifstream& file, unsigned long& len, unsigned long& id)
        {
            int i = 0;

            for(i;i<8;++i)
            {
                Chunks.chunk_buffer.push_back(file.get());
            };

            len = little_to_big_endian(Chunks.chunk_buffer);

            for(i = 4;i<8;++i) 
            {
                id  += Chunks.chunk_buffer[i];       // get 4 byte chunk id
            };

        };           
};
