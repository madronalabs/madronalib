//
//  MLTextUtils.cpp
//  madronalib
//
//  Created by Randy Jones on 12/3/14.
//
//

#include "MLTextUtils.h"

namespace ml { namespace textUtils {
	
	using namespace utf;
	
	static const int npos = -1;
		
	bool isDigit(codepoint_type c)
	{
		if (c >= '0' && c <= '9')
			return true;
		return false;
	}
	bool isASCII(codepoint_type c)
	{
		return(c < 0x7f);
	}
	
	bool isLatin(codepoint_type c)
	{
		// includes Latin-1 Supplement
		return(c <= 0xFF);
	}
	
	bool isWhitespace(codepoint_type ch)
	{
		return (ch >= 0x0009 && ch <= 0x000D) || ch == 0x0020 || ch == 0x0085 || ch == 0x00A0 || ch == 0x1680
		|| (ch >= 0x2000 && ch <= 0x200A) || ch == 0x2028 || ch == 0x2029 || ch == 0x202F
		||  ch == 0x205F || ch == 0x3000;
	}
	
	bool isCJK(codepoint_type ch)
	{
		return (ch >= 0x4E00 && ch <= 0x9FBF)   // CJK Unified Ideographs
		|| (ch >= 0x2E80 && ch <= 0x2FDF)   // CJK Radicals Supplement & Kangxi Radicals
		|| (ch >= 0x2FF0 && ch <= 0x30FF)   // Ideographic Description Characters, CJK Symbols and Punctuation & Japanese
		|| (ch >= 0x3100 && ch <= 0x31BF)   // Korean
		|| (ch >= 0xAC00 && ch <= 0xD7AF)   // Hangul Syllables
		|| (ch >= 0xF900 && ch <= 0xFAFF)   // CJK Compatibility Ideographs
		|| (ch >= 0xFE30 && ch <= 0xFE4F)   // CJK Compatibility Forms
		|| (ch >= 0x31C0 && ch <= 0x4DFF);  // Other exiensions
	}
	
	char * spaceStr( int numIndents )
	{
		static char * pBuf = (char *)"                                                   ";
		static int len = (int)strlen(pBuf);
		int n = numIndents*2;
		if (n > len) n = len;
		return &pBuf[len - n];
	}
	
	int digitsToNaturalNumber(const char32_t* p)
	{
		if(!p) return 0;
		int v = 0;
		int l = 0;
		int d;
		char c;
		
		const int kMLMaxNumberDigits = 14;
		
		while (p[l] && (l < kMLMaxNumberDigits-1))
		{
			c = p[l];
			if (c >= '0' && c <= '9')
				d = (c - '0');
			else
				break;
			v = (v * 10) + d;
			l++;
		}
		return v;
	}
	
	const char* naturalNumberToDigits(int value, char* pDest) 
	{
		const int base = 10;
		char* ptr = pDest, *ptr1 = pDest, tmp_char;
		int tmp_value;
		
		if(value <= 0)
		{
			*pDest = '0';
			*(++pDest) = '\0';
			return pDest;
		}
		do 
		{
			tmp_value = value;
			value /= base;
			*ptr++ = "0123456789abcdef"[tmp_value - value*base];
		} while ( value > 0 );
		
		*ptr-- = '\0';
		while(ptr1 < ptr) 
		{
			tmp_char = *ptr;
			*ptr-- = *ptr1;
			*ptr1++ = tmp_char;
		}
		return pDest;
	}
	
	int textToNaturalNumber(const TextFragment& frag)
	{
		std::vector<codepoint_type> vec = textToCodePointVector(frag);
		return digitsToNaturalNumber(vec.data());
	}
	
	TextFragment naturalNumberToText(int i)
	{
		const int kMLMaxNumberDigits = 14;
		
		char buf[kMLMaxNumberDigits + 2] = {0};
		char *p = buf + kMLMaxNumberDigits + 1;	
		char *end = p;
		
		// null-terminate the string
		*end = 0;
		
		// work backwards
		do 
		{
			*--p = '0' + (i % 10);
			i /= 10;
		} 
		while (i != 0);
		return (TextFragment(p, end - p));
	}
	
	
	int findFirst(const TextFragment& frag, const codepoint_type b)
	{
		int r = npos;
		if(!frag) return r;
		int i=0;
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return r;
			if(c == b)
			{
				r = i;
				break;
			}
			i++;
		}	
		return r;
	}
	
