#include "PPGraphView.h"
#include "common/CutterSeekable.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/CachedFontMetrics.h"
#include "common/TempConfig.h"
#include "common/SyntaxHighlighter.h"
#include "common/BasicBlockHighlighter.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/Helpers.h"

#include <QColorDialog>
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QToolTip>
#include <QTextDocument>
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QClipboard>
#include <QApplication>
#include <QAction>

#include <cmath>

#include "Cutter.h"
#include "common/Colors.h"
#include "common/Configuration.h"
#include "common/CachedFontMetrics.h"
#include "common/TempConfig.h"

#include <llvm-c/Target.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "plugins/ppCutter/core/PPCutterCore.h"
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

std::vector<QString> PPGraphView::instructionColors = {
    "#f00", // UNKNOWN = 0,
    "#000000", // SEQUENTIAL,
    "#0c0", // DIRECT_CALL,     // call + return -> sequential
    "#33c", // INDIRECT_CALL,   // call + return -> sequential
    "#00c", // RETURN,          // end of BB and function
    "#4c0", // TRAP,            // end of BB, unknown successor
    "#03f", // DIRECT_BRANCH,   // end of BB, one successor BB
    "#43f", // INDIRECT_BRANCH, // end of BB, one or more successor BBs
    "#83f", // COND_BRANCH,     // end of BB, one or more successor BBs
};

PPGraphView::PPGraphView(QWidget *parent, CutterSeekable *seekable, MainWindow *mainWindow,
                          QList<QAction *> additionalMenuActions)
    : CutterGraphView(parent),
      blockMenu(new DisassemblyContextMenu(this, mainWindow)),
      contextMenu(new QMenu(this)),
      seekable(seekable),
      actionUnhighlight(this),
      actionUnhighlightInstruction(this)
{
    highlight_token = nullptr;
    auto *layout = new QVBoxLayout(this);
    // Signals that require a refresh all
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(functionRenamed(const QString &, const QString &)), this,
            SLOT(refreshView()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(instructionChanged(RVA)), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshView()));
    connect(Core(), SIGNAL(refreshCodeViews()), this, SLOT(refreshView()));

    connectSeekChanged(false);

    // ESC for previous
    QShortcut *shortcut_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    shortcut_escape->setContext(Qt::WidgetShortcut);
    connect(shortcut_escape, &QShortcut::activated, seekable, &CutterSeekable::seekPrev);

    // Branch shortcuts
    QShortcut *shortcut_take_true = new QShortcut(QKeySequence(Qt::Key_T), this);
    shortcut_take_true->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_true, &QShortcut::activated, this, &PPGraphView::takeTrue);
    QShortcut *shortcut_take_false = new QShortcut(QKeySequence(Qt::Key_F), this);
    shortcut_take_false->setContext(Qt::WidgetShortcut);
    connect(shortcut_take_false, &QShortcut::activated, this, &PPGraphView::takeFalse);

    // Navigation shortcuts
    QShortcut *shortcut_next_instr = new QShortcut(QKeySequence(Qt::Key_J), this);
    shortcut_next_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_next_instr, &QShortcut::activated, this, &PPGraphView::nextInstr);
    QShortcut *shortcut_prev_instr = new QShortcut(QKeySequence(Qt::Key_K), this);
    shortcut_prev_instr->setContext(Qt::WidgetShortcut);
    connect(shortcut_prev_instr, &QShortcut::activated, this, &PPGraphView::prevInstr);
    shortcuts.append(shortcut_escape);
    shortcuts.append(shortcut_next_instr);
    shortcuts.append(shortcut_prev_instr);

    // Context menu that applies to everything
    contextMenu->addAction(&actionExportGraph);
    contextMenu->addMenu(layoutMenu);
    contextMenu->addSeparator();
    contextMenu->addActions(additionalMenuActions);


    QAction *highlightBB = new QAction(this);
    actionUnhighlight.setVisible(false);

    highlightBB->setText(tr("Highlight block"));
    connect(highlightBB, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        RVA currBlockEntry = blockForAddress(this->seekable->getOffset())->entry;

        QColor background = disassemblyBackgroundColor;
        if (auto block = bbh->getBasicBlock(currBlockEntry)) {
            background = block->color;
        }

        QColor c = QColorDialog::getColor(background, this, QString(),
                                          QColorDialog::DontUseNativeDialog);
        if (c.isValid()) {
            bbh->highlight(currBlockEntry, c);
        }
        Config()->colorsUpdated();
    });

    actionUnhighlight.setText(tr("Unhighlight block"));
    connect(&actionUnhighlight, &QAction::triggered, this, [this]() {
        auto bbh = Core()->getBBHighlighter();
        bbh->clear(blockForAddress(this->seekable->getOffset())->entry);
        Config()->colorsUpdated();
    });

    QAction *highlightBI = new QAction(this);
    actionUnhighlightInstruction.setVisible(false);

    highlightBI->setText(tr("Highlight instruction"));
    connect(highlightBI, &QAction::triggered, this,
            &PPGraphView::onActionHighlightBITriggered);

    actionUnhighlightInstruction.setText(tr("Unhighlight instruction"));
    connect(&actionUnhighlightInstruction, &QAction::triggered, this,
            &PPGraphView::onActionUnhighlightBITriggered);

    blockMenu->addAction(highlightBB);
    blockMenu->addAction(&actionUnhighlight);
    blockMenu->addAction(highlightBI);
    blockMenu->addAction(&actionUnhighlightInstruction);


    // Include all actions from generic context menu in block specific menu
    blockMenu->addSeparator();
    blockMenu->addActions(contextMenu->actions());

    connect(blockMenu, &DisassemblyContextMenu::copy, this, &PPGraphView::copySelection);

    // Add header as widget to layout so it stretches to the layout width
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignTop);

    this->scale_thickness_multiplier = true;
    connect(PPCore(), SIGNAL(annotationsChanged()), this, SLOT(refreshSeek()));
    connect(PPCore(), SIGNAL(stateChanged()), this, SLOT(refreshView()));
}

