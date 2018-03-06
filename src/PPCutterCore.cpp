#include <QJsonArray>
#include <QJsonObject>
#include "PPCutterCore.h"

#include "rapidjson/document.h"
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>

#include <llvm-c/Target.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
//#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>

#include "PPCutterCore.h"
#include <pp/ElfPatcher.h>
#include <pp/StateCalculators/AEE/ApeStateCalculator.h>
#include <pp/StateCalculators/PureSw/PureSwUpdateStateCalculator.h>
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
    if (aType == ENTRYPOINT)
        return "entrypoint";
    else if (aType == INST_TYPE)
        return "inst_type";
    else if (aType == LOAD_REF)
        return "load_ref";

    return "ERROR";
}

void PPCutterCore::loadFile(QString path)
{
    auto logger = get_logger("PP-Core");

    // create emtpy file
    file = std::unique_ptr<PPFile>(new PPFile());

    json annData1 = {{"name", "f1"}};
    json annData2 = {{"name", "f2"}};
    json annData3 = {{"name", "f3"}};
    json annData4 = {{"name", "f4"}};
    json annData5 = {{"name", "f5"}};
    file->annotations.emplace_back(0x204, ENTRYPOINT, "test comment 3", annData3);
    file->annotations.emplace_back(0x128, ENTRYPOINT, "test comment 1", annData1);
    file->annotations.emplace_back(0x158, ENTRYPOINT, "test comment 2", annData2);
    file->annotations.emplace_back(0x2c0, ENTRYPOINT, "test comment 4", annData4);
    file->annotations.emplace_back(0x2c8, ENTRYPOINT, "test comment 5", annData5);

    //PPAnnotation testAnnotation(0x204, ENTRYPOINT, "test comment", annData1);
    //PPAnnotation testAnnotation2(0x204, ENTRYPOINT, "test comment", annData2);
    //PPAnnotation testAnnotation3(0x204, ENTRYPOINT, "test comment", annData3);
    //PPAnnotation testAnnotation4(0x204, ENTRYPOINT, "test comment", annData4);
    //testAnnotation.offset = 0x204;
    //testAnnotation.type = ENTRYPOINT;
    //testAnnotation.comment = "test annotation";
    //testAnnotation.data = data;//std::unique_ptr<Value>(new Value());


    //state->defineFunction(0x128, "f2");
    //state->defineFunction(0x158, "f3");
    //state->defineFunction(0x2c0, "f4");
    //state->defineFunction(0x2c8, "f5");

    //file->annotations.push_back(std::move(testAnnotation));

    PPFunction testFun;
    file->functions.push_back(testFun);

    std::string inputFile = path.toStdString();
    std::cout << "inputFile: " << inputFile << std::endl;
    uint64_t k0 = 0x12345678;
    uint64_t k1 = 0x8765432100000000;
    int rounds = 12;

    auto elf = llvm::make_unique<ELFIO::elfio>();
    if (!elf->load(inputFile))
    {
        std::cout << "PP: File not found" << std::endl;
        logger->error("File \"{}\" is not found or it is not an ELF file",
                      inputFile);
        exit(-1);
    }
    std::cout << "PP: File loaded" << std::endl;

    const ELFIO::Elf_Half machine = elf->get_machine();
    std::unique_ptr<StateCalculator> stateCalc;

    if (machine == EM_ARM) {
        std::cout << "PP: identified ELF as ARM" << std::endl;

        LLVMInitializeARMTargetInfo();
        LLVMInitializeARMTargetMC();
        LLVMInitializeARMDisassembler();

        objDis = llvm::make_unique<ObjectDisassembler>(
            llvm::make_unique<Architecture::Thumb::Info>());
        state = llvm::make_unique<DisassemblerState>(objDis->getInfo());

        std::unique_ptr<StateUpdateFunction> updateFunc;
        if (false) // cli.m0_)
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
    } else if (machine == EM_RISCV) {
        std::cout << "PP: processing ELF as RISCV" << std::endl;

        LLVMInitializeRISCVTargetInfo();
        LLVMInitializeRISCVTargetMC();
        LLVMInitializeRISCVDisassembler();

        objDis = llvm::make_unique<ObjectDisassembler>(
            llvm::make_unique<Architecture::Riscv::Info>());
        state = llvm::make_unique<DisassemblerState>(objDis->getInfo());
        stateCalc = llvm::make_unique<ApeStateCalculator>(
            *state, llvm::make_unique<PrinceApeStateUpdateFunction>(
                        *state, k0, k1, rounds));
    }

    if (!objDis || !state || !stateCalc) {
        std::cout << "PP: Architecture of the elf file is not supported" << std::endl;
        return;
    }

    if (state->loadElf(inputFile))
        return;

    // apply annotations
    for (auto &annotation : file->annotations) {
        std::cout << "adding annotation @ " << annotation.offset << std::endl;
        switch (annotation.type) {
            case ENTRYPOINT:
                {
                    std::cout << "type: ENTRYPOINT, name=" << annotation.data["name"] << std::endl;
                    state->defineFunction(annotation.offset, annotation.data["name"]);
                    break;
                }

            default:
                std::cout << "type: unknown" << std::endl;
                // not implemented
                break;
        }
    }


    std::cout << "==============================" << std::endl;

    int num_rounds = 0;
    try {
        while (objDis->disassemble(*state)) num_rounds++;
        std::cout << "rounds: " << num_rounds << std::endl;
        while (objDis->disassemble(*state)) num_rounds++;
        std::cout << "rounds: " << num_rounds << std::endl;
        std::cout << "functions: " << state->functions.size() << std::endl;


        stateCalc->prepare();
        state->cleanupState();
    } catch (const Exception &e) {
        std::cout << "PP: Aborted disassembling due to exception: " << e.what() << std::endl;
    }
}

PPAnnotation* PPCutterCore::getAnnotationAt(AddressType addr)
{
    for (auto& annotation: file->annotations) {
        if (annotation.offset == addr)
            return &annotation;
    }
    return nullptr;
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
        functions.PushBack(function, alloc);
    }
    file.AddMember("functions", functions, alloc);

    Value annotations(kArrayType);
    for (int j = 0; j <= 3; j++) {
        Value annotation(kObjectType);
        annotation.AddMember("offset", 7512, alloc);
        annotation.AddMember("name", "main", alloc);
        annotation.AddMember("type", "entrypoint", alloc);
        annotations.PushBack(annotation, alloc);
    }
    file.AddMember("annotations", annotations, alloc);

    files.PushBack(file, alloc);
    d.AddMember("files", files, alloc);

    // write to file
    std::ofstream ofs("output.json");
    OStreamWrapper osw(ofs);
    PrettyWriter<OStreamWrapper> writer(osw);
    d.Accept(writer);
}
