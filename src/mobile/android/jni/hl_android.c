/*
 * Copyright (C)2005-2017 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#if __ANDROID__

#include <mobile/android/hl_android.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>

static JavaVM *jvm;

struct jcallBundle {
	jclass classCalled;
	jmethodID methID;
} jcallBundle_d;


static JNIEnv* getEnv()
{
	JNIEnv *env;
	jint rs = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	assert (rs == JNI_OK);
	return env;
}

//to use when a local ref is needed
static struct jcallBundle getClassStaticMethod(JNIEnv *env , const char* class, const char* method, const char* paramAndReturn)
{
	//LOG_ANDROID("JNI class invoke");
	struct jcallBundle bundle;

	bundle.classCalled = (*env)->FindClass(env, class);
	if ((*env)->ExceptionOccurred(env))
	{
		ERROR_ANDROID_FMT("Could not find class : %s ", class);
	}
	bundle.methID = (*env)->GetStaticMethodID(env, bundle.classCalled, method, paramAndReturn);
	if ((*env)->ExceptionOccurred(env))
	{
		ERROR_ANDROID_FMT("Could not find method : %s \n in class : %s", method, class);
	}

	return bundle;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	jvm = vm;
	LOG_ANDROID("JNI_OnLoad() called");
	if ((*jvm)->GetEnv(jvm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOG_ANDROID("Failed to get the JVM environment through JavaVM::GetEnv()");
		return -1;
	}
	return JNI_VERSION_1_6;
}


const char* hl_mobile_get_resource_path(const char* file)
{
	jstring ret;

	JNIEnv *env = getEnv();

	struct jcallBundle resourcePath = getClassStaticMethod(env, "com/playdigious/evoland2/Utils", "getResourcePath", "(Ljava/lang/String;)Ljava/lang/String;");

	jobject str = (*env)->NewStringUTF(env, file);

	ret = (jstring)(*env)->CallStaticObjectMethod(env, resourcePath.classCalled, resourcePath.methID, str);

	const char *string = (*env)->GetStringUTFChars(env, ret, NULL);

	(*env)->ReleaseStringUTFChars(env, ret, string);
	(*env)->DeleteLocalRef(env, ret);
	(*env)->DeleteLocalRef(env, str);
	(*env)->DeleteLocalRef(env, resourcePath.classCalled);

	return string;
}

const char* hl_mobile_get_document_path(const char* file)
{
	jstring ret;

	JNIEnv *env = getEnv();

	struct jcallBundle documentPath = getClassStaticMethod(env, "com/playdigious/evoland2/Utils", "getDocumentPath", "(Ljava/lang/String;)Ljava/lang/String;");

	jobject str = (*env)->NewStringUTF(env, file);

	ret = (jstring)(*env)->CallStaticObjectMethod(env, documentPath.classCalled, documentPath.methID, str);

    const char *string = (*env)->GetStringUTFChars(env, ret, NULL);

	(*env)->ReleaseStringUTFChars(env, ret, string);
	(*env)->DeleteLocalRef(env, ret);
	(*env)->DeleteLocalRef(env, str);
	(*env)->DeleteLocalRef(env, documentPath.classCalled);

	return string;
}

bool hl_mobile_file_exists(const char* file)
{
	jboolean ret;

	JNIEnv *env = getEnv();

	struct jcallBundle exists = getClassStaticMethod(env, "com/playdigious/evoland2/Utils", "exists", "(Ljava/lang/String;)Z");

	jobject str = (*env)->NewStringUTF(env, file);

	ret = (*env)->CallStaticBooleanMethod(env, exists.classCalled, exists.methID, str);

	(*env)->DeleteLocalRef(env, str);
	(*env)->DeleteLocalRef(env, exists.classCalled);

	return (ret != JNI_FALSE) ? true : false;
}

bool hl_mobile_create_directory(const char* dir, int mode)
{
	jboolean ret;

	JNIEnv *env = getEnv();

	struct jcallBundle createDir = getClassStaticMethod(env, "com/playdigious/evoland2/Utils", "createDir", "(Ljava/lang/String;)Z");

	jobject str = (*env)->NewStringUTF(env, dir);

	ret = (*env)->CallStaticBooleanMethod(env, createDir.classCalled, createDir.methID, str);

	(*env)->DeleteLocalRef(env, str);
	(*env)->DeleteLocalRef(env, createDir.classCalled);

	return (ret != JNI_FALSE) ? true : false;
}

#endif
