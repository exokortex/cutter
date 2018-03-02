#ifndef PPCUTTER_H
#define PPCUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#include <memory>

#include "rapidjson/document.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>

#include <pp/types.h>
#include <pp/objectdisassembler.h>
#include <nlohmann/json.hpp>

#define PPCore() (PPCutterCore::getInstance())

using json = nlohmann::json;


struct PPFile;
struct PPFunction;
struct PPAnnotation;

enum PPAnnotationType {
    ENTRYPOINT,
    INST_TYPE
};

struct PPFile
{
    std::string path;
    std::string md5sum;
    std::vector<PPFunction> functions;
    std::vector<PPAnnotation> annotations;
};

struct PPFunction
{
    std::string name;
    AddressType offset;
    SizeType size;
    std::string md5sum;
};

struct PPAnnotation
{
    AddressType offset;
    PPAnnotationType type;
    std::string comment;
    json data;

    PPAnnotation() {
    }

    PPAnnotation(AddressType _offset, PPAnnotationType _type, std::string _comment, json _data) {
        offset = _offset;
         type = _type;
        comment = _comment;
        data = _data;
    }
};

Q_DECLARE_METATYPE(PPAnnotation)

class PPCutterCore: public QObject
{
    Q_OBJECT
    friend class ppccClass;

private:
    std::unique_ptr<DisassemblerState> state;
    std::unique_ptr<ObjectDisassembler> objDis;
    std::unique_ptr<PPFile> file;
    rapidjson::Document jsonDocument;

public:
    explicit PPCutterCore(QObject *parent = 0);
    ~PPCutterCore();
    static PPCutterCore *getInstance();

    static InstructionType parseInstructionType(const std::string iType);
    static std::string instructionTypeToString(const InstructionType iType);
    static std::string annotationTypeToString(const PPAnnotationType aType);

    void loadFile(QString path);
    void saveProject();

    inline const DisassemblerState &getState() const {
        return *state;
    }

    inline const ObjectDisassembler &getObjDis() const {
        return *objDis;
    }

    inline const PPFile &getFile() const {
        return *file;
    }

signals:

public slots:

};

class ppccClass : public PPCutterCore
{
};

#endif // PPCUTTER_H