void PPGraphView::connectSeekChanged(bool disconn)
{
    if (disconn) {
        disconnect(seekable, &CutterSeekable::seekableSeekChanged, this, &PPGraphView::onSeekChanged);
    } else {
        connect(seekable, &CutterSeekable::seekableSeekChanged, this, &PPGraphView::onSeekChanged);
    }
}

PPGraphView::~PPGraphView()
{
    for (QShortcut *shortcut : shortcuts) {
        delete shortcut;
    }
}

void PPGraphView::refreshView()
{
    CutterGraphView::refreshView();
    loadCurrentGraph();
    emit viewRefreshed();
}

void PPGraphView::loadCurrentGraph()
{
    TempConfig tempConfig;
    tempConfig.set("scr.color", COLOR_MODE_16M)
    .set("asm.bb.line", false)
    .set("asm.lines", false)
    .set("asm.lines.fcn", false);

    disassembly_blocks.clear();
    blocks.clear();

    if (!PPCore()->isReady())
        return;

    const PPBinaryFile& file = PPCore()->getFile();
    ::Function *ppFunction = file.getFunctionAt(seekable->getOffset());

    if (!ppFunction)
        return;

    Analysis anal;
    anal.ready = true;

    Function f;
    f.ready = true;
    f.entry = file.getEntrypointAt(seekable->getOffset()).address;

    windowTitle = tr("PP-Graph");
    if (ppFunction == NULL) {
        windowTitle += " (Empty)";
    } else {
        QString qname = QString::fromStdString(ppFunction->getJoinedName());
        windowTitle += " (" + qname + ")";
    }
    emit nameChanged(windowTitle);

    if (!ppFunction) {
        return;
    }

    setEntry(f.entry);

    for (auto& bb : PPCore()->getBasicBlocksOfFunction(*ppFunction, f.entry, false))
    {
        // get address of first instruction (= address of block)
        RVA block_entry = bb->inst_begin()->address;

        DisassemblyBlock db;
        GraphBlock gb;
        gb.entry = block_entry;
        db.entry = block_entry;
        db.true_path = RVA_INVALID;
        db.false_path = RVA_INVALID;

        for (auto sit = bb->succ_begin(); sit != bb->succ_end(); ++sit) {
            const BasicBlock *succ = *sit;
            // get address of first instruction (= address of block)
            RVA addr = (succ->inst_begin())->address;
            gb.edges.emplace_back(addr);
        }

        // mark block if it is the entry of the function
        for (auto& entrypoint : ppFunction->getEntryPoints())
        {
            if (entrypoint.address != block_entry)
                continue;

            Instr i;
            i.addr = block_entry;
            i.size = 1;

            std::string titles = "<font color='#44f'>Entry Point: " + entrypoint.name + "</font>";
            QString text = QString::fromUtf8(titles.c_str());

            QTextDocument textDoc;
            textDoc.setHtml(text);
            RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);
            bool cropped;
            int blockLength = Config()->getGraphBlockMaxChars() + Core()->getConfigb("asm.bytes") * 24 + Core()->getConfigb("asm.emu") * 10;
            i.text = Text(RichTextPainter::cropped(richText, blockLength, "...", &cropped));
            i.fullText = Text();
            db.instrs.push_back(i);
        }

        for (auto dii = bb->inst_begin(); dii != bb->inst_end(); ++dii) {
            const DecodedInstruction& di = *dii;

            Instr i;
            i.addr = di.address;

            const PPBinaryFile& f = PPCore()->getFile();

            int size = f.getState().archInfo.getInstructionSize(di.instruction);

            // Skip last byte, otherwise it will overlap with next instruction
            i.size = size - 1;

            QString textColor = palette().text().color().name();
            QString color = (di.type == 1) ? textColor : instructionColors[di.type];

            bool annotated = false;
            if (f.getState().annotations_by_address.count(di.address))
                annotated = f.getState().annotations_by_address.at(di.address).size() != 0;
            std::string asmString = PPCore()->getObjDis().getInfo().printInstrunction(di.instruction);
            QString asmQString = QString::fromUtf8(asmString.c_str());

            BinaryDataViewType instBytes = f.getState().getData(di.address, size);

            QString qbytes = "";

            for (int b = 0; b < size; b++)
                qbytes += QString("%1").arg((quint8)instBytes[b], 2, 16, QChar('0'));

            qbytes = qbytes.leftJustified(2*6, ' '); // 6 bytes maximum
            qbytes = qbytes.replace(" ", "&nbsp;");

            QString comment = "";
            if (di.type != InstructionType::SEQUENTIAL)
                comment = QString(" (%1)").arg(QString::fromStdString(toString(di.type)).trimmed());


            QString states = QString::fromStdString(PPCore()->getFile().getStates(di.address));

            QString qCPE = "";
            if (CERTAIN == PPCore()->getObjDis().getInfo().isConstantLoad(di.instruction)) {
                DisassemblerState& ds = const_cast<DisassemblerState&>(PPCore()->getState());
                const ConstantPoolEntry & cpe = PPCore()->getObjDis().getInfo().extractConstant(ds, di);
                qCPE = "<font color='#ff7878'> ; = " + PPCore()->addrToString(cpe.value) + "</font>";
            }

            QString disas = QString("<font color='%10'>%1%2&nbsp;%8</font>&nbsp;<font color='%4'>%6&nbsp;&nbsp;%3%5%9")
                    .arg(di.address, 8, 16, QChar('0'))
                    .arg(annotated ? "*" : "&nbsp;")
                    .arg(asmQString)
                    .arg(color)
                    .arg(comment)
                    .arg(states.replace(" ", "&nbsp;"))
                    .arg(qbytes)
                    .arg(qCPE)
                    .arg(textColor);

            QTextDocument textDoc;
            textDoc.setHtml(disas);

            RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);

            bool cropped;
            int blockLength = Config()->getGraphBlockMaxChars() + Core()->getConfigb("asm.bytes") * 24 + Core()->getConfigb("asm.emu") * 10;
            i.text = Text(RichTextPainter::cropped(richText, blockLength, "...", &cropped));
            if(cropped)
                i.fullText = richText;
            else
                i.fullText = Text();
            db.instrs.push_back(i);
        }


        disassembly_blocks[db.entry] = db;
        prepareGraphNode(gb);
        f.blocks.push_back(db);

        addBlock(gb);
    }

    /*
    for (const QJsonValueRef &value : func["blocks"].toArray()) {
        QJsonObject block = value.toObject();
        RVA block_entry = block["offset"].toVariant().toULongLong();
        RVA block_size = block["size"].toVariant().toULongLong();
        RVA block_fail = block["fail"].toVariant().toULongLong();
        RVA block_jump = block["jump"].toVariant().toULongLong();

        DisassemblyBlock db;
        GraphBlock gb;
        gb.entry = block_entry;
        db.entry = block_entry;
        db.true_path = RVA_INVALID;
        db.false_path = RVA_INVALID;
        if (block_fail) {
            db.false_path = block_fail;
            gb.edges.emplace_back(block_fail);
        }
        if (block_jump) {
            if (block_fail) {
                db.true_path = block_jump;
            }
            gb.edges.emplace_back(block_jump);
        }

        QJsonObject switchOp = block["switchop"].toObject();
        if (!switchOp.isEmpty()) {
            QJsonArray caseArray = switchOp["cases"].toArray();
            for (QJsonValue caseOpValue : caseArray) {
                QJsonObject caseOp = caseOpValue.toObject();
                bool ok;
                RVA caseJump = caseOp["jump"].toVariant().toULongLong(&ok);
                if (!ok) {
                    continue;
                }
                gb.edges.emplace_back(caseJump);
            }
        }

        QJsonArray opArray = block["ops"].toArray();
        for (int opIndex = 0; opIndex < opArray.size(); opIndex++) {
            QJsonObject op = opArray[opIndex].toObject();
            Instr i;
            i.addr = op["offset"].toVariant().toULongLong();

            if (opIndex < opArray.size() - 1) {
                // get instruction size from distance to next instruction ...
                RVA nextOffset = opArray[opIndex + 1].toObject()["offset"].toVariant().toULongLong();
                i.size = nextOffset - i.addr;
            } else {
                // or to the end of the block.
                i.size = (block_entry + block_size) - i.addr;
            }

            QTextDocument textDoc;
            textDoc.setHtml(CutterCore::ansiEscapeToHtml(op["text"].toString()));

            i.plainText = textDoc.toPlainText();

            RichTextPainter::List richText = RichTextPainter::fromTextDocument(textDoc);
            //Colors::colorizeAssembly(richText, textDoc.toPlainText(), 0);

            bool cropped;
            int blockLength = Config()->getGraphBlockMaxChars() + Core()->getConfigb("asm.bytes") * 24 +
                              Core()->getConfigb("asm.emu") * 10;
            i.text = Text(RichTextPainter::cropped(richText, blockLength, "...", &cropped));
            if (cropped)
                i.fullText = richText;
            else
                i.fullText = Text();
            db.instrs.push_back(i);
        }
        disassembly_blocks[db.entry] = db;
        prepareGraphNode(gb);

        addBlock(gb);
    }*/
    cleanupEdges(blocks);

    if (!disassembly_blocks.empty()) {
        computeGraphPlacement();
    }
}

