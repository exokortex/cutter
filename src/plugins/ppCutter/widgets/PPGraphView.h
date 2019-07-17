#ifndef PPGRAPHVIEW_H
#define PPGRAPHVIEW_H

// Based on the DisassemblerGraphView from x64dbg

#include <QWidget>
#include <QPainter>
#include <QShortcut>
#include <QLabel>

#include "widgets/GraphView.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/RichTextPainter.h"
#include "common/CutterSeekable.h"

#include <vector>
#include <set>
#include <pp/types.h>
#include <pp/objectdisassembler.h>
#include "MainWindow.h"

class QTextEdit;
class SyntaxHighlighter;

class PPGraphView : public GraphView
{
    Q_OBJECT

    struct Text {
        std::vector<RichTextPainter::List> lines;

        Text() {}

        Text(const QString &text, QColor color, QColor background)
        {
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t rt;
            rt.highlight = false;
            rt.text = text;
            rt.textColor = color;
            rt.textBackground = background;
            rt.flags = rt.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
            richText.push_back(rt);
            lines.push_back(richText);
        }

        Text(const RichTextPainter::List &richText)
        {
            lines.push_back(richText);
        }

        QString ToQString() const
        {
            QString result;
            for (const auto &line : lines) {
                for (const auto &t : line) {
                    result += t.text;
                }
            }
            return result;
        }
    };

    struct Instr {
        ut64 addr = 0;
        ut64 size = 0;
        Text text;
        Text fullText;
        QString plainText;
        std::vector<unsigned char> opcode; //instruction bytes

        bool empty() const { return size == 0; }
        bool contains(ut64 addr) const;
    };

    struct Token {
        int start;
        int length;
        QString type;
        Instr *instr;
        QString name;
        QString content;
    };

    //struct HighlightToken {
    //    QString type; //highlight_token[0]
    //    ut64 start; //highlight_token[1]
    //    QString name; //highlight_token[2]
//
    //    bool equalsToken(const Token &token)
    //    {
    //        return this->type == token.type &&
    //               this->start == token.start &&
    //               this->name == token.name;
    //    }
//
    //    static HighlightToken *fromToken(const Token &token)
    //    {
    //        //TODO: memory leaks
    //        auto result = new HighlightToken();
    //        result->type = token.type;
    //        result->start = token.start;
    //        result->name = token.name;
    //        return result;
    //    }
    //};

    struct DisassemblyBlock {
        Text header_text;
        std::vector<Instr> instrs;
        ut64 entry = 0;
        ut64 true_path = 0;
        ut64 false_path = 0;
        bool terminal = false;
        bool indirectcall = false;
    };

    struct Function {
        bool ready;
        ut64 entry;
        ut64 update_id;
        std::vector<DisassemblyBlock> blocks;
    };

    struct Analysis {
        ut64 entry = 0;
        std::unordered_map<ut64, Function> functions;
        bool ready = false;
        ut64 update_id = 0;
        QString status = "Analyzing...";

        bool find_instr(ut64 addr, ut64 &func, ut64 &instr)
        {
            //TODO implement
            Q_UNUSED(addr);
            Q_UNUSED(func);
            Q_UNUSED(instr);
            return false;
        }

        //dummy class
    };

public:
    PPGraphView(QWidget *parent);
    ~PPGraphView() override;
    std::unordered_map<ut64, DisassemblyBlock> disassembly_blocks;
    virtual void drawBlock(QPainter &p, GraphView::GraphBlock &block) override;
    virtual void blockClicked(GraphView::GraphBlock &block, QMouseEvent *event, QPoint pos) override;
    virtual void blockDoubleClicked(GraphView::GraphBlock &block, QMouseEvent *event,
                                    QPoint pos) override;
    virtual bool helpEvent(QHelpEvent *event) override;
    virtual void blockHelpEvent(GraphView::GraphBlock &block, QHelpEvent *event, QPoint pos) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock &from,
                                                           GraphView::GraphBlock *to) override;
    virtual void blockTransitionedTo(GraphView::GraphBlock *to) override;

    void loadCurrentGraph();
    QString windowTitle;
    QTextEdit *header = nullptr;

    int getWidth() { return width; }
    int getHeight() { return height; }
    std::unordered_map<ut64, GraphBlock> getBlocks() { return blocks; }
    using EdgeConfigurationMapping = std::map<std::pair<ut64, ut64>, EdgeConfiguration>;
    EdgeConfigurationMapping getEdgeConfigurations();

public slots:
    void refreshView();
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void onSeekChanged(RVA addr);
    void toggleSync();

    void zoom(QPointF mouseRelativePos, double velocity);
    void zoomReset();

    void takeTrue();
    void takeFalse();

    void nextInstr();
    void prevInstr();

    void copySelection();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

private slots:
    void on_actionExportGraph_triggered();

private:
    bool transition_dont_seek = false;

    Token *highlight_token;
    // Font data
    std::unique_ptr<CachedFontMetrics<qreal>> mFontMetrics;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int baseline;
    bool emptyGraph;

    DisassemblyContextMenu *blockMenu;
    QMenu *contextMenu;

    MainWindow *main;
    static std::vector<QString> instructionColors;
    std::set<AddressType> associatedAddresses;

    void connectSeekChanged(bool disconnect);

    void initFont();
    void prepareGraphNode(GraphBlock &block);
    void cleanupEdges();
    void prepareHeader();
    Token *getToken(Instr *instr, int x);
    QPoint getTextOffset(int line) const;
    QPoint getInstructionOffset(const DisassemblyBlock &block, int line) const;
    RVA getAddrForMouseEvent(GraphBlock &block, QPoint *point);
    Instr *getInstrForMouseEvent(GraphBlock &block, QPoint *point);
    /**
     * @brief Get instructions placement and size relative to block.
     * Inefficient don't use this function when iterating over all instructions.
     * @param block
     * @param addr
     * @return
     */
    QRectF getInstrRect(GraphView::GraphBlock &block, RVA addr) const;
    void showInstruction(GraphView::GraphBlock &block, RVA addr);
    DisassemblyBlock *blockForAddress(RVA addr);
    void seekLocal(RVA addr, bool update_viewport = true);
    void seekInstruction(bool previous_instr);
    CutterSeekable *seekable = nullptr;
    QList<QShortcut *> shortcuts;
    QList<RVA> breakpoints;

    QColor disassemblyBackgroundColor;
    QColor disassemblySelectedBackgroundColor;
    QColor disassemblySelectionColor;
    QColor PCSelectionColor;
    QColor jmpColor;
    QColor brtrueColor;
    QColor brfalseColor;
    QColor retShadowColor;
    QColor indirectcallShadowColor;
    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor graphNodeColor;
    QColor mAddressColor;
    QColor mAddressBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointColor;
    QColor mDisabledBreakpointColor;

    QAction actionExportGraph;
    QAction actionUnhighlight;
    QAction actionSyncOffset;

    QLabel *emptyText = nullptr;
    SyntaxHighlighter *highlighter = nullptr;

signals:
    void viewRefreshed();
    void viewZoomed();
    void graphMoved();
    void resized();

public:
    bool isGraphEmpty()     { return emptyGraph; }
};

#endif // PPGRAPHVIEW_H
