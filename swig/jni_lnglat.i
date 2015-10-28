//  Scheme is based on std_string.i

%{
// Include LngLat class
#include "util/types.h"
%}

// const LngLat &
%typemap(jni) const Tangram::LngLat & "jobject"
%typemap(jtype) const Tangram::LngLat & "LngLat"
%typemap(jstype) const Tangram::LngLat & "LngLat"

%typemap(in) const Tangram::LngLat &
%{
   if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null LngLat");
     return $null;
   }

   static jclass clazz = jenv->FindClass("com/mapzen/tangram/LngLat");

   static jfieldID lonId = jenv->GetFieldID(clazz, "longitude", "D");
   static jfieldID latId = jenv->GetFieldID(clazz, "latitude", "D");

   Tangram::LngLat $1_out;
   $1_out.longitude = jenv->GetDoubleField($input, lonId);
   $1_out.latitude = jenv->GetDoubleField($input, latId);
   $1 = &$1_out;
%}

%typemap(out) const Tangram::LngLat &
%{
   static jclass clazz = jenv->FindClass("com/mapzen/tangram/LngLat");
   static jmethodID constructor = jenv->GetMethodID(clazz, "<init>", "(DD)V");
   $result = jenv->NewObject(clazz, constructor, $1->longitude, $1->latitude);
%}

%typemap(javain) const Tangram::LngLat & "$javainput"

%typemap(javaout) const Tangram::LngLat & {
    return $jnicall;
}

//// LngLat
%typemap(jni) Tangram::LngLat "jobject"
%typemap(jtype) Tangram::LngLat "LngLat"
%typemap(jstype) Tangram::LngLat "LngLat"

%typemap(in) Tangram::LngLat
%{
   if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null LngLat");
     return $null;
   }

   static jclass clazz = jenv->FindClass("com/mapzen/tangram/LngLat");
   static jfieldID lonId = jenv->GetFieldID(clazz, "longitude", "D");
   static jfieldID latId = jenv->GetFieldID(clazz, "latitude", "D");

   $1.longitude = jenv->GetDoubleField($input, lonId);
   $1.latitude = jenv->GetDoubleField($input, latId);

%}

%typemap(out) Tangram::LngLat
%{
   static jclass clazz = jenv->FindClass("com/mapzen/tangram/LngLat");
   static jmethodID constructor = jenv->GetMethodID(clazz, "<init>", "(DD)V");
   $result = jenv->NewObject(clazz, constructor, $1->longitude, $1->latitude);
%}

%typemap(javain) Tangram::LngLat "$javainput"

%typemap(javaout) Tangram::LngLat {
    return $jnicall;
}
