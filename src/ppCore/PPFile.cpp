#include "PPFile.h"

PPFile::~PPFile() {
//  for (auto addr : annotations) {
//    for (auto type : addr.second) {
//      delete type.second;
//    }
//  }
}

nlohmann::json PPFile::asJson() {
  json j = {
          {"path", path},
          {"md5sum", "00000000"}
  };

  for (auto& addr : annotations) {
    for (auto& type : addr.second) {
      j["annotations"].push_back(type.second.data);
    }
  }

  return j;
}