PPGraphView::EdgeConfigurationMapping PPGraphView::getEdgeConfigurations()
{
    EdgeConfigurationMapping result;
    for (auto &block : blocks) {
        for (const auto &edge : block.second.edges) {
            result[ {block.first, edge.target}] = edgeConfiguration(block.second, &blocks[edge.target], false);
        }
    }
    return result;
}

void PPGraphView::prepareGraphNode(GraphBlock &block)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];
    int width = 0;
    int height = 0;
    for (auto &line : db.header_text.lines) {
        int lw = 0;
        for (auto &part : line)
            lw += mFontMetrics->width(part.text);
        if (lw > width)
            width = lw;
        height += 1;
    }
    for (Instr &instr : db.instrs) {
        for (auto &line : instr.text.lines) {
            int lw = 0;
            for (auto &part : line)
                lw += mFontMetrics->width(part.text);
            if (lw > width)
                width = lw;
            height += 1;
        }
    }
    int extra = static_cast<int>(4 * charWidth + 4);
    block.width = static_cast<int>(width + extra + charWidth);
    block.height = (height * charHeight) + extra;
}

void PPGraphView::drawBlock(QPainter &p, GraphView::GraphBlock &block, bool interactive)
{
    QRectF blockRect(block.x, block.y, block.width, block.height);

    const qreal padding = 2 * charWidth;

    p.setPen(Qt::black);
    p.setBrush(Qt::gray);
    p.setFont(Config()->getFont());
    p.drawRect(blockRect);

    breakpoints = Core()->getBreakpointsAddresses();

    // Render node
    DisassemblyBlock &db = disassembly_blocks[block.entry];
    bool block_selected = false;
    bool PCInBlock = false;
    RVA selected_instruction = RVA_INVALID;

    // Figure out if the current block is selected
    RVA addr = seekable->getOffset();
    RVA PCAddr = Core()->getProgramCounterValue();
    for (const Instr &instr : db.instrs) {
        if (instr.contains(addr)) {
            block_selected = true;
            selected_instruction = instr.addr;
        }
        if (instr.contains(PCAddr)) {
            PCInBlock = true;
        }

        // TODO: L219
    }

    p.setPen(QColor(0, 0, 0, 0));
    if (db.terminal) {
        p.setBrush(retShadowColor);
    } else if (db.indirectcall) {
        p.setBrush(indirectcallShadowColor);
    } else {
        p.setBrush(QColor(0, 0, 0, 100));
    }

    p.setPen(QPen(graphNodeColor, 1));

    if (block_selected) {
        p.setBrush(disassemblySelectedBackgroundColor);
    } else {
        p.setBrush(disassemblyBackgroundColor);
    }

    // Draw basic block background
    p.drawRect(blockRect);
    auto bb = Core()->getBBHighlighter()->getBasicBlock(block.entry);
    if (bb) {
        QColor color(bb->color);
        p.setBrush(color);
        p.drawRect(blockRect);
    }

    const int firstInstructionY = block.y + getInstructionOffset(db, 0).y();

    // Stop rendering text when it's too small
    auto transform = p.combinedTransform();
    QRect screenChar = transform.mapRect(QRect(0, 0, charWidth, charHeight));

    if (screenChar.width() * qhelpers::devicePixelRatio(p.device()) < 4) {
        return;
    }

    // Draw different background for selected instruction
    if (selected_instruction != RVA_INVALID) {
        int y = firstInstructionY;
        for (const Instr &instr : db.instrs) {
            //if (instr.addr > selected_instruction) {
            //    break;
            //}
            auto selected = instr.addr == selected_instruction;
            if (selected) {
                p.fillRect(QRect(static_cast<int>(block.x + charWidth), y,
                                 static_cast<int>(block.width - (10 + padding)),
                                 int(instr.text.lines.size()) * charHeight), disassemblySelectionColor);
            } else if (associatedAddresses.count(instr.addr)) {
                p.fillRect(QRect(static_cast<int>(block.x + charWidth), y,
                                 static_cast<int>(block.width - (10 + padding)),
                                 int(instr.text.lines.size()) * charHeight), QColor(0x8f, 0xf0, 0x82));
            }
            y += int(instr.text.lines.size()) * charHeight;
        }
    }

    // Highlight selected tokens
    if (interactive && highlight_token != nullptr) {
        int y = firstInstructionY;
        qreal tokenWidth = mFontMetrics->width(highlight_token->content);

        for (const Instr &instr : db.instrs) {
            int pos = -1;

            while ((pos = instr.plainText.indexOf(highlight_token->content, pos + 1)) != -1) {
                int tokenEnd = pos + highlight_token->content.length();

                if ((pos > 0 && instr.plainText[pos - 1].isLetterOrNumber())
                        || (tokenEnd < instr.plainText.length() && instr.plainText[tokenEnd].isLetterOrNumber())) {
                    continue;
                }

                qreal widthBefore = mFontMetrics->width(instr.plainText.left(pos));
                if (charWidth * 3 + widthBefore > block.width - (10 + padding)) {
                    continue;
                }

                qreal highlightWidth = tokenWidth;
                if (charWidth * 3 + widthBefore + tokenWidth >= block.width - (10 + padding)) {
                    highlightWidth = block.width - widthBefore - (10 +  2 * padding);
                }

                QColor selectionColor = ConfigColor("wordHighlight");

                p.fillRect(QRectF(block.x + charWidth * 3 + widthBefore, y, highlightWidth,
                                  charHeight), selectionColor);
            }

            y += int(instr.text.lines.size()) * charHeight;
        }
    }

    // Highlight program counter
    if (PCInBlock) {
        int y = firstInstructionY;
        for (const Instr &instr : db.instrs) {
            if (instr.addr > PCAddr) {
                break;
            }
            auto PC = instr.addr == PCAddr;
            if (PC) {
                p.fillRect(QRect(static_cast<int>(block.x + charWidth), y,
                                 static_cast<int>(block.width - (10 + padding)),
                                 int(instr.text.lines.size()) * charHeight), PCSelectionColor);
            }
            y += int(instr.text.lines.size()) * charHeight;
        }
    }

    qreal render_height = viewport()->size().height();

    // Stop rendering text when it's too small
    if (charHeight * getViewScale() * p.device()->devicePixelRatioF() < 4) {
        return;
    }

    // Render node text
    auto x = block.x + padding;
    int y = block.y + getTextOffset(0).y();
    qreal lineHeightRender = charHeight;
    for (auto &line : db.header_text.lines) {

        RichTextPainter::paintRichText<qreal>(&p, x, y, block.width, charHeight, 0, line,
                                       mFontMetrics.get());
        y += charHeight;
    }

    for (const Instr &instr : db.instrs) {
        if (Core()->isBreakpoint(breakpoints, instr.addr)) {
            p.fillRect(QRect(static_cast<int>(block.x + charWidth), y,
                             static_cast<int>(block.width - (10 + padding)),
                             int(instr.text.lines.size()) * charHeight), ConfigColor("gui.breakpoint_background"));
            if (instr.addr == selected_instruction) {
                p.fillRect(QRect(static_cast<int>(block.x + charWidth), y,
                                 static_cast<int>(block.width - (10 + padding)),
                                 int(instr.text.lines.size()) * charHeight), disassemblySelectionColor);
            }
        }
        for (auto &line : instr.text.lines) {
            int rectSize = qRound(charWidth);
            if (rectSize % 2) {
                rectSize++;
            }
            // Assume charWidth <= charHeight
            // TODO: Breakpoint/Cip stuff
            QRectF bpRect(x - rectSize / 3.0, y + (charHeight - rectSize) / 2.0, rectSize, rectSize);
            Q_UNUSED(bpRect);

            RichTextPainter::paintRichText<qreal>(&p, x + charWidth, y,
                                           block.width - charWidth, charHeight, 0, line,
                                           mFontMetrics.get());
            y += charHeight;

        }
    }
}

