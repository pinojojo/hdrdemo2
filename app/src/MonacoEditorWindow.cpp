#include "MonacoEditorWindow.hpp"
#include <QtWebEngineWidgets/QWebEngineView> // 修改这行
#include <QWebChannel>
#include <QFile>

MonacoEditorWindow::MonacoEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("GLSL编辑器");
    resize(800, 600);

    // 创建中心部件
    auto centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建布局
    auto layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // 创建WebView
    webView = new QWebEngineView(this);
    layout->addWidget(webView);

    // 初始化Monaco Editor
    initializeMonaco();
}

void MonacoEditorWindow::initializeMonaco()
{
    // 加载Monaco Editor
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { margin: 0; }
        #container { width: 100%; height: 100vh; }
    </style>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.36.1/min/vs/loader.min.js"></script>
</head>
<body>
    <div id="container"></div>
    <script>
        require.config({ paths: { 'vs': 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.36.1/min/vs' }});
        require(['vs/editor/editor.main'], function() {
            var editor = monaco.editor.create(document.getElementById('container'), {
                value: '// GLSL Editor\n\nvoid main() {\n    \n}',
                language: 'cpp',
                theme: 'vs-dark',
                automaticLayout: true,
                minimap: {
                    enabled: true
                }
            });

            // 设置GLSL关键字高亮
            monaco.languages.setMonarchTokensProvider('cpp', {
                keywords: [
                    'attribute', 'uniform', 'varying', 'layout', 'centroid', 'flat',
                    'smooth', 'noperspective', 'patch', 'sample', 'break', 'continue',
                    'do', 'for', 'while', 'switch', 'case', 'default', 'if', 'else',
                    'subroutine', 'in', 'out', 'inout', 'float', 'double', 'int',
                    'void', 'bool', 'true', 'false', 'invariant', 'discard', 'return',
                    'mat2', 'mat3', 'mat4', 'vec2', 'vec3', 'vec4', 'ivec2', 'ivec3',
                    'ivec4', 'bvec2', 'bvec3', 'bvec4', 'uint', 'uvec2', 'uvec3',
                    'uvec4', 'lowp', 'mediump', 'highp', 'precision', 'sampler2D',
                    'sampler3D', 'samplerCube', 'struct'
                ],
                tokenizer: {
                    root: [
                        [/[a-zA-Z_]\w*/, {
                            cases: {
                                '@keywords': 'keyword',
                                '@default': 'identifier'
                            }
                        }]
                    ]
                }
            });
        });
    </script>
</body>
</html>
    )";

    webView->setHtml(html);
}