#include "Sample1.h"
#include <string.h>
#include <ctype.h>

JNIEXPORT jint JNICALL 
Java_Sample1_intMethod(JNIEnv *env, jobject obj, jint num){
   return num * num;
}

JNIEXPORT jboolean JNICALL 
Java_Sample1_booleanMethod(JNIEnv *env, jobject obj, jboolean boolean){
   return !boolean;
}

JNIEXPORT jstring JNICALL 
Java_Sample1_stringMethod(JNIEnv *env, jobject obj, jstring jstr){
     const char *str = env->GetStringUTFChars(jstr, 0);
     char cap[128];
     strcpy(cap, str);
     env->ReleaseStringUTFChars(jstr, str);
	 char* ptr = cap;
	 while (*ptr != '\0'){
		 *ptr = toupper(*ptr);
		 ptr++;
	 }
     return env->NewStringUTF(cap);
}

JNIEXPORT jint JNICALL 
Java_Sample1_intArrayMethod(JNIEnv *env, jobject obj, jintArray array){
     int i, sum = 0;
     jsize len = env->GetArrayLength(array);
     jint *body = env->GetIntArrayElements(array, 0);
     for (i=0; i<len; i++) {
		sum += body[i];
     }
     env->ReleaseIntArrayElements(array, body, 0);
     return sum;
}
