#ifndef PPBINARYFILE_H
#define PPBINARYFILE_H

#include <QVariant>
#include <pp/types.h>
#include <pp/config.h>
#include <pp/StateCalculators/AEE/ApeStateCalculator.h>
#include <pp/StateCalculators/PureSw/PureSwUpdateStateCalculator.h>
#include <pp/objectdisassembler.h>

#include <pp/ElfPatcher.h>
#include <pp/StateUpdateFunctions/crc/CrcStateUpdateFunction.hpp>
#include <pp/StateUpdateFunctions/prince_ape/PrinceApeStateUpdateFunction.hpp>
#include <pp/StateUpdateFunctions/sum/SumStateUpdateFunction.hpp>
#include <pp/architecture/riscv/info.h>
#include <pp/architecture/riscv/replace_instructions.h>
#include <pp/architecture/thumbv7m/info.h>

Q_DECLARE_METATYPE(UpdateType)

class PPBinaryFile
{
  private:
    struct EntryPointRange {
      AddressType start;
      AddressType end;
      std::string functionName;
    };
  public:
    std::string path;
    std::string md5sum;
    ELFIO::Elf_Half machine;

    std::unique_ptr<DisassemblerState> state;
    std::unique_ptr<ObjectDisassembler> objDis;
    std::unique_ptr<StateCalculator> stateCalc;

    std::vector<std::shared_ptr<Annotation>> annotations;

    std::vector<EntryPointRange> entrypoint_ranges;

    PPBinaryFile(std::string inputFile);
    ~PPBinaryFile();
    void createIndex();
    void disassemble();
    bool calculateStates();
    void buildFunctionCache();

    ::Function* getFunctionAt(AddressType addr) const;
    ::Function::EntryPoint& getEntrypointAt(AddressType addr) const;
    AddressType getStartAddressOfFunction(const ::Function& function) const;
    AddressType getEndAddressOfFunction(const ::Function& function) const;

    std::set<std::shared_ptr<Annotation>> getAnnotationsAt(AddressType addr);

    std::shared_ptr<Annotation> createAnnotation(Annotation::Type type, AddressType anchorAddress);
    void deleteAnnotation(std::shared_ptr<Annotation> annotation);

    std::set<AddressType> getAssociatedAddresses(AddressType addr);

    std::string getStates(AddressType addr);

    inline std::string getPath() {
      return path;
    }

    inline const DisassemblerState &getState() const {
      return *state;
    }

    inline const std::vector<std::shared_ptr<Annotation>>& getAnnotations() {
      return annotations;
    }

    inline void setAnnotations(std::vector<std::shared_ptr<Annotation>> _annotations) {
      annotations = _annotations;
    }
};


#endif
