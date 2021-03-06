#include "androidimageprovider.h"

AndroidImageProvider::AndroidImageProvider()
: QQuickImageProvider(QQmlImageProviderBase::Image)
{
    Java env;
    jclass clazz = env.findClass("de/hsaugsburg/informatik/mplane/ImageHelper");
    m_instance = env.createInstance(clazz);
    m_loadApplicationIcon = env->GetMethodID(clazz, "loadApplicationIcon", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");

    jclass bitmapClass = env.findClass("android/graphics/Bitmap");
    m_getWidth = env->GetMethodID(bitmapClass, "getWidth", "()I");
    m_getHeight = env->GetMethodID(bitmapClass, "getHeight", "()I");
    m_getPixels = env->GetMethodID(bitmapClass, "getPixels", "([IIIIIII)V");
}

AndroidImageProvider::~AndroidImageProvider()
{
    Java env;
    env->DeleteGlobalRef(m_instance);
}

QImage AndroidImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    Java env;

    jstring packageName = env->NewString((const jchar *)id.constData(), id.length());
    jobject bm = env->CallObjectMethod(m_instance, m_loadApplicationIcon, packageName);
    env->DeleteLocalRef(packageName);

    if (bm == NULL)
    {
        return QImage();
    }

    jint width = env->CallIntMethod(bm, m_getWidth);
    jint height = env->CallIntMethod(bm, m_getHeight);

    if (size)
    {
        *size = QSize(width, height);
    }

    jintArray pixels = env->NewIntArray(width * height);
    env->CallVoidMethod(bm, m_getPixels, pixels, 0, width, 0, 0, width, height);

    jint *realPixels = env->GetIntArrayElements(pixels, NULL);

    CleanupInfo *cleanupInfo = new CleanupInfo;
    cleanupInfo->pixels = (jintArray)env->NewGlobalRef(pixels);
    cleanupInfo->realPixels = realPixels;

    QImage image((uchar *)realPixels, width, height, QImage::Format_ARGB32, cleanupHandler, (void *)cleanupInfo);
    env->DeleteLocalRef(pixels);
    env->DeleteLocalRef(bm);
    return image;
}

void AndroidImageProvider::cleanupHandler(void *info)
{
    CleanupInfo *cleanupInfo = reinterpret_cast<CleanupInfo *>(info);

    Java env;
    env->ReleaseIntArrayElements(cleanupInfo->pixels, cleanupInfo->realPixels, 0);
    env->DeleteGlobalRef(cleanupInfo->pixels);

    delete cleanupInfo;
}

namespace
{
    static int init_AndroidImageProvider()
    {
        Java::registerClass("de/hsaugsburg/informatik/mplane/ImageHelper");
        Java::registerClass("android/graphics/Bitmap");
        return 1;
    }

    static int __AndroidImageProvider = init_AndroidImageProvider();
}
