#ifndef PPCUTTER_H
#define PPCUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#include <memory>

/*
#include "rapidjson/document.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
*/

#include <pp/types.h>
#include <pp/StateCalculators/AEE/ApeStateCalculator.h>
#include <pp/StateCalculators/PureSw/PureSwUpdateStateCalculator.h>
#include <pp/objectdisassembler.h>
#include <nlohmann/json.hpp>

#include "PPFile.h"

#define PPCore() (PPCutterCore::getInstance())

class PPCutterCore: public QObject
{
    Q_OBJECT
    friend class ppccClass;

private:
    std::unique_ptr<DisassemblerState> state;
    std::unique_ptr<ObjectDisassembler> objDis;
    std::unique_ptr<StateCalculator> stateCalc;
    std::unique_ptr<PPFile> file;
    bool ready;
    const json defaultAnnotationData = R"({
        "entrypoint": {
            "name": ""
          },
        "inst_type": {
            "itype": "call.direct"
          },
        "load_ref": {
            "addr": "0x80000000"
          }
      })"_json;
    std::map<PPAnnotationType, std::string> annotationTypeToStringMap;
    std::map<std::string, PPAnnotationType> stringToAnnotationTypeMap;
    void addAnnotationType(PPAnnotationType, std::string);

    void applyAnnotations();
    void disassemble();

public:
    explicit PPCutterCore(QObject *parent = 0);
    ~PPCutterCore();
    static PPCutterCore *getInstance();

    static InstructionType parseInstructionType(const std::string iType);
    static std::string instructionTypeToString(const InstructionType iType);
    std::string annotationTypeToString(const PPAnnotationType aType);
    PPAnnotationType annotationTypeFromString(const std::string str);

    static QString jsonToQString(json json);

    void fullRedo();

    void updateAnnotation(AddressType addr, json data);

    std::map<PPAnnotationType, std::string>& getAnnotationTypes() {
        return annotationTypeToStringMap;
    };

    void loadFile(QString path);
    void saveProject();

    inline bool isReady() {
        return ready;
    }

    inline const DisassemblerState &getState() const {
        return *state;
    }

    inline const ObjectDisassembler &getObjDis() const {
        return *objDis;
    }

    inline const PPFile& getFile() const {
        return *file;
    }

    inline const json& getDefaultAnnotationData() {
        return defaultAnnotationData;
    }

    void registerAnnotationChange();

signals:
    void annotationsChanged();

public slots:

};

class ppccClass : public PPCutterCore
{
};

#endif // PPCUTTER_H
