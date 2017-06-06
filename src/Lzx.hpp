#ifndef LZX_HPP
#define LZX_HPP

// Taken from: https://bitbucket.org/alisci01/xnbdecompressor
// "Ported" to C++ by spacechase0

//#region HEADER
/* This file was derived from libmspack
 * (C) 2003-2004 Stuart Caie.
 * (C) 2011 Ali Scissons.
 *
 * The LZX method was created by Jonathan Forbes and Tomi Poutanen, adapted
 * by Microsoft Corporation.
 *
 * This source file is Dual licensed; meaning the end-user of this source file
 * may redistribute/modify it under the LGPL 2.1 or MS-PL licenses.
 */ 
//#region LGPL License
/* GNU LESSER GENERAL public LICENSE version 2.1
 * LzxDecoder is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General public License (LGPL) version 2.1 
 */
//#endregion
//#region MS-PL License
/* 
 * MICROSOFT public LICENSE
 * This source code is subject to the terms of the Microsoft public License (Ms-PL). 
 *  
 * Redistribution and use in source and binary forms, with or without modification, 
 * is permitted provided that redistributions of the source code retain the above 
 * copyright notices and this file header. 
 *  
 * Additional copyright notices should be appended to the list above. 
 * 
 * For details, see <http://www.opensource.org/licenses/ms-pl.html>. 
 */
//#endregion
/*
 * This derived work is recognized by Stuart Caie and is authorized to adapt
 * any changes made to lzxd.c in his libmspack library and will still retain
 * this dual licensing scheme. Big thanks to Stuart Caie!
 * 
 * DETAILS
 * This file is a pure C# port of the lzxd.c file from libmspack, with minor
 * changes towards the decompression of XNB files. The original decompression
 * software of LZX encoded data was written by Suart Caie in his
 * libmspack/cabextract projects, which can be located at 
 * http://http://www.cabextract.org.uk/
 */
//#endregion

#include <SFML/Config.hpp>
#include <stdexcept>
#include <vector>
#include <istream>

namespace lzx
{
    /* CONSTANTS */
    struct LzxConstants {
        public: static constexpr sf::Uint16 MIN_MATCH =				2;
        public: static constexpr sf::Uint16 MAX_MATCH =				257;
        public: static constexpr sf::Uint16 NUM_CHARS =				256;
        public: enum BLOCKTYPE {
            INVALID = 0,
            VERBATIM = 1,
            ALIGNED = 2,
            UNCOMPRESSED = 3
        };
        public: static constexpr sf::Uint16 PRETREE_NUM_ELEMENTS =	20;
        public: static constexpr sf::Uint16 ALIGNED_NUM_ELEMENTS =	8;
        public: static constexpr sf::Uint16 NUM_PRIMARY_LENGTHS =	7;
        public: static constexpr sf::Uint16 NUM_SECONDARY_LENGTHS = 249;
        
        public: static constexpr sf::Uint16 PRETREE_MAXSYMBOLS = 	PRETREE_NUM_ELEMENTS;
        public: static constexpr sf::Uint16 PRETREE_TABLEBITS =		6;
        public: static constexpr sf::Uint16 MAINTREE_MAXSYMBOLS = 	NUM_CHARS + 50*8;
        public: static constexpr sf::Uint16 MAINTREE_TABLEBITS = 	12;
        public: static constexpr sf::Uint16 LENGTH_MAXSYMBOLS = 	NUM_SECONDARY_LENGTHS + 1;
        public: static constexpr sf::Uint16 LENGTH_TABLEBITS =		12;
        public: static constexpr sf::Uint16 ALIGNED_MAXSYMBOLS = 	ALIGNED_NUM_ELEMENTS;
        public: static constexpr sf::Uint16 ALIGNED_TABLEBITS = 	7;
                
        public: static constexpr sf::Uint16 LENTABLE_SAFETY =		64;
    };

    /* EXCEPTIONS */
    class UnsupportedWindowSizeRange : public std::exception
    {
    };
    struct LzxState {
        public: sf::Uint32						R0, R1, R2;			/* for the LRU offset system				*/
        public: sf::Uint16					main_elements;		/* number of main tree elements				*/
        public: int						header_read;		/* have we started decoding at all yet? 	*/
        public: LzxConstants::BLOCKTYPE	block_type;			/* type of this block						*/
        public: sf::Uint32						block_length;		/* uncompressed length of this block 		*/
        public: sf::Uint32						block_remaining;	/* uncompressed sf::Uint8s still left to decode	*/
        public: sf::Uint32						frames_read;		/* the number of CFDATA blocks processed	*/
        public: int						intel_filesize;		/* magic header value used for transform	*/
        public: int						intel_curpos;		/* current offset in transform space		*/
        public: int						intel_started;		/* have we seen any translateable data yet?	*/
        
        public: std::vector<sf::Uint16>		PRETREE_table;
        public: std::vector<sf::Uint8>		PRETREE_len;
        public: std::vector<sf::Uint16>		MAINTREE_table;
        public: std::vector<sf::Uint8>		MAINTREE_len;
        public: std::vector<sf::Uint16>		LENGTH_table;
        public: std::vector<sf::Uint8>		LENGTH_len;
        public: std::vector<sf::Uint16>		ALIGNED_table;
        public: std::vector<sf::Uint8>		ALIGNED_len;
        
        // NEEDED MEMBERS
        // CAB actualsize
        // CAB window
        // CAB window_size
        // CAB window_posn
        public: sf::Uint32		actual_size;
        public: std::vector<sf::Uint8>	window;
        public: sf::Uint32		window_size;
        public: sf::Uint32		window_posn;
    };
    //#region Our BitBuffer Class
    class BitBuffer
    {
        public:
        sf::Uint32 buffer;
        public: sf::Uint8 bitsleft;
        std::istream& byteStream;
        
        public: BitBuffer(std::istream& stream);
        public: void InitBitStream();
        public: void EnsureBits(sf::Uint8 bits);
        public: sf::Uint32 PeekBits(sf::Uint8 bits);
        public: void RemoveBits(sf::Uint8 bits);
        public: sf::Uint32 ReadBits(sf::Uint8 bits);
        public: sf::Uint32 GetBuffer();
        public: sf::Uint8 GetBitsLeft();
    };
    //#endregion
    class LzxDecoder
    {
        public:
        public: static std::vector<sf::Uint32> position_base;
        public: static std::vector<sf::Uint8> extra_bits;
        
        private: LzxState m_state;
        
        public: LzxDecoder (int window);
        
        public: int Decompress(std::istream& inData, int inLen, std::iostream& outData, int outLen);
        
        // READ_LENGTHS(table, first, last)
        // if(lzx_read_lens(LENTABLE(table), first, last, bitsleft))
        //   return ERROR (ILLEGAL_DATA)
        // 
        
        // TODO make returns throw exceptions
        private: int MakeDecodeTable(sf::Uint32 nsyms, sf::Uint32 nbits, std::vector<sf::Uint8>& length, std::vector<sf::Uint16>& table);
        
        // TODO throw exceptions instead of returns
        private: void ReadLengths(std::vector<sf::Uint8>& lens, sf::Uint32 first, sf::Uint32 last, BitBuffer& bitbuf);
        
        private: sf::Uint32 ReadHuffSym(std::vector<sf::Uint16>& table, std::vector<sf::Uint8>& lengths, sf::Uint32 nsyms, sf::Uint32 nbits, BitBuffer& bitbuf);
        
    };
}

#endif // LZX_HPP