GraphView::EdgeConfiguration PPGraphView::edgeConfiguration(GraphView::GraphBlock &from,
                                                                      GraphView::GraphBlock *to,
                                                                      bool interactive)
{
    EdgeConfiguration ec;
    DisassemblyBlock &db = disassembly_blocks[from.entry];
    if (to->entry == db.true_path) {
        ec.color = brtrueColor;
    } else if (to->entry == db.false_path) {
        ec.color = brfalseColor;
    } else {
        ec.color = jmpColor;
    }
    ec.start_arrow = false;
    ec.end_arrow = true;
    if (interactive) {
        if (from.entry == currentBlockAddress) {
            ec.width_scale = 2.0;
        } else if (to->entry == currentBlockAddress) {
            ec.width_scale = 2.0;
        }
    }
    return ec;
}


RVA PPGraphView::getAddrForMouseEvent(GraphBlock &block, QPoint *point)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = static_cast<int>(db.header_text.lines.size());
    if (mouse_row < cur_row) {
        return db.entry;
    }

    Instr *instr = getInstrForMouseEvent(block, point);
    if (instr) {
        return instr->addr;
    }

    return RVA_INVALID;
}

PPGraphView::Instr *PPGraphView::getInstrForMouseEvent(
    GraphView::GraphBlock &block, QPoint *point, bool force)
{
    DisassemblyBlock &db = disassembly_blocks[block.entry];

    // Remove header and margin
    int off_y = getInstructionOffset(db, 0).y();

    // Get mouse coordinate over the actual text
    int text_point_y = point->y() - off_y;
    int mouse_row = text_point_y / charHeight;

    int cur_row = static_cast<int>(db.header_text.lines.size());

    for (Instr &instr : db.instrs) {
        if (mouse_row < cur_row + (int)instr.text.lines.size()) {
            return &instr;
        }
        cur_row += instr.text.lines.size();
    }
    if (force && !db.instrs.empty()) {
        if (mouse_row <= 0) {
            return &db.instrs.front();
        } else {
            return &db.instrs.back();
        }
    }

    return nullptr;
}

