#include <jni.h>
extern "C" JNIEXPORT jboolean JNICALL Java_com_zoavintsoa_secondscreen_NativeMsQuicTransport_nativeAvailable(JNIEnv*, jobject){return JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL Java_com_zoavintsoa_secondscreen_NativeMsQuicTransport_nativeConnect(JNIEnv*, jobject, jstring, jint, jstring){return JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL Java_com_zoavintsoa_secondscreen_NativeMsQuicTransport_nativeSendControl(JNIEnv*, jobject, jbyteArray){return JNI_FALSE;}
extern "C" JNIEXPORT jboolean JNICALL Java_com_zoavintsoa_secondscreen_NativeMsQuicTransport_nativeSendKeyframeRequest(JNIEnv*, jobject){return JNI_FALSE;}
extern "C" JNIEXPORT void JNICALL Java_com_zoavintsoa_secondscreen_NativeMsQuicTransport_nativeClose(JNIEnv*, jobject){}
