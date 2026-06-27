#include <iostream>
#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (std::getline(ss, item, delim)) {
    result.push_back(item);
  }

  return result;
}

bool starts_with(const std::string &str, const std::string &sub,
                 bool ignore_case = false) {
  int str_len = str.size();
  int sub_len = sub.size();
  if (str_len < sub_len)
    return false;

  if (ignore_case) {
    return std::equal(sub.begin(), sub.end(), str.begin(), [](char a, char b) {
      return std::tolower(a) == std::tolower(b);
    });
  }
  return str.compare(0, sub_len, sub) == 0;
}
