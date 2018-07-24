#include <QJsonArray>
#include <QJsonObject>

/*
#include "rapidjson/document.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
*/

#include <llvm-c/Target.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
//#include <llvm/Support/TargetRegistry.h>
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

#include "ppCore/PPCutterCore.h"

Q_GLOBAL_STATIC(ppccClass, uniqueInstance)

PPCutterCore::PPCutterCore(QObject *parent) :
    QObject(parent)
{
    addAnnotationType(ENTRYPOINT, "entrypoint");
    addAnnotationType(INST_TYPE, "inst_type");
    addAnnotationType(LOAD_REF, "load_ref");
}

void PPCutterCore::addAnnotationType(PPAnnotationType type, std::string str)
{
    annotationTypeToStringMap[type] = str;
    stringToAnnotationTypeMap[str] = type;
}

PPCutterCore *PPCutterCore::getInstance()
{
    return uniqueInstance;
}

PPCutterCore::~PPCutterCore()
{
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

std::string PPCutterCore::annotationTypeToString(const PPAnnotationType aType)
{
    if (annotationTypeToStringMap.find(aType) != annotationTypeToStringMap.end())
        return annotationTypeToStringMap[aType];
    else
        return "ERROR";
}

PPAnnotationType PPCutterCore::annotationTypeFromString(const std::string str)
{
    return stringToAnnotationTypeMap[str];
}

void PPCutterCore::loadFile(QString path)
{
    auto logger = get_logger();

    file = std::make_unique<PPFile>();

//    updateAnnotation(0x00000128, R"({"entrypoint": {"name":"main"}})"_json);
//    updateAnnotation(0x00000158, R"({"entrypoint": {"name":"__udivsi3"}})"_json);
//    updateAnnotation(0x000001e4, R"({"entrypoint": {"name":"DebugMon_Handler"}})"_json);
//    updateAnnotation(0x00000204, R"({"entrypoint": {"name":"entry0"}})"_json);
//    updateAnnotation(0x000002c0, R"({"entrypoint": {"name":"__aeabi_ldiv0"}})"_json);
//    updateAnnotation(0x000002c8, R"({"entrypoint": {"name":"_exit"}})"_json);

    std::string inputFile = path.toStdString();
    std::cout << "inputFile: " << inputFile << std::endl;
    uint64_t k0 = 0x12345678;
    uint64_t k1 = 0x8765432100000000;
    int rounds = 12;

    auto elf = llvm::make_unique<ELFIO::elfio>();
    if (!elf->load(inputFile))
    {
        std::cout << "PP: File not found" << std::endl;
        std::cerr << "File '" << inputFile << "' not found or it is not an ELF file";
        exit(-1);
    }
    std::cout << "PP: File loaded" << std::endl;

    const ELFIO::Elf_Half machine = elf->get_machine();

    std::cout << "PP: machine (" << machine << ")" << std::endl;

#ifdef ARM_TARGET_ENABLED
    std::cout << "PP: checking for ARM (" << EM_ARM << ")" << std::endl;
    if (machine == EM_ARM) {
      std::cout << "PP: identified ELF as ARM" << std::endl;

      LLVMInitializeARMTargetInfo();
      LLVMInitializeARMTargetMC();
      LLVMInitializeARMDisassembler();

      objDis = llvm::make_unique<ObjectDisassembler>(
          llvm::make_unique<Architecture::Thumb::Info>());
      state = llvm::make_unique<DisassemblerState>(objDis->getInfo());

      std::unique_ptr<StateUpdateFunction> updateFunc;
      if (false) // (cli.m0_)
        updateFunc =
            llvm::make_unique<SumStateUpdateFunction<false, true>>(*state);
      else
        updateFunc =
            llvm::make_unique<CrcStateUpdateFunction<Crc32c<32>, true, true>>(
                *state);

      stateCalc = llvm::make_unique<PureSwUpdateStateCalculator>(
          *state, std::move(updateFunc));
      stateCalc->definePreState(objDis->getInfo().sanitize(elf->get_entry()),
                                CryptoState{4});
    }
#endif // ARM_TARGET_ENABLED
#ifdef RISCV_TARGET_ENABLED
    std::cout << "PP: checking for RISCV (" << EM_RISCV << ")" << std::endl;
    if (machine == EM_RISCV) {
      std::cout << "PP: identified ELF as RISCV" << std::endl;

      LLVMInitializeRISCVTargetInfo();
      LLVMInitializeRISCVTargetMC();
      LLVMInitializeRISCVDisassembler();

      bool rv32 = elf->get_class() == ELFCLASS32;
      objDis = llvm::make_unique<ObjectDisassembler>(
          llvm::make_unique<Architecture::Riscv::Info>(rv32));

      state = llvm::make_unique<DisassemblerState>(objDis->getInfo());
      stateCalc = llvm::make_unique<ApeStateCalculator>(
          *state, llvm::make_unique<PrinceApeStateUpdateFunction>(
                      *state, k0, k1, rounds));
    }
#endif // RISCV_TARGET_ENABLED

    if (!objDis || !state || !stateCalc) {
        std::cout << "PP: Architecture of the elf file is not supported" << std::endl;
        return;
    }

    if (state->loadElf(inputFile))
        return;

    applyAnnotations();
    std::cout << "==============================" << std::endl;
    disassemble();

    ready = true;
}

void PPCutterCore::applyAnnotations()
{
    std::cout << "applying annotations: " << file->asJson() << std::endl;
    for (auto addr : file->annotations) {
        std::cout << "adding annotation @ " << std::hex << addr.first << std::endl;
        for (auto& type : addr.second) {
            PPAnnotation& annotation = type.second;
            switch (type.first) {
                case ENTRYPOINT:
                {
                    std::cout << "type: ENTRYPOINT, name=" << annotation.data["name"] << std::endl;
                    state->defineFunction(annotation.offset, annotation.data["name"]);
                    break;
                }

                case INST_TYPE:
                {
                    std::cout << "type: INST_TYPE, itype=" << annotation.data["itype"] << std::endl;
                    DecodedInstruction& di = const_cast<DecodedInstruction&>(objDis->disassembleAddr(*state, annotation.offset));
                    di.type = parseInstructionType(annotation.data["itype"]);
                    state->defineFunction(annotation.offset, annotation.data["name"]);
                    break;
                }

                default:
                    std::cout << "type: unknown" << std::endl;
                    break;
            }
        }
    }
}

void PPCutterCore::disassemble()
{
    int num_rounds = 0;
    try {
        while (objDis->disassemble(*state)) num_rounds++;
        std::cout << "rounds: " << num_rounds << std::endl;


        for (auto addr : file->annotations) {
            for (auto &type : addr.second) {

            }
        }

        while (objDis->disassemble(*state)) num_rounds++;
        std::cout << "rounds: " << num_rounds << std::endl;
        std::cout << "functions: " << state->functions.size() << std::endl;
    } catch (const Exception &e) {
        std::cout << "PP: Aborted disassembling due to exception: " << e.what() << std::endl;
    }

    try {
        stateCalc->prepare();
        state->cleanupState();
    } catch (const Exception &e) {
        std::cout << "PP: could not prepare state due to: " << e.what() << std::endl;
    }
}

void PPCutterCore::registerAnnotationChange()
{
    emit annotationsChanged();
}

void PPCutterCore::fullRedo()
{
    applyAnnotations();
    std::cout << "==============================" << std::endl;
    disassemble();
}

QString PPCutterCore::jsonToQString(json json)
{
    QString data = "";
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (data != "")
        data += ", ";
        std::string key = it.key();
        std::string value = it.value();
        data += QString::fromUtf8(key.c_str());
        data += "=";
        data += QString::fromUtf8(value.c_str());
    }
    return data;
}

void PPCutterCore::updateAnnotation(AddressType addr, json data)
{
    for (json::iterator it = data.begin(); it != data.end(); ++it) {
        std::string name = it.key();
        PPAnnotationType type = annotationTypeFromString(name);
        // TODO delete previous value
        file->annotations[addr][type].data = it.value();
    }
    registerAnnotationChange();
}
