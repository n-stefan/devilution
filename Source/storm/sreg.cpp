#define NOMINMAX
#include "storm.h"
#include <string>
#include <unordered_map>
#include "../rmpq/file.h"

class Registry {
public:
  Registry(const char* path)
    : path_(path)
  {
    File f(path);
    if (f) {
      std::unordered_map<std::string, std::string>* cat = nullptr;
      for (auto line : f) {
        if (line[0] == '[') {
          size_t end = line.find(']');
          if (end != std::string::npos) {
            std::string name = line.substr(1, end - 1);
            cat = &values[name];
          }
        } else if (cat) {
          size_t mid = line.find('=');
          if (mid != std::string::npos) {
            std::string name = line.substr(0, mid);
            (*cat)[name] = line.substr(mid + 1);
          }
        }
      }
    }
  }

  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> values;

  void flush() {
    File fo(path_, "wt");
    for (auto& cat : values) {
      fo.printf("[%s]\n", cat.first.c_str());
      for (auto& val : cat.second) {
        fo.printf("%s=%s\n", val.first.c_str(), val.second.c_str());
      }
    }
  }

  static Registry& instance() {
    static Registry inst("config.ini");
    return inst;
  }

  const char* get(const char *keyname, const char *valuename) {
    auto it1 = values.find(keyname);
    if (it1 != values.end()) {
      auto it2 = it1->second.find(valuename);
      if (it2 != it1->second.end()) {
        return it2->second.c_str();
      }
    }
    return nullptr;
  }

private:
  std::string path_;
};

BOOL  SRegLoadString(const char *keyname, const char *valuename, BYTE flags, char *buffer, unsigned int buffersize) {
  auto value = Registry::instance().get(keyname, valuename);
  if (value) {
    SStrCopy(buffer, value, buffersize);
    return TRUE;
  }
  return FALSE;
}

BOOL  SRegLoadValue(const char *keyname, const char *valuename, BYTE flags, int *result) {
  auto value = Registry::instance().get(keyname, valuename);
  if (value) {
    *result = atoi(value);
    return TRUE;
  }
  return FALSE;
}

BOOL  SRegSaveString(const char *keyname, const char *valuename, BYTE flags, char *string) {
  Registry::instance().values[keyname][valuename] = string;
  Registry::instance().flush();
  return TRUE;
}

BOOL  SRegSaveValue(const char *keyname, const char *valuename, BYTE flags, DWORD result) {
  Registry::instance().values[keyname][valuename] = std::to_string(result);
  Registry::instance().flush();
  return TRUE;
}

BOOL  SRegDeleteValue(const char *keyname, const char *valuename, BYTE flags) {
  Registry::instance().values[keyname].erase(valuename);
  Registry::instance().flush();
  return TRUE;
}