QRectF PPGraphView::getInstrRect(GraphView::GraphBlock &block, RVA addr) const
{
    auto blockIt = disassembly_blocks.find(block.entry);
    if (blockIt == disassembly_blocks.end()) {
        return QRectF();
    }
    auto &db = blockIt->second;
    if (db.instrs.empty()) {
        return QRectF();
    }

    size_t sequenceAddr = db.instrs[0].addr;
    size_t firstLineWithAddr = 0;
    size_t currentLine = 0;
    for (size_t i = 0; i < db.instrs.size(); i++) {
        auto &instr = db.instrs[i];
        if (instr.addr != sequenceAddr) {
            sequenceAddr = instr.addr;
            firstLineWithAddr = currentLine;
        }
        if (instr.contains(addr)) {
            while (i < db.instrs.size() && db.instrs[i].addr == sequenceAddr) {
                currentLine += db.instrs[i].text.lines.size();
                i++;
            }
            QPointF topLeft = getInstructionOffset(db, static_cast<int>(firstLineWithAddr));
            return QRectF(topLeft, QSizeF(block.width - 4 * charWidth,
                                          charHeight * int(currentLine - firstLineWithAddr)));
        }
        currentLine += instr.text.lines.size();
    }
    return QRectF();
}

void PPGraphView::showInstruction(GraphView::GraphBlock &block, RVA addr)
{
    QRectF rect = getInstrRect(block, addr);
    rect.translate(block.x, block.y);
    showRectangle(QRect(rect.x(), rect.y(), rect.width(), rect.height()), true);
}

