#include <QJsonArray>
#include <QJsonObject>
#include "PPCutterCore.h"

#include "rapidjson/document.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>

using namespace rapidjson;

Q_GLOBAL_STATIC(ppccClass, uniqueInstance)

PPCutterCore::PPCutterCore(QObject *parent) :
    QObject(parent)
{
}

PPCutterCore *PPCutterCore::getInstance()
{
    return uniqueInstance;
}

PPCutterCore::~PPCutterCore()
{
}

void PPCutterCore::saveProject()
{
    // create new document
    Document d;
    d.SetObject();
    Document::AllocatorType& alloc = d.GetAllocator();

    // add members
    d.AddMember("name", "TestProject", alloc);
    d.AddMember("ppProjectFormatVersion", "0.0.1-alpha", alloc);

    Value files(kArrayType);

    Value file(kObjectType);

    file.AddMember("path", "path/to/elf/file", alloc);
    file.AddMember("md5sum", "ef07fe90d5900ca8716e23a58de04ece", alloc);

    Value functions(kArrayType);

    for (int i = 0; i <= 3; i++) {
        Value function(kObjectType);
        function.AddMember("name", "main", alloc);
        function.AddMember("offset", 7512, alloc);
        function.AddMember("size", 382, alloc);
        function.AddMember("md5sum", "ef07fe90d5900ca8716e23a58de04ece", alloc);

        Value annotations(kArrayType);

        for (int j = 0; j <= 3; j++) {
            Value annotation(kObjectType);
            annotation.AddMember("offset", 7512, alloc);
            annotation.AddMember("name", "main", alloc);
            annotation.AddMember("type", "entrypoint", alloc);
            annotations.PushBack(annotation, alloc);
        }

        function.AddMember("annotations", annotations, alloc);
        functions.PushBack(function, alloc);
    }

    file.AddMember("functions", functions, alloc);

    files.PushBack(file, alloc);

    d.AddMember("files", files, alloc);


    // write to file
    std::ofstream ofs("output.json");
    OStreamWrapper osw(ofs);
    PrettyWriter<OStreamWrapper> writer(osw);
    d.Accept(writer);
}
