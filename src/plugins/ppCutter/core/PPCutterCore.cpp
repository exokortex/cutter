#include <QJsonArray>
#include <QJsonObject>

#include <llvm-c/Target.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <pp/ElfPatcher.h>
#include <pp/StateUpdateFunctions/crc/CrcStateUpdateFunction.hpp>
#include <pp/StateUpdateFunctions/prince_ape/PrinceApeStateUpdateFunction.hpp>
#include <pp/StateUpdateFunctions/sum/SumStateUpdateFunction.hpp>
#include <pp/architecture/riscv/info.h>
#include <pp/architecture/riscv/replace_instructions.h>
#include <pp/architecture/thumbv7m/info.h>
#include <pp/basicblock.h>
#include <pp/config.h>
#include <pp/disassemblerstate.h>
#include <pp/exception.h>
#include <pp/logger.h>
#include <pp/types.h>
#include <pp/function.h>
#include <pp/config.h>

#include "plugins/ppCutter/core/PPCutterCore.h"
#include "Cutter.h"

Q_GLOBAL_STATIC(ppccClass, uniqueInstance)

PPCutterCore::PPCutterCore(QObject *parent) :
        QObject(parent)
{
    addAnnotationType(AT_ENTRYPOINT, "entrypoint");
    addAnnotationType(AT_INST_TYPE, "inst_type");
    addAnnotationType(AT_LOAD_REF, "load_ref");
}

PPCutterCore *PPCutterCore::getInstance()
{
    return uniqueInstance;
}

PPCutterCore::~PPCutterCore()
{
}

void PPCutterCore::loadFile(std::string path)
{
    file = std::make_unique<PPBinaryFile>(path);
    file->disassemble();
    ready = true;
}

std::set<const ::BasicBlock*> PPCutterCore::getBasicBlocksOfFunction(::Function& function, AddressType entrypointAddress)
{
    std::set<const ::BasicBlock*> res;
    for (auto& frag : function) {
        //const ::Fragment* frag = *it;
        if (frag->getStartAddress() == entrypointAddress) {
            const BasicBlock *bb = llvm::dyn_cast_or_null<BasicBlock>(frag);
            if (bb == nullptr)
                continue;
            getSuccessorsRecursive(res, *bb);
            break;
        }
    }
    return res;
}

void PPCutterCore::getSuccessorsRecursive(std::set<const ::BasicBlock*>& collection, const ::BasicBlock& bb)
{
    // if it was not visited before, add all successors
    if (collection.insert(&bb).second) {
        for (auto successor = bb.succ_begin(); successor < bb.succ_end(); successor++)
        {
            getSuccessorsRecursive(collection, **successor);
        }
    }
}

void PPCutterCore::addAnnotationType(AnnotationType type, std::string str)
{
    annotationTypeToStringMap[type] = str;
    stringToAnnotationTypeMap[str] = type;
}

InstructionType PPCutterCore::parseInstructionType(const std::string iType)
{
    if (iType == "unknown")
        return UNKNOWN;
    else if (iType == "sequential")
        return SEQUENTIAL;
    else if (iType == "call.direct")
        return DIRECT_CALL;
    else if (iType == "call.indirect")
        return INDIRECT_CALL;
    else if (iType == "return")
        return RETURN;
    else if (iType == "trap")
        return TRAP;
    else if (iType == "branch.direct")
        return DIRECT_BRANCH;
    else if (iType == "branch.indirect")
        return INDIRECT_BRANCH;
    else if (iType == "branch.conditional")
        return COND_BRANCH;
    //assert(false && "missing case");
    return UNKNOWN;
}

std::string PPCutterCore::instructionTypeToString(const InstructionType iType)
{
    if (iType == UNKNOWN)
        return "unknown";
    else if (iType == SEQUENTIAL)
        return "sequential";
    else if (iType == DIRECT_CALL)
        return "call.direct";
    else if (iType == INDIRECT_CALL)
        return "call.indirect";
    else if (iType == RETURN)
        return "return";
    else if (iType == TRAP)
        return "trap";
    else if (iType == DIRECT_BRANCH)
        return "branch.direct";
    else if (iType == INDIRECT_BRANCH)
        return "branch.indirect";
    else if (iType == COND_BRANCH)
        return "branch.conditional";
    //assert(false && "missing case");
    return "ERROR";
}

