#ifndef PPCUTTER_H
#define PPCUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#include <memory>


#include "PPBinaryFile.h"

#define PPCore() (PPCutterCore::getInstance())



class PPCutterCore: public QObject
{
    Q_OBJECT
    friend class ppccClass;

private:

    std::unique_ptr<PPBinaryFile> file;

    bool ready;
    std::map<Annotation::Type, std::string> annotationTypeToStringMap;
    std::map<std::string, Annotation::Type> stringToAnnotationTypeMap;
    void addAnnotationType(Annotation::Type, std::string);

    void applyAnnotations();
    void disassemble();

public:
    explicit PPCutterCore(QObject *parent = 0);
    ~PPCutterCore();
    static PPCutterCore *getInstance();

    void loadFile(std::string path);

    void disassembleAll();
    void calculateAll();
    void saveProject(std::string filepath);
    void loadProject(std::string filepath);

    static QString toString(const UpdateType updateType);
    static QString addrToString(const AddressType addr);

    static InstructionType parseInstructionType(const std::string iType);
    static std::string instructionTypeToString(const InstructionType iType);

    static AddressType strToAddress(QString qstr, bool* ok = nullptr);

    std::string annotationTypeToString(const Annotation::Type aType);
    Annotation::Type annotationTypeFromString(const std::string str);

    static std::string updateTypeToString(const UpdateType updateType);
    static UpdateType updateTypeFromString(const std::string str);


    std::set<const ::BasicBlock*> getBasicBlocksOfFunction(::Function& function, AddressType entrypointAddress);
    void getSuccessorsRecursive(std::set<const ::BasicBlock*>& collection, const ::BasicBlock& fragment);

    std::map<Annotation::Type, std::string>& getAnnotationTypes() {
        return annotationTypeToStringMap;
    };


    inline bool isReady() {
        return ready;
    }

    inline const DisassemblerState &getState() const {
        return *file->state;
    }

    inline const ObjectDisassembler &getObjDis() const {
        return *file->objDis;
    }

    inline PPBinaryFile& getFile() const {
        return *file;
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
