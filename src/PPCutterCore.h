#ifndef PPCUTTER_H
#define PPCUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#define PPCore() (PPCutterCore::getInstance())

class PPCutterCore: public QObject
{
    Q_OBJECT
    friend class ppccClass;

public:
    explicit PPCutterCore(QObject *parent = 0);
    ~PPCutterCore();
    static PPCutterCore *getInstance();

signals:

public slots:

private:

};

class ppccClass : public PPCutterCore
{
};

#endif // PPCUTTER_H