std::string PPCutterCore::annotationTypeToString(const AnnotationType aType)
{
    if (annotationTypeToStringMap.count(aType))
        return annotationTypeToStringMap[aType];
    else
        return "ERROR";
}

AnnotationType PPCutterCore::annotationTypeFromString(const std::string str)
{
    if (stringToAnnotationTypeMap.count(str))
        return stringToAnnotationTypeMap[str];
    else
        return AT_INVALID;
}

UpdateType PPCutterCore::updateTypeFromString(const std::string str)
{
    if (str =="INVALID") {
        return UpdateType::INVALID;
    } else if (str == "CONSTANT_LOAD") {
        return UpdateType::CONSTANT_LOAD;
    } else if (str == "SIGNATURE_LOAD") {
        return UpdateType::SIGNATURE_LOAD;
    } else if (str == "CONST_INJECTION") {
        return UpdateType::CONST_INJECTION;
    }
}


//void PPCutterCore::applyAnnotations()
//{
//    std::cout << "applying annotations: " << file->asJson() << std::endl;
//    for (auto addr : file->annotations) {
//        std::cout << "adding annotation @ " << std::hex << addr.first << std::endl;
//        for (auto& type : addr.second) {
//            PPAnnotation& annotation = type.second;
//            switch (type.first) {
//                case ENTRYPOINT:
//                {
//                    std::cout << "type: ENTRYPOINT, name=" << annotation.data["name"] << std::endl;
//                    state->defineFunction(annotation.offset, annotation.data["name"]);
//                    break;
//                }
//
//                case INST_TYPE:
//                {
//                    std::cout << "type: INST_TYPE, itype=" << annotation.data["itype"] << std::endl;
//                    DecodedInstruction& di = const_cast<DecodedInstruction&>(objDis->disassembleAddr(*state, annotation.offset));
//                    di.type = parseInstructionType(annotation.data["itype"]);
//                    state->defineFunction(annotation.offset, annotation.data["name"]);
//                    break;
//                }
//
//                default:
//                    std::cout << "type: unknown" << std::endl;
//                    break;
//            }
//        }
//    }
//}

void PPCutterCore::disassembleAll()
{
    Core()->cmd("e asm.bits=16");
    file->disassemble();
}

void PPCutterCore::calculateAll()
{
    file->calculateStates();
}

void PPCutterCore::loadProject(std::string filepath)
{
    file->setAnnotations(AnnotationsIO::loadAnnotationsFromFile(*file->state, filepath));
    file->disassemble();
}

void PPCutterCore::saveProject(std::string filepath)
{
    AnnotationsIO::saveAnnotationsToFile(*file->state, filepath, file->getAnnotations());
}

void PPCutterCore::registerAnnotationChange()
{
    emit annotationsChanged();
}

//QString PPCutterCore::jsonToQString(json json)
//{
//    QString data = "";
//    for (auto it = json.begin(); it != json.end(); ++it) {
//        if (data != "")
//        data += ", ";
//        std::string key = it.key();
//        std::string value = it.value();
//        data += QString::fromUtf8(key.c_str());
//        data += "=";
//        data += QString::fromUtf8(value.c_str());
//    }
//    return data;
//}

//void PPCutterCore::updateAnnotation(AddressType addr, json data)
//{
//    for (json::iterator it = data.begin(); it != data.end(); ++it) {
//        std::string name = it.key();
//        PPAnnotationType type = annotationTypeFromString(name);
//        switch (type) {
//            case ENTRYPOINT:
//                if (!it.value()["name"].is_string())
//                    assert(false);
//                file->state->entrypoint_annotations[addr].name = it.value()["name"];
//                break;
//            case INST_TYPE:
//                //file->state.entrypoint_annotations[addr].name = data["name"];
//                break;
//            default:
//                assert(false);
//                break;
//        }
//    }
//    registerAnnotationChange();
//}

QString PPCutterCore::addrToString(AddressType addr)
{
    return QString::asprintf("0x%08x", addr);
}

AddressType PPCutterCore::strToAddress(QString qstr, bool* ok)
{
    if (qstr.startsWith("0x")) {
        return static_cast<AddressType>(qstr.toULong(ok, 16));
    } else {
        return static_cast<AddressType>(qstr.toULong(ok, 10));
    }
}