	int findLast(const TextFragment& frag, const codepoint_type b)
	{
		int r = npos;
		if(!frag) return r;
		int i=0;
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return r;
			if(c == b)
			{
				r = i;
			}
			i++;
		}	
		return r;
	}

	int findFirst(const TextFragment& frag, std::function<bool(codepoint_type)> matchFn)
	{
		int r = npos;
		if(!frag) return r;
		int i=0;
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return r;
			if(matchFn(c))
			{
				r = i;
				break;
			}
			i++;
		}	
		return r;
	}
	
	// TODO dumb, have to call matchFn on each code point because we have no revese iterator 
	int findLast(const TextFragment& frag, std::function<bool(codepoint_type)> matchFn)
	{
		int r = npos;
		if(!frag) return r;
		int i=0;
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return r;
			if(matchFn(c))
			{
				r = i;
			}
			i++;
		}	
		return r;
	}
		
	TextFragment map(const TextFragment& frag, std::function<codepoint_type(codepoint_type)> f)
	{
		if(!frag) return TextFragment();
		std::vector<codepoint_type> vec = textToCodePointVector(frag);
		std::transform(vec.begin(), vec.end(), vec.begin(), f);
		return codePointVectorToText(vec);
	}
	
	TextFragment reduce(const TextFragment& frag, std::function<bool(codepoint_type)> matchFn)
	{
		if(!frag) return TextFragment();
		char buf[frag.lengthInBytes()];
		char* pb = buf;
		
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return TextFragment();
			if(matchFn(c))
			{
				pb = utf::internal::utf_traits<utf::utf8>::encode(c, pb);
			}
		}	
		
		return TextFragment(buf, pb - buf);
	}
	
	std::vector< TextFragment > split(TextFragment frag, codepoint_type delimiter)
	{
		std::vector<TextFragment> output;
		int start = 0;
		int end = 0;
		int pieceLen = 0;
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return std::vector< TextFragment >();
			pieceLen++;
			end++;
			if(c == delimiter)
			{			
				if(pieceLen > 1)
				{
					output.push_back(subText(frag, start, end - 1));
				}
				start = end;
				pieceLen = 0;
			}
		}	
		if(pieceLen > 0)
		{
			output.push_back(subText(frag, start, end));
		}
		return output;
	}
	
	TextFragment join(const std::vector<TextFragment>& vec)
	{
		TextFragment sum;
		int len = vec.size();	 
		for(int i=0; i<len; ++i)
		{
			TextFragment frag = vec[i];
			sum = TextFragment(sum, vec[i]);			
		}
		return sum;
	}
	
	TextFragment join(const std::vector<TextFragment>& vec, codepoint_type delimiter)
	{
		TextFragment delimFrag(delimiter);
		TextFragment sum;
		int len = vec.size();	 
		for(int i=0; i<len; ++i)
		{
			TextFragment frag = vec[i];
			sum = TextFragment(sum, vec[i]);	
			if(ml::within(i, 0, len - 1))
			{
				sum = TextFragment(sum, delimFrag);
			}
		}
		return sum;
	}
	
	TextFragment subText(const TextFragment& frag, int start, int end)
	{		
		// this impl does an unneccesary copy, to keep TextFragment very simple for now.
		if(!frag) return TextFragment();
		if(start >= end) return TextFragment();
		
		// temp buffer big enough to hold whole input fragment if needed.
		// we won't know the output fragment size in bytes until iterating the code points. 
		int len = frag.lengthInBytes();
		SmallStackBuffer temp(len);
		char* buf = temp.data();
		char* pb = buf;
		
		auto first = codepoint_iterator<const char*>(frag.getText());		
		auto it = first;
		for(int i=0; i<start; ++i)
		{
			++it;
		}
		
		for (int i=0; i<end - start; ++i) 
		{
			// write the codepoint as UTF-8 to the buffer
			if(!utf::internal::validate_codepoint(*it)) return TextFragment();
			pb = utf::internal::utf_traits<utf::utf8>::encode(*it, pb);
			++it;
		}	
		
		return TextFragment(buf, pb - buf);
	}
	
	TextFragment stripFileExtension(const TextFragment& frag)
	{
		int dotLoc = findLast(frag, '.');
		if(dotLoc >= 0)
		{
			return subText(frag, 0, dotLoc);
		}		
		return frag;
	}
	
	TextFragment getShortFileName(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, slashLoc + 1, frag.lengthInCodePoints());
		}
		return frag;
	}
	
	TextFragment getPath(const TextFragment& frag)
	{
		int slashLoc = findLast(frag, '/');
		if(slashLoc >= 0)
		{
			return subText(frag, 0, slashLoc);
		}
		return frag;
	}
	
	// TODO extend to recognize Cyrillic and other scripts
	Symbol bestScriptForTextFragment(const TextFragment& frag)
	{
		for (const codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return "unknown";
			// if there are any CJK characters, return CJK
			if (isCJK(c)) 
			{ 
				return "cjk"; 
			}
			else if(!isLatin(c))
			{
				return "unknown";
			}
		}
		return "latin";
	}
	
	static const char base64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	int indexOf(const char* str, char c)
	{
		int r = -1;
		int len = strlen(str);
		for(int i=0; i<len; ++i)
		{
			if(str[i] == c)
			{
				r = i;
				break;
			}
		}
		return r;
	}
	
	TextFragment base64Encode(const std::vector<uint8_t>& in)
	{
		int len = in.size();
		std::vector<char> out;
		int b;
		for (int i = 0; i < len; i += 3)  
		{
			b = (in[i] & 0xFC) >> 2;
			out.push_back(base64table[b]);
			b = (in[i] & 0x03) << 4;
			if (i + 1 < len)      
			{
				b |= (in[i + 1] & 0xF0) >> 4;
				out.push_back(base64table[b]);
				b = (in[i + 1] & 0x0F) << 2;
				if (i + 2 < len)  
				{
					b |= (in[i + 2] & 0xC0) >> 6;
					out.push_back(base64table[b]);
					b = in[i + 2] & 0x3F;
					out.push_back(base64table[b]);
				} 
				else  
				{
					out.push_back(base64table[b]);
					out.push_back('=');
				}
			} 
			else      
			{
				out.push_back(base64table[b]);
				out.push_back('=');
				out.push_back('=');
			}
		}
		out.push_back(0);		
		return TextFragment(out.data());
	}

	std::vector<uint8_t> base64Decode(const TextFragment& frag)
	{
		int len = frag.lengthInBytes();
		if(len % 4) return std::vector<uint8_t>();
		std::vector<uint8_t> decoded;
		const char * inChars = frag.getText();
		int b[4];
		for(int i=0; i<len; i += 4)
		{
			for(int j=0; j<4; ++j)
			{
				b[j] = indexOf(base64table, inChars[i + j]);
			}
			decoded.push_back((b[0] << 2) | (b[1] >> 4));
			if(b[2] < 64)
			{
				decoded.push_back((b[1] << 4) | (b[2] >> 2));
				if (b[3] < 64)  
				{
					decoded.push_back((b[2] << 6) | b[3]);
				}
			}
		}
		return decoded;
	}
		
	TextFragment stripWhitespaceAtEnds(const TextFragment& frag)
	{
		std::function<bool(codepoint_type)> f([](codepoint_type c){ return !isWhitespace(c); });
		int first = findFirst(frag, f);
		int last = findLast(frag, f);
		if((first == npos) || (last == npos)) return TextFragment();
		return(subText(frag, first, last + 1));
	}
	
	TextFragment stripAllWhitespace(const TextFragment& frag)
	{
		std::function<bool(codepoint_type)> f([](codepoint_type c){ return !isWhitespace(c); });
		return reduce(frag, f);
	}
	
	std::vector<uint8_t> AES256CBCEncode(const std::vector<uint8_t>& input, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv)
	{
		if( !(input.size() > 0) || !(key.size() == 32) || !(iv.size() == 32) ) return std::vector<uint8_t>();

		aes256_context ctx; 
		aes256_init(&ctx, key.data());
		
		const int blockSize = 16;
		int inputSize = input.size();
		int blocks = inputSize/blockSize + 1;
		int paddedSize = blockSize*(blocks);
		
		// add PKCS padding 	
		std::vector<uint8_t> plaintext = input;
		plaintext.resize(paddedSize);
		int padBytes = paddedSize - inputSize;
		for(int i = inputSize; i < paddedSize; ++i)
		{
			plaintext[i] = padBytes;
		}
		
		std::vector<uint8_t> ciphertext(paddedSize);
		uint8_t currentIV[blockSize];
		uint8_t workVector[blockSize];
				
		for(int i=0; i<blockSize; ++i)
		{
			currentIV[i] = iv[i];	
		}
		
		for(int b=0; b<blocks; ++b)
		{
			// get plaintext XOR IV
			for(int i=0; i<blockSize; ++i)
			{
				workVector[i] = plaintext[b*blockSize + i] ^ currentIV[i];
			}
			
			aes256_encrypt_ecb(&ctx, workVector);
			
			// write to ciphertext, get new IV
			for(int i=0; i<blockSize; ++i)
			{
				ciphertext[b*blockSize + i] = workVector[i];
				currentIV[i] = workVector[i];
			}
		}
		
		aes256_done(&ctx);
		return ciphertext;
	}
	
	std::vector<uint8_t> AES256CBCDecode(const std::vector<uint8_t>& cipher, const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv)
	{
		if( !(cipher.size() > 0) || (key.size() < 32) || (iv.size() < 32) ) return std::vector<uint8_t>();
		
		aes256_context ctx; 
		aes256_init(&ctx, key.data());
				
		const int blockSize = 16;
		int blocks = cipher.size() / blockSize;
		
		std::vector<uint8_t> plaintext(blockSize*blocks);
		
		uint8_t currentIV[blockSize];
		uint8_t nextIV[blockSize];
		uint8_t workVector[blockSize];

		for(int i=0; i<blockSize; ++i)
		{
			currentIV[i] = iv[i];	
		}

		for(int b=0; b<blocks; ++b)
		{			
			// get next cipher block and use ciphertext as next IV
			for(int i=0; i<blockSize; ++i)
			{
				workVector[i] = cipher[b*blockSize + i];
				nextIV[i] = workVector[i];	
			}

			aes256_decrypt_ecb(&ctx, workVector);
			
			// write to plaintext, XOR work vector with IV
			for(int i=0; i<blockSize; ++i)
			{
				workVector[i] ^= currentIV[i];
				plaintext[b*blockSize + i] = workVector[i];
				currentIV[i] = nextIV[i];
			}
		}
		
		aes256_done(&ctx);
		
		// remove PKCS padding
		int paddedSize = plaintext.size();
		if(paddedSize % blockSize == 0)
		{
			int padBytes = plaintext[paddedSize - 1];
			if((padBytes <= 16) && (padBytes < paddedSize))
			{
				plaintext.resize(paddedSize - padBytes);
			}
		}
		
		return plaintext;
	}
	
	bool collate(const TextFragment& a, const TextFragment& b)
	{
		auto ia = a.begin();
		auto ib = b.begin();
		
		int iterEnds = 0;		
		while(!iterEnds)
		{
			codepoint_type ca = *ia;
			codepoint_type cb = *ib;
			
			if(!utf::internal::validate_codepoint(ca)) return false;
			if(!utf::internal::validate_codepoint(cb)) return false;

			if(ca != cb)
			{
				// the code points differ.
				// compare codepoints, produce a result and bail.
				if(isLatin(ca) && isLatin(cb))
				{
					char la = tolower(ca);
					char lb = tolower(cb);
					
					if(la != lb)
					{
						// different letters
						return la < lb;
					}
					else
					{
						// different cases but same letter. define lower case as less within letter.
						return ca > cb;
					}
				}
				else
				{
					// TODO collate other languages better using miniutf library.
					return ca < cb;
				}
			}
			else
			{
				++ia;
				++ib;
			}
			
			int aEnd = (ia == a.end());
			int bEnd = (ib == b.end());
			iterEnds = (aEnd << 1) | bEnd;
		}
		
		switch(iterEnds)
		{
			case 1: // b ended but not a: a > b.
				return false;
			case 2: // a ended but not b: a < b.
				return true;
			case 3: // both ended, a == b.
			default: // impossible
				return false;
		}
		return false;
	}

