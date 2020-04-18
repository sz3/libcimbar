// Base64/85/91 en/decoders, still C-String, TSV, XML and JSON friendly.
// - rlyeh 2011..2016, zlib/libpng licensed.

// Some notes {
// - [x] Base64: canonical implementation.
// - [x] Base85: 17% to 08% smaller than Base64, still C-String friendly. Custom Z85 variant.
// - [x] Base91: 19% to 10% smaller than Base64, still JSON, XML and TSV friendly. Custom basE91 variant.
// - [x] Encoded data can be "quoted", splitted with tabs, spaces, linefeeds and carriages.
// }

// Extra licensing {
// - Base64 is based on code by René Nyffenegger (zlib/libpng licensed)
// - Base91 is based on code by Joachim Henke {
//     Copyright (c) 2000-2006 Joachim Henke
//     http://base91.sourceforge.net/ (v0.6.0)
//     Redistribution and use in source and binary forms, with or without
//     modification, are permitted provided that the following conditions are met:
//      - Redistributions of source code must retain the above copyright notice,
//        this list of conditions and the following disclaimer.
//      - Redistributions in binary form must reproduce the above copyright notice,
//        this list of conditions and the following disclaimer in the documentation
//        and/or other materials provided with the distribution.
//      - Neither the name of Joachim Henke nor the names of his contributors may
//        be used to endorse or promote products derived from this software without
//        specific prior written permission.
//     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//     ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//     LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//     CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//     SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//     INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//     ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//     POSSIBILITY OF SUCH DAMAGE.
// } }

#pragma once
#include <string>

#define BASE_VERSION "1.0.2" /* (2016/04/20) Base85 support (custom Z85); In-place API; Project renamed
#define BASE_VERSION "1.0.1" // (2015/12/07) Update sample
#define BASE_VERSION "1.0.0" // (2014/04/26) Base64 support
#define BASE_VERSION "0.0.0" // (2013/04/12) Initial commit (custom basE91) */

namespace {

    /* Public API */

    template<unsigned N> struct base {
        static std::string encode( const std::string &binary );              // functional api
        static std::string decode( const std::string &text );                // functional api
        static bool encode( std::string &out, const std::string &binary );   // in-place api
        static bool decode( std::string &out, const std::string &text );     // in-place api
    };

    typedef base<64> base64;
    typedef base<85> base85;
    typedef base<91> base91;

    /* API details */

    template<>
    bool base<91>::encode( std::string &out, const std::string &binary ) {

        const unsigned char enctab[91] = {
            /* // Henke's original
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', //00..12
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', //13..25
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', //26..38
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', //39..51
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '#', '$', //52..64
            '%', '&', '(', ')', '*', '+', ',', '.', '/', ':', ';', '<', '=', //65..77
            '>', '?', '@', '[', ']', '^', '_', '`', '{', '|', '}', '~', '"'  //78..90 */
            // // rlyeh's modification
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', //00..12
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', //13..25
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', //26..38
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', //39..51
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '#', '$', //52..64
            '%', '&', '(', ')', '*', '+', ',', '.', '/', ':', ';', '-', '=', //65..77
            '\\','?', '@', '[', ']', '^', '_', '`', '{', '|', '}', '~', '\'' //78..90
        };

        out.clear();
        const unsigned char *ib = (unsigned char *) binary.c_str();

        unsigned long queue = 0;
        unsigned int nbits = 0;

        for( size_t len = binary.size(); len--; ) {
            queue |= *ib++ << nbits;
            nbits += 8;
            if (nbits > 13) {   /* enough bits in queue */
                unsigned int val = queue & 8191;

                if (val > 88) {
                    queue >>= 13;
                    nbits -= 13;
                } else {    /* we can take 14 bits */
                    val = queue & 16383;
                    queue >>= 14;
                    nbits -= 14;
                }
                out.push_back( enctab[val % 91] );
                out.push_back( enctab[val / 91] );
            }
        }

        /* process remaining bits from bit queue; write up to 2 bytes */
        if (nbits) {
            out.push_back( enctab[queue % 91] );
            if (nbits > 7 || queue > 90)
                out.push_back( enctab[queue / 91] );
        }

        return true;
    }


