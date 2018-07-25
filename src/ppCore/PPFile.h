#ifndef PPFILE_H
#define PPFILE_H

#include <QVariant>
#include <nlohmann/json.hpp>
#include <pp/types.h>

using json = nlohmann::json;

class PPFile;
class PPFunction;
struct PPAnnotation;

enum PPAnnotationType {
  ENTRYPOINT = 0,
  INST_TYPE,
  LOAD_REF
};

struct PPAnnotation
{
    AddressType offset;
    PPAnnotationType type;
    json data;

    PPAnnotation() {
    }

    PPAnnotation(AddressType _offset, PPAnnotationType _type, json _data)
            : offset(_offset), type(_type), data(_data) {}
};

Q_DECLARE_METATYPE(PPAnnotation)

//class PPFunction
//{
//  std::string name;
//  AddressType offset;
//  SizeType size;
//  std::string md5sum;
//};

class PPFile
{
  public:
    std::string path;
    std::string md5sum;
    //std::map<AddressType, PPFunction*> functions;
    std::map<AddressType, std::map<PPAnnotationType, PPAnnotation>> annotations;

    ~PPFile();

    json asJson();
};




#endif
