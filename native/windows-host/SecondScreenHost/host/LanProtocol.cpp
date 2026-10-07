#include "LanProtocol.h"
namespace second_screen {
static void put16(std::vector<uint8_t>& o,uint16_t v){o.push_back(v>>8);o.push_back(v);}
static void put32(std::vector<uint8_t>& o,uint32_t v){o.push_back(v>>24);o.push_back(v>>16);o.push_back(v>>8);o.push_back(v);}
static void put64(std::vector<uint8_t>& o,uint64_t v){for(int i=7;i>=0;--i)o.push_back(static_cast<uint8_t>(v>>(i*8)));}
std::vector<uint8_t> makeControlFrame(MessageType t,const std::string& json){
 std::vector<uint8_t> o; o.reserve(12+json.size()); o.insert(o.end(),{'S','S','C','P'});
 o.push_back(1); o.push_back(static_cast<uint8_t>(t)); put16(o,0); put32(o,(uint32_t)json.size());
 o.insert(o.end(),json.begin(),json.end()); return o;
}
std::vector<uint8_t> makeVideoFrame(uint8_t codec,bool key,uint64_t ts,const std::vector<uint8_t>& a){
 std::vector<uint8_t> o; o.reserve(20+a.size()); o.insert(o.end(),{'S','S','V','F'});
 o.push_back(codec); o.push_back(key?1:0); put16(o,0); put64(o,ts); put32(o,(uint32_t)a.size());
 o.insert(o.end(),a.begin(),a.end()); return o;
}
}