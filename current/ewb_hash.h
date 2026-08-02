#ifndef EWB_HASH_H
#define EWB_HASH_H

#include <string>

std::string ewbMd5(const std::string& input);
std::string ewbSha256(const std::string& input);

#endif
