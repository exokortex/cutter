#ifndef PPCUTTER_H
#define PPCUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#include <memory>

#include <pp/types.h>
#include <pp/objectdisassembler.h>

#define PPCore() (PPCutterCore::getInstance())

struct PPFile;
struct PPFunction;

struct PPFile
{
    std::string name;
    std::vector<std::unique_ptr<PPFunction>> functions;
};

struct PPFunction
{
    std::string name;
};

class PPCutterCore: public QObject
{
    Q_OBJECT
    friend class ppccClass;

private:
    std::unique_ptr<DisassemblerState> state;
    std::unique_ptr<ObjectDisassembler> objDis;

public:
    explicit PPCutterCore(QObject *parent = 0);
    ~PPCutterCore();
    static PPCutterCore *getInstance();

    void loadFile(QString path);
    void saveProject();

    inline const DisassemblerState &getState() const {
        return *state;
    }
    
    inline const ObjectDisassembler &getObjDis() const {
        return *objDis;
    }

signals:

public slots:

};

class ppccClass : public PPCutterCore
{
};

#endif // PPCUTTER_H
