#ifndef EWB_VALUES_H
#define EWB_VALUES_H

#include <string>

void ewbRaise(const std::string& message);

bool ewbIsNumber(const std::string& value);
int ewbIntValue(const std::string& value);

std::string ewbSum(const std::string& left, const std::string& right);
std::string ewbSub(const std::string& left, const std::string& right);
std::string ewbMul(const std::string& left, const std::string& right);
std::string ewbDiv(const std::string& left, const std::string& right);
std::string ewbNegative(const std::string& value);

std::string ewbOr(const std::string& left, const std::string& right);
std::string ewbAnd(const std::string& left, const std::string& right);
std::string ewbBitwiseOr(const std::string& left, const std::string& right);
std::string ewbBitwiseAnd(const std::string& left, const std::string& right);
std::string ewbBitwiseNot(const std::string& value);

std::string ewbCompare(const std::string& left, const std::string& right, const std::string& op);

#endif
