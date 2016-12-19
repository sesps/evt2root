
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

class __avalue : public std::string {
  bool is_int, is_double;
public:
  __avalue() : std::string() { is_int = false; is_double = false; }
  __avalue(const std::string& str) : std::string(str)
    { is_int = false; is_double = false; }
  operator const char*() { return c_str(); }
  operator int() { return atoi(c_str()); }
  operator double() { return atof(c_str()); }
  void SetInt() { is_int = true; is_double = false; }
  void SetDouble() { is_int = true; is_double = true; }
  bool IsInt() { return is_int; }
  bool IsDouble() { return is_double; }
  __avalue& operator=(const char* str)
    { assign(str); is_int = false; is_double = false; return *this; }
  __avalue& operator=(const std::string& str)
    { assign(str); is_int = false; is_double = false; return *this; }
  __avalue& operator=(const __avalue& aval)
  {
    assign(aval.c_str());
    is_int = aval.is_int;
    is_double = aval.is_double;
    return *this;
  }
};

struct ValueType : public std::vector<__avalue> {
  ValueType() : std::vector<__avalue>() {}
  ValueType(const std::string& str) : std::vector<__avalue>()
    { push_back(__avalue(str)); }
  ValueType& operator=(const std::string& str)
    { if(empty()) push_back(__avalue(str)); else at(0) = str; return *this; }
  operator __avalue()
    { return at(0); }
  operator const char*()
    { return at(0).c_str(); }
  operator int()
    { return atoi(at(0).c_str()); }
  operator double()
    { return atof(at(0).c_str()); }
  ValueType& operator=(ValueType& VT)
  {
    clear();
    for(iterator i = VT.begin(); i != VT.end(); ++i)
      push_back(*i);

    return *this;
  }
};

inline std::ostream& operator<<(std::ostream& out, const ValueType& val)
{
  if(val.size() != 1)
  {
    out << "[" << val.size() << "] = { ";
    for(unsigned int i = 0; i < val.size(); ++i)
    {
      out << static_cast<std::string>(val[i]);
      if(i != val.size() - 1)
        out <<  ", ";
    }
    return out << " }";
  }
  else
    return out << static_cast<std::string>(val.front());
}