    template<>
    bool base<91>::decode( std::string &out, const std::string &text ) {

        const unsigned char dectab[256] = {
            /* // Henke's original
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //000..015
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //016..031
            91, 62, 90, 63, 64, 65, 66, 91, 67, 68, 69, 70, 71, 91, 72, 73, //032..047
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 74, 75, 76, 77, 78, 79, //048..063
            80,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //064..079
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 81, 91, 82, 83, 84, //080..095
            85, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //096..111
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 86, 87, 88, 89, 91, //112..127
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //128..143
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //144..159
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //160..175
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //176..191
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //192..207
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //208..223
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //224..239
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91  //240..255 */
            // // rlyeh's modification
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //000..015
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //016..031
            91, 62, 91, 63, 64, 65, 66, 90, 67, 68, 69, 70, 71, 76, 72, 73, //032..047 // @34: ", @39: ', @45: -
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 74, 75, 91, 77, 91, 79, //048..063 // @60: <, @62: >
            80,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //064..079
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 81, 78, 82, 83, 84, //080..095 // @92: slash
            85, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //096..111
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 86, 87, 88, 89, 91, //112..127
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //128..143
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //144..159
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //160..175
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //176..191
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //192..207
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //208..223
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, //224..239
            91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91  //240..255
        };

        out.clear();
        const unsigned char *ib = (unsigned char *) text.c_str();

        unsigned long queue = 0;
        unsigned int nbits = 0;
        int val = -1;

        for( size_t len = text.size(); len--; ) {
            unsigned int d = dectab[*ib++];
            if (d == 91)
                continue;   /* ignore non-alphabet chars */
            if (val == -1)
                val = d;    /* start next value */
            else {
                val += d * 91;
                queue |= val << nbits;
                nbits += (val & 8191) > 88 ? 13 : 14;
                do {
                    out.push_back( char( queue ) );
                    queue >>= 8;
                    nbits -= 8;
                } while (nbits > 7);
                val = -1;   /* mark value complete */
            }
        }

        /* process remaining bits; write at most 1 byte */
        if (val != -1)
            out.push_back( char( queue | val << nbits ) );

        return true;
    }

    // base85 (z85): standard rfc size (multiples of 4/5)

    inline bool encode85( std::string &out, const unsigned char *raw, size_t rawlen ) {
        if( rawlen % 4 ) {
            return false; // error: raw string size must be multiple of 4
        }
        // encode
        const char encoder[86] =
            "0123456789" "abcdefghij" "klmnopqrst" "uvwxyzABCD"             // 00..39
            "EFGHIJKLMN" "OPQRSTUVWX" "YZ.-:+=^!/" "*?&<>()[]{" "}@%$#";    // 40..84 // free chars: , ; _ ` | ~ \'
        out.resize( rawlen * 5 / 4 );
        for( size_t o = 0; o < rawlen * 5 / 4; raw += 4 ) {
            unsigned value = (raw[0] << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3];
            out[o++] = encoder[ (value / 0x31C84B1) % 0x55 ];
            out[o++] = encoder[   (value / 0x95EED) % 0x55 ];
            out[o++] = encoder[    (value / 0x1C39) % 0x55 ];
            out[o++] = encoder[      (value / 0x55) % 0x55 ];
            out[o++] = encoder[               value % 0x55 ];
        }
        return true;
    }

