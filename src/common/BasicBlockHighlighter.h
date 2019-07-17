#ifndef BASICKBLOCKHIGHLIGHTER_H
#define BASICKBLOCKHIGHLIGHTER_H

class BasicBlockHighlighter;

#include "Cutter.h"
#include <map>

struct CuBasicBlock {
    RVA address;
    QColor color;
};

typedef std::map<RVA, CuBasicBlock*>::iterator BasicBlockIt;

class BasicBlockHighlighter
{
public:
    BasicBlockHighlighter();
    ~BasicBlockHighlighter();

    void highlight(RVA address, const QColor &color);
    void clear(RVA address);
    CuBasicBlock *getBasicBlock(RVA address);

private:
    std::map<RVA, CuBasicBlock*> bbMap;
};

#endif   // BASICBLOCKHIGHLIGHTER_H