#pragma mark Symbol utilities
	
	Symbol addFinalNumber(Symbol sym, int n)
	{
		TextFragment t(sym.getTextFragment(), textUtils::naturalNumberToText(n));
		return Symbol(t.getText());
	}
	
	Symbol stripFinalNumber(Symbol sym)
	{
		// make temporary buffer on stack  
		const TextFragment& frag = sym.getTextFragment();
		int points = frag.lengthInCodePoints();
		char32_t buf[points + 1];
		
		// read into char32 array for random access
		int i=0;
		for (codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return Symbol();
			buf[i++] = c;
		}			
		
		// null terminate
		buf[points] = 0;
		
		// no final number? return
		if(!textUtils::isDigit(buf[points - 1])) return sym;
		
		// read backwards until non-digit
		int firstDigitPos = 0;
		for(int i=points - 2; i >= 0; --i)
		{
			char32_t c = buf[i];
			if(!textUtils::isDigit(c))
			{
				firstDigitPos = i + 1; break;
			}
		}
		
		ml::TextFragment subFrag(textUtils::subText(frag, 0, firstDigitPos));
		return subFrag.getText();
	}
	
	// if the symbol's text ends in a positive integer, return that number. Otherwise return 0.
	int getFinalNumber(Symbol sym)
	{		
		// make temporary buffer on stack  
		const TextFragment& frag = sym.getTextFragment();
		int points = frag.lengthInCodePoints();
		char32_t buf[points + 1];

		// read into char32 array for random access
		int i=0;
		for (codepoint_type c : frag) 
		{
			if(!utf::internal::validate_codepoint(c)) return 0;
			buf[i++] = c;
		}					
		
		// null terminate
		buf[points] = 0;
		
		// no final number? return
		if(!textUtils::isDigit(buf[points - 1])) return 0;
		
		// read backwards until non-digit
		int firstDigitPos = 0;
		for(int i=points - 2; i >= 0; --i)
		{
			char32_t c = buf[i];
			if(!textUtils::isDigit(c))
			{
				firstDigitPos = i + 1; break;
			}
		}

		// note, null terminated char32_t string needed
		int r = digitsToNaturalNumber(buf + firstDigitPos);		
		return r;
	}

	Symbol stripFinalCharacter(Symbol sym)
	{
		TextFragment frag = sym.getTextFragment();
		int len = frag.lengthInCodePoints();
		return Symbol(subText(frag, 0, len - 1));
	}
	