PPGraphView::DisassemblyBlock *PPGraphView::blockForAddress(RVA addr)
{
    for (auto &blockIt : disassembly_blocks) {
        DisassemblyBlock &db = blockIt.second;
        for (const Instr &i : db.instrs) {
            if (i.addr == RVA_INVALID || i.size == RVA_INVALID) {
                continue;
            }

            if (i.contains(addr)) {
                return &db;
            }
        }
    }
    return nullptr;
}

const PPGraphView::Instr *PPGraphView::instrForAddress(RVA addr)
{
    DisassemblyBlock *block = blockForAddress(addr);
    for (const Instr &i : block->instrs) {
        if (i.addr == RVA_INVALID || i.size == RVA_INVALID) {
            continue;
        }

        if (i.contains(addr)) {
            return &i;
        }
    }
    return nullptr;
}

void PPGraphView::onSeekChanged(RVA addr)
{
    blockMenu->setOffset(addr);
    DisassemblyBlock *db = blockForAddress(addr);
    bool switchFunction = false;
    if (!db) {
        // not in this function, try refreshing
        refreshView();
        db = blockForAddress(addr);
        switchFunction = true;
    }
    if (db) {
        // This is a local address! We animated to it.
        transition_dont_seek = true;
        showBlock(blocks[db->entry], !switchFunction);
        showInstruction(blocks[db->entry], addr);
    }
}


