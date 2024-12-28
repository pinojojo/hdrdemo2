#ifndef GLSLEDITOR_HPP
#define GLSLEDITOR_HPP

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

// 代码编辑器类
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
};

// GLSL语法高亮器
class GLSLHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit GLSLHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat builtinVarsFormat;
    QTextCharFormat builtinFuncsFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat preprocessorFormat;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
};

// 主编辑器窗口
class GLSLEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit GLSLEditor(QWidget *parent = nullptr);

private:
    void setupEditor();
    void createActions();

    CodeEditor *editor;
    GLSLHighlighter *highlighter;
};

#endif // GLSLEDITOR_HPP