    inline bool decode85( std::string &out, const unsigned char *z85, size_t z85len ) {
        if( z85len % 5 ) {
            return false; // error: z85 string size must be multiple of 5
        }
        // decode
        const unsigned char decoder[128] = {
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 0x00..0x0F
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 0x10..0x1F
             0, 68,  0, 84, 83, 82, 72,  0, 75, 76, 70, 65,  0, 63, 62, 69, // 0x20..0x2F
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 64,  0, 73, 66, 74, 71, // 0x30..0x3F
            81, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, // 0x40..0x4F
            51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77,  0, 78, 67,  0, // 0x50..0x5F
             0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, // 0x60..0x6F
            25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 79,  0, 80,  0,  0, // 0x70..0x7F
        };
        out.resize( z85len * 4 / 5 );
        for( size_t o = 0; o < z85len * 4 / 5; z85 += 5 ) {
            unsigned value = decoder[z85[0]] * 0x31C84B1 + decoder[z85[1]] * 0x95EED +
                             decoder[z85[2]] *    0x1C39 + decoder[z85[3]] *    0x55 + decoder[z85[4]];
            out[o++] = (value >> 24) & 0xFF;
            out[o++] = (value >> 16) & 0xFF;
            out[o++] = (value >>  8) & 0xFF;
            out[o++] = (value >>  0) & 0xFF;
        }
        return true;
    }

    // base85 (z85): arbitrary size (this may lead up to four additional bytes)

    template<>
    bool base<85>::encode( std::string &out, const std::string &rawstr ) {
        // create padding, if needed
        std::string pad4 = std::string( (const char *)&rawstr[0], rawstr.size() ) + '\1' + std::string("\0\0\0\0", 4 - (rawstr.size() + 1) % 4);
        return encode85( out, (const unsigned char *)pad4.c_str(), pad4.size() );
    }

    template<>
    bool base<85>::decode( std::string &out, const std::string &z85str ) {
        if( !decode85( out, (const unsigned char *)&z85str[0], z85str.size() ) ) {
            return false;            
        } else {
            // remove padding, if needed
            while( out.size() && *out.rbegin() == '\0' ) out.resize( out.size() - 1 );
               if( out.size() && *out.rbegin() == '\1' ) out.resize( out.size() - 1 );
            return true;
        }
    }

    // base64

    template<>
    bool base<64>::encode( std::string &out, const std::string &text ) {

        const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        unsigned char const* bytes_to_encode = (unsigned char const *)text.c_str();
        unsigned int in_len = (unsigned int)text.size();
        unsigned int i = 0;
        unsigned int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        out.clear();

        while( in_len-- ) {
            char_array_3[i++] = *(bytes_to_encode++);
            if( i == 3 ) {
                char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] =   char_array_3[2] & 0x3f;

                for( i = 0; (i <4) ; i++ )
                    out += chars[char_array_4[i]];
                i = 0;
            }
        }

        if( i ) {
            for( j = i; j < 3; j++ )
                char_array_3[j] = '\0';

            char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] =   char_array_3[2] & 0x3f;

            for( j = 0; (j < i + 1); j++ )
                out += chars[char_array_4[j]];

            while( (i++ < 3) )
                out += '=';
        }

        return true;
    }

    template<>
    bool base<64>::decode( std::string &out, const std::string &encoded ) {

        const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";

        unsigned int in_len = (unsigned int)encoded.size();
        unsigned int i = 0;
        unsigned int j = 0;
        unsigned int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        out.clear();

        while (in_len-- && ( encoded[in_] != '=') && //is_base64(encoded[in_])) {
            (isalnum(encoded[in_]) || encoded[in_] == '+' || encoded[in_] == '/')) {
            char_array_4[i++] = encoded[in_]; in_++;
            if (i ==4) {
                for (i = 0; i <4; i++)
                    char_array_4[i] = chars.find(char_array_4[i]);

                char_array_3[0] =  (char_array_4[0] << 2)        + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    out += char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j <4; j++)
                char_array_4[j] = 0;

            for (j = 0; j <4; j++)
                char_array_4[j] = chars.find(char_array_4[j]);

            char_array_3[0] =  (char_array_4[0] << 2)        + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++) out += char_array_3[j];
        }

        return true;
    }


    // aliases
    template<> std::string base<64>::encode( const std::string &binary ) {
        std::string out;
        return base<64>::encode( out, binary ) ? out : std::string();
    }
    template<> std::string base<64>::decode( const std::string &text ) {
        std::string out;
        return base<64>::decode( out, text ) ? out : std::string();
    }

    template<> std::string base<85>::encode( const std::string &binary ) {
        std::string out;
        return base<85>::encode( out, binary ) ? out : std::string();
    }
    template<> std::string base<85>::decode( const std::string &text ) {
        std::string out;
        return base<85>::decode( out, text ) ? out : std::string();
    }

    template<> std::string base<91>::encode( const std::string &binary ) {
        std::string out;
        return base<91>::encode( out, binary ) ? out : std::string();
    }
    template<> std::string base<91>::decode( const std::string &text ) {
        std::string out;
        return base<91>::decode( out, text ) ? out : std::string();
    }
}


