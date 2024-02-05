

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


void print_byte(char &c){ std::cout << c << "\n"; };

// 32 bit crc validation
// all bytes after the length up to the crc bytes
void crc_32(std::ifstream &file)
{
    // doing nothing for now, need to impliment this
    for(int i { 4 }; i > 0; --i){ file.get(); };
};



// look into ramifications 
unsigned long little_to_big_endian(std::vector<char> &endian_buff)
{
    return (int)endian_buff[3] | (int)endian_buff[2]<<8 | (int)endian_buff[1]<<16 | (int)endian_buff[0]<<24;
};

















class Chunks 
{

    public:
    // IHDR
        unsigned long png_width { };             // little endian
        unsigned long png_height { };            // little endian
        unsigned char b_depth { };               // valid values 1, 2, 4, 8, 16
        unsigned char color_type { };            // valid values 0, 2, 3, 4, 6
        unsigned char comp_method { };           // should be 0 for standard
        unsigned char filter_method { };         // should be 0 for standard
        unsigned char interlace_method { };      // 0 == no interlace, 1 == Adam7
    // sRGB

        // 0 == Perceptual
        // 1 == Relative Colorimetric
        // 2 == Saturation
        // 3 == Absolute Colorimetric
        char rendering_intent { };
    // gAMA
        unsigned long gamma { };
    // pHYs
        unsigned long pixel_per_unit_x { };
        unsigned long pixel_per_unit_y { };

        // 0 == unknown
        // 1 == meter
        unsigned char unit_specificer { };


    // 0x49 0x48 0x44 0x52
        void make_ihdr(std::ifstream& file, unsigned long length)
        {
          
            std::vector<char> endian_buff { };
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

            if(length != 0) { std::cout << "file sync error" << "\n"; throw; };



            crc_32(file); // offsets files by 4 bytes
        };

    // 0x73 0x52 0x47 0x42
        void make_sRGB(std::ifstream &file, unsigned long &length)
        {
            this->rendering_intent = file.get(); --length;

            if(length != 0) { std::cout << "file sync error" << "\n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

    // 0x67 0x41 0x4D 0x41
        void make_gAMA(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->gamma = file.get(); --length; };

            if(length != 0) { std::cout << "file sync error" << "\n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

        void sample_gamma(){
            int sample { };
            sample = sample / (2^b_depth - 1);
        };

    // 0x70 0x48 0x59 0x73
        void make_pHYs(std::ifstream &file, unsigned long &length)
        {
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_x = file.get(); --length; };
            for(int i = 0; i < 4; ++i){ this->pixel_per_unit_y = file.get(); --length; };

            this->unit_specificer = file.get(); --length;

            if(length != 0) { std::cout << "file sync error" << "\n"; throw; };

            crc_32(file); // offsets files by 4 bytes
        };

    //0x49 0x44 0x54 0x78
        void make_IDAT(std::ifstream &file, unsigned long &length)
        {
        

            unsigned long data = this->png_width * this->png_height;
            for(unsigned long h = this->png_height; h > 0; --h)
            {
                for(unsigned long w = this->png_height; w > 0; --w)
                {
                    std::cout << (char)file.get() << "\n";
                };
            }

        };
} Chunks;

















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

            if(id != 295) std::cout << 
            "WARNING! PNG header not followed by IDHR chunk" <<
            " - We're decoding a questionably encoded PNG" << "\n";

            Chunks.make_ihdr(file, len);

            while(!file.eof())
            {
                len = id = 0;

                identify_chunk(file, len, id); // offsets file reader by 8 bytes on every call

                // it is a guarantee that the chunk is offset up to the first byte after the header
                switch(id)
                {
                    //case 295: Chunks.make_ihdr(file, len); break;
                    case 334: Chunks.make_sRGB(file, len); break;
                    case 310: Chunks.make_gAMA(file, len); break;
                    case 388: Chunks.make_pHYs(file, len); break;
                    case 290: Chunks.make_IDAT(file, len); break;  
                    default: 
                        std::cout << "did not catch a chunk id" << "\n";
                        throw;
                };
            };
        };
       
    public:

        PNG(std::string &filepath) { read_bytes(filepath); };

        ~PNG(){};

        unsigned long width(){  return Chunks.png_width ; };
        unsigned long height(){ return Chunks.png_height; };
        
    protected:

        std::vector<char> PNG_SIG  
        { 
            '\x89', '\x50', '\x4e', '\x47', '\x0d', '\x0a', '\x1a', '\x0a' 
        }; 

        void validate_header(std::ifstream &file)
        {
            std::vector<char> head_buff;

            for(int i { }; i < 8; ++i) head_buff.push_back(file.get());               // read first 8 bytes into header buffer
            if(this->PNG_SIG != head_buff){ std::cout << "Bad PNG" << "\n"; throw; }; // validate header as valid png
        };

        void identify_chunk(std::ifstream &file, unsigned long &len, unsigned long &id)
        {
            for(int i = 0; i < 4; ++i) { len += file.get(); };  // get 4 byte chunk size 
            for(int i = 0; i < 4; ++i) { id  += file.get(); };  // get 4 byte chunk id
        };           
};







