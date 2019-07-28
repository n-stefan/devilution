#pragma once

#include <string>

namespace __Trace {

static struct __null_type {} __null_value;

template<class T>
std::string _to_string(const char* prefix, T value) {
  return prefix + std::to_string(value);
}

template<>
inline std::string _to_string(const char* prefix, const char* value) {
  std::string str(prefix);
  str.push_back('"');
  str.append(value);
  str.push_back('"');
  return str;
}

template<>
inline std::string _to_string(const char* prefix, char* value) {
  std::string str(prefix);
  str.push_back('"');
  str.append(value);
  str.push_back('"');
  return str;
}

template<>
inline std::string _to_string(const char* prefix, __null_type) {
  return "";
}

inline const char* _to_string(const char* prefix) {
  return "";
}

struct Guard {
  Guard(const char* name, const std::string& args);
  ~Guard();
};

}

#ifdef _DEBUG
#define __EXPAND(x) x
#define __ARG(...) __Trace::_to_string("\n\t" #__VA_ARGS__ " = ", ## __VA_ARGS__)
#define __STRINGIFY(x) #x
#define __CONCAT(x, y) x ## y
#define __TOSTRING(x) __STRINGIFY(x)
#define __VAR_NAME(x) __CONCAT(__trace_, x)
#define __LOG_STACK(args) __Trace::Guard __VAR_NAME(__LINE__)(__FILE__ ":" __TOSTRING(__LINE__), args)
#define __LOG_STACK_ARGS(a, b, c, d, e, f, g, h, i, j, ...) __LOG_STACK(__ARG(a) + __ARG(b) + __ARG(c) + __ARG(d) + __ARG(e) + __ARG(f) + __ARG(g) + __ARG(h) + __ARG(i) + __ARG(j))
#define LOG_STACK(...) __EXPAND(__LOG_STACK_ARGS(__VA_ARGS__, __Trace::__null_value, \
  __Trace::__null_value, __Trace::__null_value, __Trace::__null_value, __Trace::__null_value, __Trace::__null_value, \
  __Trace::__null_value, __Trace::__null_value, __Trace::__null_value, __Trace::__null_value, __Trace::__null_value))
#else
#define LOG_STACK(...)
#endif