#ifdef BASE_BUILD_TESTS

// tiny unittest suite. rlyeh, public domain {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define suite(...) if(printf("------ " __VA_ARGS__),puts(""),true)
#define test(...)  (errno=0,++tst,err+=!(ok=!!(__VA_ARGS__))),printf("[%s] %d %s (%s)\n",ok?" OK ":"FAIL",__LINE__,#__VA_ARGS__,strerror(errno))
unsigned tst=0,err=0,ok=atexit([]{ suite("summary"){ printf("[%s] %d tests = %d passed + %d errors\n",err?"FAIL":" OK ",tst,tst-err,err); }});
// } /*usage:*/ int main() { /* orphan test */ test(1<2); suite("grouped tests") { test(1<2); test(1<2); } }

#include <cassert>
#include <iostream>

void unittest( const std::string &binary ) {
    bool allows_preview = [&binary]{ for( const unsigned char &ch : binary ) if( ch > 127 ) return 0; return 1; }();
    size_t base64len = 0, base85len = 0, base91len = 0, binarylen = binary.size();

    {
        std::string enc64 = base64::encode(binary);
        std::string dec64 = base64::decode(enc64);
        base64len = enc64.size();

        // sanity check
        test( dec64 == binary );
    }

    {
        std::string enc85 = base85::encode(binary);
        std::string dec85 = base85::decode(enc85);
        base85len = enc85.size();

        // sanity check
        test( dec85 == binary );

        // C-string checks
        test( enc85.find_first_of('"') == std::string::npos );
        test( enc85.find_first_of('\\') == std::string::npos );
    }   

    {
        std::string enc91 = base91::encode(binary);
        std::string dec91 = base91::decode(enc91);
        base91len = enc91.size();

        // sanity check
        test( dec91 == binary );

        // XML, JSON checks
        test( enc91.find_first_of('<') == std::string::npos );
        test( enc91.find_first_of('>') == std::string::npos );
        test( enc91.find_first_of('"') == std::string::npos );

        // extra whitespace check (base91 strings with extra whitespaces)
        std::string white91 = " \r\n\f\t\v\n" + enc91 + " \r\n\f\t\v\n";
        test( binary == base91::decode(white91));

        // break whitespace check (base91 strings with whitespaces in between)
        std::string break91 = enc91;
        break91.insert( break91.size() / 2, " \r\n\f\t\v\n" );
        test( binary == base91::decode(break91));

        // split text check
        std::string split91 = enc91.substr(0, enc91.size() / 2) + "\r\n\r\n\t\t  " + enc91.substr(enc91.size() / 2);
        test( binary == base91::decode(split91) );
    }   

    // more sanity checks
    test( binarylen     <= base91len );
    test( base91len     <= base85len );
    test( base85len - 4 <= base64len ); // optional 4-bytes overhead

    auto overhead = [&binarylen]( size_t encoding_size ) -> size_t { return ((encoding_size*100/binarylen)-100); };
    std::cout << "\nresults: " << ( allows_preview ? std::string() + '\"' + binary + '\"' : "(hidden text)" ) << '\n';
    std::cout << "\tbinary: " << overhead(binarylen) << "% overhead (total: " << binarylen << " bytes)\n";
    std::cout << "\tbase64: " << overhead(base64len) << "% overhead (total: " << base64len << " bytes)\n";
    std::cout << "\tbase85: " << overhead(base85len) << "% overhead (total: " << base85len << " bytes)\n";
    std::cout << "\tbase91: " << overhead(base91len) << "% overhead (total: " << base91len << " bytes)\n\n";   
}