#pragma mark NameMaker
	
	// base-26 arithmetic with letters (A = 0) produces A, B, ... Z, BA, BB ...
	const Symbol NameMaker::nextName()
	{
		std::vector<int> digits;
		const int base = 26;
		const char baseChar = 'A';
		int a, m, d, rem;
		
		a = index++;
		
		if (!a)
		{
			digits.push_back(0);
		}
		else while(a)
		{
			d = a / base;
			m = d * base;
			rem = a - m;
			digits.push_back(rem);
			a = d;
		}
		
		int c = 0;
		while(digits.size() && (c < maxLen - 1))
		{
			d = digits.back();
			digits.pop_back();
			
			buf[c++] = static_cast<char>(d) + baseChar;
		}
		
		buf[c++] = 0;
		return Symbol(buf);
	}
	
	static const char kLetters[33] = "aabcdeefghijklmnnoopqrssttuvwxyz";
	std::vector<Symbol> vectorOfNonsenseSymbols( int len )
	{
		ml::RandomSource randSource;
		std::vector<Symbol> words;
		for(int i = 0; i < len; ++i)
		{
			std::string newStr;
			uint32_t r32 = randSource.getIntSample() >> 16;
			int wordLen = (r32 & 7) + 3;
			
			for(int j=0; j<wordLen; ++j)
			{
				r32 = randSource.getIntSample() >> 16;
				int idx = (r32 & 31);
				newStr += (kLetters[idx]);
			}
			words.push_back(Symbol(newStr.c_str()));
		}
		return words;
	}
} } // ml:textUtils