void PPGraphView::takeTrue()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->true_path != RVA_INVALID) {
        seekable->seek(db->true_path);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void PPGraphView::takeFalse()
{
    DisassemblyBlock *db = blockForAddress(seekable->getOffset());
    if (!db) {
        return;
    }

    if (db->false_path != RVA_INVALID) {
        seekable->seek(db->false_path);
    } else if (!blocks[db->entry].edges.empty()) {
        seekable->seek(blocks[db->entry].edges[0].target);
    }
}

void PPGraphView::seekInstruction(bool previous_instr)
{
    RVA addr = seekable->getOffset();
    DisassemblyBlock *db = blockForAddress(addr);
    if (!db) {
        return;
    }

    for (size_t i = 0; i < db->instrs.size(); i++) {
        Instr &instr = db->instrs[i];
        if (!instr.contains(addr)) {
            continue;
        }

        // Found the instruction. Check if a next one exists
        if (!previous_instr && (i < db->instrs.size() - 1)) {
            seekable->seek(db->instrs[i + 1].addr);
        } else if (previous_instr && (i > 0)) {
            while (i > 0 && db->instrs[i].addr == addr) { // jump over 0 size instructions
                i--;
            }
            seekable->seek(db->instrs[i].addr);
            break;
        }
    }
}

void PPGraphView::nextInstr()
{
    seekInstruction(false);
}

void PPGraphView::prevInstr()
{
    seekInstruction(true);
}

void PPGraphView::seekLocal(RVA addr, bool update_viewport)
{
    RVA curAddr = seekable->getOffset();
    if (addr == curAddr) {
        return;
    }
    connectSeekChanged(true);
    seekable->seek(addr);
    associatedAddresses = PPCore()->getFile().getAssociatedAddresses(addr);
    connectSeekChanged(false);
    if (update_viewport) {
        viewport()->update();
    }
}

void PPGraphView::copySelection()
{
    if (!highlight_token) return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(highlight_token->content);
}

PPGraphView::Token *PPGraphView::getToken(Instr *instr, int x)
{
    x -= (int) (3 * charWidth); // Ignore left margin
    if (x < 0) {
        return nullptr;
    }

    int clickedCharPos = mFontMetrics->position(instr->plainText, x);
    if (clickedCharPos > instr->plainText.length()) {
        return nullptr;
    }

    static const QRegularExpression tokenRegExp(R"(\b(?<!\.)([^\s]+)\b(?!\.))");
    QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(instr->plainText);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();

        if (match.capturedStart() <= clickedCharPos && match.capturedEnd() > clickedCharPos) {
            auto t = new Token;
            t->start = match.capturedStart();
            t->length = match.capturedLength();
            t->content = match.captured();
            t->instr = instr;

            return t;
        }
    }

    return nullptr;
}

