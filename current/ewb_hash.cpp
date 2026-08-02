#include "ewb_hash.h"

#include <cstdint>
#include <vector>

static uint32_t rotateLeft(uint32_t value,unsigned amount)
{ return (value<<amount) | (value>>(32-amount));
}

static uint32_t rotateRight(uint32_t value,unsigned amount)
{ return (value>>amount) | (value<<(32-amount));
}

static std::string digestHex(const std::vector<uint32_t>& words,bool littleEndian)
{ static const char digits[]="0123456789abcdef";
  std::string result;
  result.reserve(words.size()*8);
  for (size_t i=0;i<words.size();i++)
    for (int byte=0;byte<4;byte++)
    { unsigned shift=littleEndian ? byte*8 : (3-byte)*8;
      unsigned value=(words[i]>>shift)&0xff;
      result.push_back(digits[value>>4]);
      result.push_back(digits[value&15]);
    }
  return result;
}

std::string ewbMd5(const std::string& input)
{ static const uint32_t constants[64]={
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391};
  static const unsigned shifts[16]={7,12,17,22,5,9,14,20,4,11,16,23,6,10,15,21};
  std::vector<uint8_t> message(input.begin(),input.end());
  uint64_t bitLength=(uint64_t)message.size()*8;
  message.push_back(0x80);
  while (message.size()%64!=56) message.push_back(0);
  for (int i=0;i<8;i++) message.push_back((uint8_t)(bitLength>>(8*i)));

  uint32_t a0=0x67452301,b0=0xefcdab89,c0=0x98badcfe,d0=0x10325476;
  for (size_t offset=0;offset<message.size();offset+=64)
  { uint32_t words[16];
    for (int i=0;i<16;i++)
      words[i]=(uint32_t)message[offset+4*i] | ((uint32_t)message[offset+4*i+1]<<8) |
               ((uint32_t)message[offset+4*i+2]<<16) | ((uint32_t)message[offset+4*i+3]<<24);
    uint32_t a=a0,b=b0,c=c0,d=d0;
    for (unsigned i=0;i<64;i++)
    { uint32_t f,g;
      unsigned shiftGroup=i/16;
      if (i<16) { f=(b&c)|(~b&d); g=i; }
      else if (i<32) { f=(d&b)|(~d&c); g=(5*i+1)%16; }
      else if (i<48) { f=b^c^d; g=(3*i+5)%16; }
      else { f=c^(b|~d); g=(7*i)%16; }
      uint32_t next=d;
      d=c; c=b;
      b=b+rotateLeft(a+f+constants[i]+words[g],shifts[shiftGroup*4+i%4]);
      a=next;
    }
    a0+=a; b0+=b; c0+=c; d0+=d;
  }
  return digestHex(std::vector<uint32_t>{a0,b0,c0,d0},true);
}

std::string ewbSha256(const std::string& input)
{ static const uint32_t constants[64]={
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
  std::vector<uint8_t> message(input.begin(),input.end());
  uint64_t bitLength=(uint64_t)message.size()*8;
  message.push_back(0x80);
  while (message.size()%64!=56) message.push_back(0);
  for (int i=7;i>=0;i--) message.push_back((uint8_t)(bitLength>>(8*i)));

  uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                 0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  for (size_t offset=0;offset<message.size();offset+=64)
  { uint32_t w[64];
    for (int i=0;i<16;i++)
      w[i]=((uint32_t)message[offset+4*i]<<24) | ((uint32_t)message[offset+4*i+1]<<16) |
           ((uint32_t)message[offset+4*i+2]<<8) | message[offset+4*i+3];
    for (int i=16;i<64;i++)
    { uint32_t s0=rotateRight(w[i-15],7)^rotateRight(w[i-15],18)^(w[i-15]>>3);
      uint32_t s1=rotateRight(w[i-2],17)^rotateRight(w[i-2],19)^(w[i-2]>>10);
      w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i=0;i<64;i++)
    { uint32_t s1=rotateRight(e,6)^rotateRight(e,11)^rotateRight(e,25);
      uint32_t choice=(e&f)^(~e&g);
      uint32_t temp1=hh+s1+choice+constants[i]+w[i];
      uint32_t s0=rotateRight(a,2)^rotateRight(a,13)^rotateRight(a,22);
      uint32_t majority=(a&b)^(a&c)^(b&c);
      uint32_t temp2=s0+majority;
      hh=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
    h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
  }
  return digestHex(std::vector<uint32_t>(h,h+8),false);
}