void unittest_inplace( const std::string &binary ) {
    {
        std::string enc64, dec64;
        test( base64::encode(enc64, binary) );
        test( base64::decode(dec64, enc64) );
        test( dec64 == binary );
    }
    {
        std::string enc85, dec85;
        test( base85::encode(enc85, binary) );
        test( base85::decode(dec85, enc85) );
        test( dec85 == binary );
    }
    {
        std::string enc91, dec91;
        test( base91::encode(enc91, binary) );
        test( base91::decode(dec91, enc91) );
        test( dec91 == binary );
    }
}

int main() {
    // basic sample
    std::string encoded_64 = base64::encode("Hello world from BASE64! \x1");
    std::string decoded_64 = base64::decode(encoded_64);
    std::cout<< decoded_64 << " <-> " << encoded_64 << std::endl;

    std::string encoded_85 = base85::encode("Hello world from BASE85! \x1");
    std::string decoded_85 = base85::decode(encoded_85);
    std::cout<< decoded_85 << " <-> " << encoded_85 << std::endl;

    std::string encoded_91 = base91::encode("Hello world from BASE91! \x1");
    std::string decoded_91 = base91::decode(encoded_91);
    std::cout<< decoded_91 << " <-> " << encoded_91 << std::endl << std::endl;

    // [ref] http://en.wikipedia.org/wiki/Base64
    unittest( "Man is distinguished, not only by his reason, but by this singular passion from\n"
     "other animals, which is a lust of the mind, that by a perseverance of delight in the continued\n"
     "and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure." );

    // Binary test
    unittest( "hello world \x1\x2");

    // Zero-ending
    unittest( std::string("abc\x1\x2\0", 6 ) );

    // 16 mb dataset
    std::string charset;
    charset.reserve( 256 * 256 * 256 );
    for( int i = 256 ; --i >= 0; ) {
        for( int j = 256 ; --j >= 0; ) {
            for( int k = 256 ; --k >= 0; ) {
                charset += char(i) + char(j) + char(k);
            }
        }
    }
    unittest_inplace( charset );

    std::cout << "All ok." << std::endl;
}
#endif


#ifdef BASE_BUILD_DEMO
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
int main( int argc, const char **argv ) {
    if( argc == 4 ) {
        std::ifstream ifs( argv[3], std::ios::binary );
        std::stringstream ss;
        ss << ifs.rdbuf();
        bool enc = argv[1][0] == 'e', dec = argv[1][0] == 'd';
        bool b64 = argv[2][0] == '6', b85 = argv[2][0] == '8', b91 = argv[2][0] == '9';
        /**/ if( b91 && enc ) return std::cout << base91::encode( ss.str() ), (ifs.good() ? 0 : 1);
        else if( b91 && dec ) return std::cout << base91::decode( ss.str() ), (ifs.good() ? 0 : 1);
        else if( b85 && enc ) return std::cout << base85::encode( ss.str() ), (ifs.good() ? 0 : 1);
        else if( b85 && dec ) return std::cout << base85::decode( ss.str() ), (ifs.good() ? 0 : 1);
        else if( b64 && enc ) return std::cout << base64::encode( ss.str() ), (ifs.good() ? 0 : 1);
        else if( b64 && dec ) return std::cout << base64::decode( ss.str() ), (ifs.good() ? 0 : 1);
    }
    std::cout << argv[0] << " [e|d] [64|85|91] file" << std::endl;
    return 1;
}
#endif
