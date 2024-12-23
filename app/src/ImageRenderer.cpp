#include "ImageRenderer.hpp"

#include "Global.hpp"

void ImageRenderer::updateTextureFromCamera()
{

    auto frameBuffer = GlobalResourceManager::getInstance().tripleBuffer.get();
    if (frameBuffer)
    {
        lzx::Frame *frame = frameBuffer->consume();

        // 检查是否要更新
        if (frame && frame->width() > 0 && frame->height() > 0 && frame->channels() > 0)
        {
            // 检查是否需要重建纹理
            if (QSize(texture->width(), texture->height()) != QSize(frame->width(), frame->height()) || texture->format() != QOpenGLTexture::RGBA8_UNorm)
            {
                // 删除旧的纹理
                delete texture;

                // 创建一个新的纹理
                texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
                texture->setSize(frame->width(), frame->height());
                texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
                texture->setWrapMode(QOpenGLTexture::ClampToBorder);
                texture->setBorderColor(QColor(Qt::black));
                texture->allocateStorage();
                texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
                texture->setMagnificationFilter(QOpenGLTexture::Linear);
            }

            // 更新纹理
            updateOpenGLTexture(texture->textureId(), frame->width(), frame->height(), frame->data(), frame->channels());
        }
        frameBuffer->consumeDone();
    }
}

void ImageRenderer::updateOpenGLTexture(GLuint textureID, int width, int height, const GLubyte *data, int bytesPerPixel)
{
    GLenum availFormats[] = {GL_RED, GL_RG, GL_RGB, GL_RGBA};
    GLenum format = availFormats[bytesPerPixel - 1];
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, (bytesPerPixel == 4) ? 4 : 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}