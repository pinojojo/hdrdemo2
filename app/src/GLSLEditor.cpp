#include "GLSLEditor.hpp"
#include <QPainter>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QFont>

// LineNumberArea 类实现
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

// CodeEditor 实现
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // 设置字体
    QFont font("Consolas", 12);
    font.setStyleHint(QFont::Monospace);
    setFont(font);

    // 设置tab为4个空格
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);

    // 设置暗色主题
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    setPalette(p);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                    lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor("#2D2D2D");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1E1E1E"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor("#858585"));
            painter.drawText(0, top, lineNumberArea->width() - 3, fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// GLSLHighlighter 实现
GLSLHighlighter::GLSLHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // 关键字格式
    keywordFormat.setForeground(QColor("#569CD6"));
    const QString keywordPatterns[] = {
        QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bfloat\\b"), QStringLiteral("\\bint\\b"),
        QStringLiteral("\\bvec2\\b"), QStringLiteral("\\bvec3\\b"), QStringLiteral("\\bvec4\\b"),
        QStringLiteral("\\bmat2\\b"), QStringLiteral("\\bmat3\\b"), QStringLiteral("\\bmat4\\b"),
        QStringLiteral("\\buniform\\b"), QStringLiteral("\\bvarying\\b"), QStringLiteral("\\bconst\\b"),
        QStringLiteral("\\bin\\b"), QStringLiteral("\\bout\\b"), QStringLiteral("\\binout\\b"),
        QStringLiteral("\\bif\\b"), QStringLiteral("\\belse\\b"), QStringLiteral("\\bfor\\b"),
        QStringLiteral("\\bwhile\\b"), QStringLiteral("\\breturn\\b"), QStringLiteral("\\bbreak\\b"),
        QStringLiteral("\\bcontinue\\b"), QStringLiteral("\\bdiscard\\b"), QStringLiteral("\\bstruct\\b")
    };

    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // 内置变量格式
    builtinVarsFormat.setForeground(QColor("#4EC9B0"));
    const QString builtinVars[] = {
        QStringLiteral("\\bgl_Position\\b"), QStringLiteral("\\bgl_FragColor\\b"),
        QStringLiteral("\\bgl_FragCoord\\b"), QStringLiteral("\\bgl_PointSize\\b")
    };

    for (const QString &pattern : builtinVars) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = builtinVarsFormat;
        highlightingRules.append(rule);
    }

    // 内置函数格式
    builtinFuncsFormat.setForeground(QColor("#DCDCAA"));
    const QString builtinFuncs[] = {
        QStringLiteral("\\bsin\\b"), QStringLiteral("\\bcos\\b"), QStringLiteral("\\btan\\b"),
        QStringLiteral("\\bnormalize\\b"), QStringLiteral("\\bdot\\b"), QStringLiteral("\\bcross\\b"),
        QStringLiteral("\\bmix\\b"), QStringLiteral("\\bstep\\b"), QStringLiteral("\\bsmoothstep\\b"),
        QStringLiteral("\\blength\\b"), QStringLiteral("\\bdistance\\b"), QStringLiteral("\\breflect\\b"),
        QStringLiteral("\\brefract\\b"), QStringLiteral("\\btexture2D\\b")
    };

    for (const QString &pattern : builtinFuncs) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = builtinFuncsFormat;
        highlightingRules.append(rule);
    }

    // 数字格式
    numberFormat.setForeground(QColor("#B5CEA8"));
    HighlightingRule rule;
    rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+\\.[0-9]+\\b|\\b[0-9]+\\b"));
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // 预处理器格式
    preprocessorFormat.setForeground(QColor("#BD63C5"));
    rule.pattern = QRegularExpression(QStringLiteral("#[^\n]*"));
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);

    // 单行注释格式
    singleLineCommentFormat.setForeground(QColor("#608B4E"));
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // 字符串格式
    quotationFormat.setForeground(QColor("#CE9178"));
    rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // 多行注释格式
    multiLineCommentFormat.setForeground(QColor("#608B4E"));
    commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void GLSLHighlighter::highlightBlock(const QString &text)
{
    // 应用所有高亮规则
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 处理多行注释
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

// GLSLEditor 实现
GLSLEditor::GLSLEditor(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("GLSL Editor");
    resize(800, 600);

    setupEditor();
    createActions();
}

void GLSLEditor::setupEditor()
{
    editor = new CodeEditor(this);
    setCentralWidget(editor);

    highlighter = new GLSLHighlighter(editor->document());

    // 设置初始内容
    editor->setPlainText(R"(// GLSL Fragment Shader
#version 330 core

uniform vec2 resolution;
uniform float time;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec3 color = vec3(uv.x, uv.y, abs(sin(time)));
    gl_FragColor = vec4(color, 1.0);
}
)");
}

void GLSLEditor::createActions()
{
    // 这里可以添加菜单栏和工具栏的操作
}