QPoint PPGraphView::getInstructionOffset(const DisassemblyBlock &block, int line) const
{
    return getTextOffset(line + static_cast<int>(block.header_text.lines.size()));
}

void PPGraphView::blockClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                         QPoint pos)
{
    Instr *instr = getInstrForMouseEvent(block, &pos, event->button() == Qt::RightButton);
    if (!instr) {
        return;
    }

    currentBlockAddress = block.entry;

    highlight_token = getToken(instr, pos.x());

    RVA addr = instr->addr;
    seekLocal(addr);

    blockMenu->setOffset(addr);
    blockMenu->setCanCopy(highlight_token);
    if (highlight_token) {
        blockMenu->setCurHighlightedWord(highlight_token->content);
    }
    viewport()->update();
}

void PPGraphView::blockContextMenuRequested(GraphView::GraphBlock &block,
                                                      QContextMenuEvent *event, QPoint /*pos*/)
{
    const RVA offset = this->seekable->getOffset();
    actionUnhighlight.setVisible(Core()->getBBHighlighter()->getBasicBlock(block.entry));
    actionUnhighlightInstruction.setVisible(Core()->getBIHighlighter()->getBasicInstruction(offset));
    event->accept();
    blockMenu->exec(event->globalPos());
}

void PPGraphView::contextMenuEvent(QContextMenuEvent *event)
{
    GraphView::contextMenuEvent(event);
    if (!event->isAccepted()) {
        //TODO: handle opening block menu using keyboard
        contextMenu->exec(event->globalPos());
        event->accept();
    }
}

void PPGraphView::showExportDialog()
{
    QString defaultName = "graph";
    if (auto f = Core()->functionIn(currentFcnAddr)) {
        QString functionName = f->name;
        // don't confuse image type guessing and make c++ names somewhat usable
        functionName.replace(QRegularExpression("[.:]"), "_");
        functionName.remove(QRegularExpression("[^a-zA-Z0-9_].*"));
        if (!functionName.isEmpty()) {
            defaultName = functionName;
        }
    }
    showExportGraphDialog(defaultName, "agf", currentFcnAddr);
}

void PPGraphView::blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos)
{
    Q_UNUSED(event);
    seekable->seekToReference(getAddrForMouseEvent(block, &pos));
}

void PPGraphView::blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos)
{
    Instr *instr = getInstrForMouseEvent(block, &pos);
    if (!instr || instr->fullText.lines.empty()) {
        QToolTip::hideText();
        event->ignore();
        return;
    }

    QToolTip::showText(event->globalPos(), instr->fullText.ToQString());
}

bool PPGraphView::helpEvent(QHelpEvent *event)
{
    if (!GraphView::helpEvent(event)) {
        QToolTip::hideText();
        event->ignore();
    }

    return true;
}

void PPGraphView::blockTransitionedTo(GraphView::GraphBlock *to)
{
    currentBlockAddress = to->entry;
    if (transition_dont_seek) {
        transition_dont_seek = false;
        return;
    }
    seekLocal(to->entry);
}


void PPGraphView::onActionHighlightBITriggered()
{
    const RVA offset = this->seekable->getOffset();
    const Instr *instr = instrForAddress(offset);

    if (!instr) {
        return;
    }

    auto bih = Core()->getBIHighlighter();
    QColor background = ConfigColor("linehl");
    if (auto currentColor = bih->getBasicInstruction(offset)) {
        background = currentColor->color;
    }

    QColor c = QColorDialog::getColor(background, this, QString(),
                                      QColorDialog::DontUseNativeDialog);
    if (c.isValid()) {
        bih->highlight(instr->addr, instr->size, c);
    }
    Config()->colorsUpdated();
}

void PPGraphView::onActionUnhighlightBITriggered()
{
    const RVA offset = this->seekable->getOffset();
    const Instr *instr = instrForAddress(offset);

    if (!instr) {
        return;
    }

    auto bih = Core()->getBIHighlighter();
    bih->clear(instr->addr, instr->size);
    Config()->colorsUpdated();
}

void PPGraphView::restoreCurrentBlock()
{
    onSeekChanged(this->seekable->getOffset()); // try to keep the view on current block
}


void PPGraphView::paintEvent(QPaintEvent *event)
{
    // PPGraphView is always dirty
    setCacheDirty();
    GraphView::paintEvent(event);
}

bool PPGraphView::Instr::contains(ut64 addr) const
{
    return this->addr <= addr && (addr - this->addr) < size;
}
