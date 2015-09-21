/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.3
 *
 * This file is not intended to be easily readable and contains a number of
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG
 * interface file instead.
 * ----------------------------------------------------------------------------- */

#define SWIGJAVA


#ifdef __cplusplus
/* SwigValueWrapper is described in swig.swg */
template<typename T> class SwigValueWrapper {
  struct SwigMovePointer {
    T *ptr;
    SwigMovePointer(T *p) : ptr(p) { }
    ~SwigMovePointer() { delete ptr; }
    SwigMovePointer& operator=(SwigMovePointer& rhs) { T* oldptr = ptr; ptr = 0; delete oldptr; ptr = rhs.ptr; rhs.ptr = 0; return *this; }
  } pointer;
  SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
  SwigValueWrapper(const SwigValueWrapper<T>& rhs);
public:
  SwigValueWrapper() : pointer(0) { }
  SwigValueWrapper& operator=(const T& t) { SwigMovePointer tmp(new T(t)); pointer = tmp; return *this; }
  operator T&() const { return *pointer.ptr; }
  T *operator&() { return pointer.ptr; }
};

template <typename T> T SwigValueInit() {
  return T();
}
#endif

/* -----------------------------------------------------------------------------
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 * ----------------------------------------------------------------------------- */

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
# if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#  define SWIGTEMPLATEDISAMBIGUATOR template
# elif defined(__HP_aCC)
/* Needed even with `aCC -AA' when `aCC -V' reports HP ANSI C++ B3910B A.03.55 */
/* If we find a maximum version that requires this, the test would be __HP_aCC <= 35500 for A.03.55 */
#  define SWIGTEMPLATEDISAMBIGUATOR template
# else
#  define SWIGTEMPLATEDISAMBIGUATOR
# endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
# if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
#   define SWIGINLINE inline
# else
#   define SWIGINLINE
# endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
# if defined(__GNUC__)
#   if !(defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define SWIGUNUSED __attribute__ ((__unused__))
#   else
#     define SWIGUNUSED
#   endif
# elif defined(__ICC)
#   define SWIGUNUSED __attribute__ ((__unused__))
# else
#   define SWIGUNUSED
# endif
#endif

#ifndef SWIG_MSC_UNSUPPRESS_4505
# if defined(_MSC_VER)
#   pragma warning(disable : 4505) /* unreferenced local function has been removed */
# endif
#endif

#ifndef SWIGUNUSEDPARM
# ifdef __cplusplus
#   define SWIGUNUSEDPARM(p)
# else
#   define SWIGUNUSEDPARM(p) p SWIGUNUSED
# endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
# define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
# define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods */
#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  ifndef GCC_HASCLASSVISIBILITY
#    define GCC_HASCLASSVISIBILITY
#  endif
#endif

#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define SWIGSTDCALL __stdcall
# else
#   define SWIGSTDCALL
# endif
#endif

/* Deal with Microsoft's attempt at deprecating C standard runtime functions */
#if !defined(SWIG_NO_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
# define _CRT_SECURE_NO_DEPRECATE
#endif

/* Deal with Microsoft's attempt at deprecating methods in the standard C++ library */
#if !defined(SWIG_NO_SCL_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_SCL_SECURE_NO_DEPRECATE)
# define _SCL_SECURE_NO_DEPRECATE
#endif



/* Fix for jlong on some versions of gcc on Windows */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
  typedef long long __int64;
#endif

/* Fix for jlong on 64-bit x86 Solaris */
#if defined(__x86_64)
# ifdef _LP64
#   undef _LP64
# endif
#endif

#include <jni.h>
#include <stdlib.h>
#include <string.h>


/* Support for throwing Java exceptions */
typedef enum {
  SWIG_JavaOutOfMemoryError = 1,
  SWIG_JavaIOException,
  SWIG_JavaRuntimeException,
  SWIG_JavaIndexOutOfBoundsException,
  SWIG_JavaArithmeticException,
  SWIG_JavaIllegalArgumentException,
  SWIG_JavaNullPointerException,
  SWIG_JavaDirectorPureVirtual,
  SWIG_JavaUnknownError
} SWIG_JavaExceptionCodes;

typedef struct {
  SWIG_JavaExceptionCodes code;
  const char *java_exception;
} SWIG_JavaExceptions_t;


static void SWIGUNUSED SWIG_JavaThrowException(JNIEnv *jenv, SWIG_JavaExceptionCodes code, const char *msg) {
  jclass excep;
  static const SWIG_JavaExceptions_t java_exceptions[] = {
    { SWIG_JavaOutOfMemoryError, "java/lang/OutOfMemoryError" },
    { SWIG_JavaIOException, "java/io/IOException" },
    { SWIG_JavaRuntimeException, "java/lang/RuntimeException" },
    { SWIG_JavaIndexOutOfBoundsException, "java/lang/IndexOutOfBoundsException" },
    { SWIG_JavaArithmeticException, "java/lang/ArithmeticException" },
    { SWIG_JavaIllegalArgumentException, "java/lang/IllegalArgumentException" },
    { SWIG_JavaNullPointerException, "java/lang/NullPointerException" },
    { SWIG_JavaDirectorPureVirtual, "java/lang/RuntimeException" },
    { SWIG_JavaUnknownError,  "java/lang/UnknownError" },
    { (SWIG_JavaExceptionCodes)0,  "java/lang/UnknownError" }
  };
  const SWIG_JavaExceptions_t *except_ptr = java_exceptions;

  while (except_ptr->code != code && except_ptr->code)
    except_ptr++;

  jenv->ExceptionClear();
  excep = jenv->FindClass(except_ptr->java_exception);
  if (excep)
    jenv->ThrowNew(excep, msg);
}


/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_JavaThrowException(jenv, SWIG_JavaIllegalArgumentException, msg); return nullreturn; } else


#include "data/properties.h"
#include <string>
#include <memory>


#include <stdexcept>


#include <string>


struct SWIG_null_deleter {
  void operator() (void const *) const {
  }
};
#define SWIG_NO_NULL_DELETER_0 , SWIG_null_deleter()
#define SWIG_NO_NULL_DELETER_1
#define SWIG_NO_NULL_DELETER_SWIG_POINTER_NEW
#define SWIG_NO_NULL_DELETER_SWIG_POINTER_OWN

SWIGINTERN void Tangram_Properties_add__SWIG_0(Tangram::Properties *self,std::string key,std::string value){
        self->add(key, value);
    }
SWIGINTERN void Tangram_Properties_add__SWIG_1(Tangram::Properties *self,std::string key,float value){
        self->add(key, value);
    }

#include <map>
#include <algorithm>
#include <stdexcept>

SWIGINTERN std::string const &std_map_Sl_std_string_Sc_std_string_Sg__get(std::map< std::string,std::string > *self,std::string const &key){
                std::map<std::string,std::string >::iterator i = self->find(key);
                if (i != self->end())
                    return i->second;
                else
                    throw std::out_of_range("key not found");
            }
SWIGINTERN void std_map_Sl_std_string_Sc_std_string_Sg__set(std::map< std::string,std::string > *self,std::string const &key,std::string const &x){
                (*self)[key] = x;
            }
SWIGINTERN void std_map_Sl_std_string_Sc_std_string_Sg__del(std::map< std::string,std::string > *self,std::string const &key){
                std::map<std::string,std::string >::iterator i = self->find(key);
                if (i != self->end())
                    self->erase(i);
                else
                    throw std::out_of_range("key not found");
            }
SWIGINTERN bool std_map_Sl_std_string_Sc_std_string_Sg__has_key(std::map< std::string,std::string > *self,std::string const &key){
                std::map<std::string,std::string >::iterator i = self->find(key);
                return i != self->end();
            }

#include "tangram.h"
#include "data/dataSource.h"
#include "data/clientGeoJsonSource.h"


static int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input) {
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }

   jboolean isCopy;

   *carr = (float*) jenv->GetPrimitiveArrayCritical(input, &isCopy);
   if (!*carr)
      return 0;

  return 1;
}

static void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input) {
  /* int i;
   * jsize sz = jenv->GetArrayLength(input);
   * for (i=0; i<sz; i++)
   *   jarr[i] = (jfloat)carr[i];
   * jenv->ReleaseFloatArrayElements(input, jarr, 0); */

  jenv->ReleasePrimitiveArrayCritical(input, carr, JNI_ABORT);
}

static jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz) {
  jfloat *arr;
  int i;
  jfloatArray jresult = jenv->NewFloatArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetFloatArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jfloat)result[i];
  jenv->ReleaseFloatArrayElements(jresult, arr, 0);
  return jresult;
}


static int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input);
static void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input);
static jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz);


static int SWIG_JavaArrayInDouble (JNIEnv *jenv, jdouble **jarr, double **carr, jdoubleArray input) {
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }

   jboolean isCopy;

   *carr = (double*) jenv->GetPrimitiveArrayCritical(input, &isCopy);
   if (!*carr)
      return 0;

  return 1;
}

static void SWIG_JavaArrayArgoutDouble (JNIEnv *jenv, jdouble *jarr, double *carr, jdoubleArray input) {
  /* int i;
   * jsize sz = jenv->GetArrayLength(input);
   * for (i=0; i<sz; i++)
   *   jarr[i] = (jdouble)carr[i];
   * jenv->ReleaseDoubleArrayElements(input, jarr, 0); */

  jenv->ReleasePrimitiveArrayCritical(input, carr, JNI_ABORT);
}

static jdoubleArray SWIG_JavaArrayOutDouble (JNIEnv *jenv, double *result, jsize sz) {
  jdouble *arr;
  int i;
  jdoubleArray jresult = jenv->NewDoubleArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetDoubleArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jdouble)result[i];
  jenv->ReleaseDoubleArrayElements(jresult, arr, 0);
  return jresult;
}


static int SWIG_JavaArrayInDouble (JNIEnv *jenv, jdouble **jarr, double **carr, jdoubleArray input);
static void SWIG_JavaArrayArgoutDouble (JNIEnv *jenv, jdouble *jarr, double *carr, jdoubleArray input);
static jdoubleArray SWIG_JavaArrayOutDouble (JNIEnv *jenv, double *result, jsize sz);


static int SWIG_JavaArrayInInt (JNIEnv *jenv, jint **jarr, int **carr, jintArray input) {
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }

   jboolean isCopy;

   *carr = (int*) jenv->GetPrimitiveArrayCritical(input, &isCopy);
   if (!*carr)
      return 0;

  return 1;
}

static void SWIG_JavaArrayArgoutInt (JNIEnv *jenv, jint *jarr, int *carr, jintArray input) {
  /* int i;
   * jsize sz = jenv->GetArrayLength(input);
   * for (i=0; i<sz; i++)
   *   jarr[i] = (jint)carr[i];
   * jenv->ReleaseIntArrayElements(input, jarr, 0); */

  jenv->ReleasePrimitiveArrayCritical(input, carr, JNI_ABORT);
}

static jintArray SWIG_JavaArrayOutInt (JNIEnv *jenv, int *result, jsize sz) {
  jint *arr;
  int i;
  jintArray jresult = jenv->NewIntArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetIntArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jint)result[i];
  jenv->ReleaseIntArrayElements(jresult, arr, 0);
  return jresult;
}


static int SWIG_JavaArrayInInt (JNIEnv *jenv, jint **jarr, int **carr, jintArray input);
static void SWIG_JavaArrayArgoutInt (JNIEnv *jenv, jint *jarr, int *carr, jintArray input);
static jintArray SWIG_JavaArrayOutInt (JNIEnv *jenv, int *result, jsize sz);

SWIGINTERN void Tangram_DataSource_update(Tangram::DataSource *self){
        Tangram::clearDataSource(*(self), false, true);
    }
SWIGINTERN void Tangram_DataSource_clear(Tangram::DataSource *self){
        Tangram::clearDataSource(*(self), true, true);
    }
SWIGINTERN std::string const &Tangram_DataSource_name(Tangram::DataSource *self){
         self->name();
    }

#ifdef __cplusplus
extern "C" {
#endif

SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_new_1Properties(JNIEnv *jenv, jclass jcls) {
  jlong jresult = 0 ;
  Tangram::Properties *result = 0 ;

  (void)jenv;
  (void)jcls;
  result = (Tangram::Properties *)new Tangram::Properties();

  *(std::shared_ptr<  Tangram::Properties > **)&jresult = result ? new std::shared_ptr<  Tangram::Properties >(result SWIG_NO_NULL_DELETER_1) : 0;

  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1clear(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::shared_ptr< Tangram::Properties > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  (arg1)->clear();
}


SWIGEXPORT jboolean JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1contains(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jboolean jresult = 0 ;
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::string *arg2 = 0 ;
  std::shared_ptr< Tangram::Properties const > *smartarg1 = 0 ;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr< const Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (bool)((Tangram::Properties const *)arg1)->contains((std::string const &)*arg2);
  jresult = (jboolean)result;
  return jresult;
}


SWIGEXPORT jfloat JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1getNumeric(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jfloat jresult = 0 ;
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::string *arg2 = 0 ;
  std::shared_ptr< Tangram::Properties const > *smartarg1 = 0 ;
  float result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr< const Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (float)((Tangram::Properties const *)arg1)->getNumeric((std::string const &)*arg2);
  jresult = (jfloat)result;
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1getString(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jstring jresult = 0 ;
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::string *arg2 = 0 ;
  std::shared_ptr< Tangram::Properties const > *smartarg1 = 0 ;
  std::string *result = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr< const Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (std::string *) &((Tangram::Properties const *)arg1)->getString((std::string const &)*arg2);
  jresult = jenv->NewStringUTF(result->c_str());
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1add_1_1SWIG_10(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::string arg2 ;
  std::string arg3 ;
  std::shared_ptr< Tangram::Properties > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return ;
  (&arg2)->assign(arg2_pstr);
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if(!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg3_pstr = (const char *)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr) return ;
  (&arg3)->assign(arg3_pstr);
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  Tangram_Properties_add__SWIG_0(arg1,arg2,arg3);
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Properties_1add_1_1SWIG_11(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jfloat jarg3) {
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::string arg2 ;
  float arg3 ;
  std::shared_ptr< Tangram::Properties > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return ;
  (&arg2)->assign(arg2_pstr);
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = (float)jarg3;
  Tangram_Properties_add__SWIG_1(arg1,arg2,arg3);
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_delete_1Properties(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  Tangram::Properties *arg1 = (Tangram::Properties *) 0 ;
  std::shared_ptr< Tangram::Properties > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;

  smartarg1 = *(std::shared_ptr<  Tangram::Properties > **)&jarg1;
  arg1 = (Tangram::Properties *)(smartarg1 ? smartarg1->get() : 0);
  (void)arg1; delete smartarg1;
}


SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_new_1Tags_1_1SWIG_10(JNIEnv *jenv, jclass jcls) {
  jlong jresult = 0 ;
  std::map< std::string,std::string > *result = 0 ;

  (void)jenv;
  (void)jcls;
  result = (std::map< std::string,std::string > *)new std::map< std::string,std::string >();
  *(std::map< std::string,std::string > **)&jresult = result;
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_new_1Tags_1_1SWIG_11(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  std::map< std::string,std::string > *arg1 = 0 ;
  std::map< std::string,std::string > *result = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::map< std::string,std::string > const & reference is null");
    return 0;
  }
  result = (std::map< std::string,std::string > *)new std::map< std::string,std::string >((std::map< std::string,std::string > const &)*arg1);
  *(std::map< std::string,std::string > **)&jresult = result;
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1size(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  unsigned int result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  result = (unsigned int)((std::map< std::string,std::string > const *)arg1)->size();
  jresult = (jlong)result;
  return jresult;
}


SWIGEXPORT jboolean JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1empty(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jboolean jresult = 0 ;
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  result = (bool)((std::map< std::string,std::string > const *)arg1)->empty();
  jresult = (jboolean)result;
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1clear(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  (arg1)->clear();
}


SWIGEXPORT jstring JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1get(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jstring jresult = 0 ;
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  std::string *arg2 = 0 ;
  std::string *result = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  try {
    result = (std::string *) &std_map_Sl_std_string_Sc_std_string_Sg__get(arg1,(std::string const &)*arg2);
  }
  catch(std::out_of_range &_e) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, (&_e)->what());
    return 0;
  }

  jresult = jenv->NewStringUTF(result->c_str());
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1set(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  std::string *arg2 = 0 ;
  std::string *arg3 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return ;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if(!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg3_pstr = (const char *)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr) return ;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  std_map_Sl_std_string_Sc_std_string_Sg__set(arg1,(std::string const &)*arg2,(std::string const &)*arg3);
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1del(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  std::string *arg2 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return ;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  try {
    std_map_Sl_std_string_Sc_std_string_Sg__del(arg1,(std::string const &)*arg2);
  }
  catch(std::out_of_range &_e) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, (&_e)->what());
    return ;
  }

}


SWIGEXPORT jboolean JNICALL Java_com_mapzen_tangram_TangramJNI_Tags_1has_1key(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jboolean jresult = 0 ;
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;
  std::string *arg2 = 0 ;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (bool)std_map_Sl_std_string_Sc_std_string_Sg__has_key(arg1,(std::string const &)*arg2);
  jresult = (jboolean)result;
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_delete_1Tags(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  std::map< std::string,std::string > *arg1 = (std::map< std::string,std::string > *) 0 ;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::map< std::string,std::string > **)&jarg1;
  delete arg1;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_DataSource_1update(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  Tangram::DataSource *arg1 = (Tangram::DataSource *) 0 ;
  std::shared_ptr< Tangram::DataSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::DataSource > **)&jarg1;
  arg1 = (Tangram::DataSource *)(smartarg1 ? smartarg1->get() : 0);
  Tangram_DataSource_update(arg1);
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_DataSource_1clear(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  Tangram::DataSource *arg1 = (Tangram::DataSource *) 0 ;
  std::shared_ptr< Tangram::DataSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::DataSource > **)&jarg1;
  arg1 = (Tangram::DataSource *)(smartarg1 ? smartarg1->get() : 0);
  Tangram_DataSource_clear(arg1);
}


SWIGEXPORT jstring JNICALL Java_com_mapzen_tangram_TangramJNI_DataSource_1name(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  Tangram::DataSource *arg1 = (Tangram::DataSource *) 0 ;
  std::shared_ptr< Tangram::DataSource > *smartarg1 = 0 ;
  std::string *result = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::DataSource > **)&jarg1;
  arg1 = (Tangram::DataSource *)(smartarg1 ? smartarg1->get() : 0);
  result = (std::string *) &Tangram_DataSource_name(arg1);
  jresult = jenv->NewStringUTF(result->c_str());
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_delete_1DataSource(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  Tangram::DataSource *arg1 = (Tangram::DataSource *) 0 ;
  std::shared_ptr< Tangram::DataSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;

  smartarg1 = *(std::shared_ptr<  Tangram::DataSource > **)&jarg1;
  arg1 = (Tangram::DataSource *)(smartarg1 ? smartarg1->get() : 0);
  (void)arg1; delete smartarg1;
}


SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_new_1MapData(JNIEnv *jenv, jclass jcls, jstring jarg1, jstring jarg2) {
  jlong jresult = 0 ;
  std::string *arg1 = 0 ;
  std::string *arg2 = 0 ;
  Tangram::ClientGeoJsonSource *result = 0 ;

  (void)jenv;
  (void)jcls;
  if(!jarg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg1_pstr = (const char *)jenv->GetStringUTFChars(jarg1, 0);
  if (!arg1_pstr) return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(jarg1, arg1_pstr);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (Tangram::ClientGeoJsonSource *)new Tangram::ClientGeoJsonSource((std::string const &)*arg1,(std::string const &)*arg2);

  *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jresult = result ? new std::shared_ptr<  Tangram::ClientGeoJsonSource >(result SWIG_NO_NULL_DELETER_1) : 0;

  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1addData(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::string *arg2 = 0 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  if(!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return ;
  }
  const char *arg2_pstr = (const char *)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr) return ;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  (arg1)->addData((std::string const &)*arg2);
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1addPoint(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2, jobject jarg2_, jdoubleArray jarg3) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::map< std::string,std::string > arg2 ;
  double *arg3 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;
  std::map< std::string,std::string > *argp2 ;
  jdouble *jarr3 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  (void)jarg2_;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  argp2 = *(std::map< std::string,std::string > **)&jarg2;
  if (!argp2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "Attempt to dereference null std::map< std::string,std::string >");
    return ;
  }
  arg2 = *argp2;
  if (!SWIG_JavaArrayInDouble(jenv, &jarr3, (double **)&arg3, jarg3)) return ;
  (arg1)->addPoint(arg2,arg3);
  SWIG_JavaArrayArgoutDouble(jenv, jarr3, (double *)arg3, jarg3);

}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1addLine(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2, jobject jarg2_, jdoubleArray jarg3, jint jarg4) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::map< std::string,std::string > arg2 ;
  double *arg3 ;
  int arg4 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;
  std::map< std::string,std::string > *argp2 ;
  jdouble *jarr3 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  (void)jarg2_;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  argp2 = *(std::map< std::string,std::string > **)&jarg2;
  if (!argp2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "Attempt to dereference null std::map< std::string,std::string >");
    return ;
  }
  arg2 = *argp2;
  if (!SWIG_JavaArrayInDouble(jenv, &jarr3, (double **)&arg3, jarg3)) return ;
  arg4 = (int)jarg4;
  (arg1)->addLine(arg2,arg3,arg4);
  SWIG_JavaArrayArgoutDouble(jenv, jarr3, (double *)arg3, jarg3);

}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1addPoly(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2, jobject jarg2_, jdoubleArray jarg3, jintArray jarg4, jint jarg5) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::map< std::string,std::string > arg2 ;
  double *arg3 ;
  int *arg4 ;
  int arg5 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;
  std::map< std::string,std::string > *argp2 ;
  jdouble *jarr3 ;
  jint *jarr4 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  (void)jarg2_;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  argp2 = *(std::map< std::string,std::string > **)&jarg2;
  if (!argp2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "Attempt to dereference null std::map< std::string,std::string >");
    return ;
  }
  arg2 = *argp2;
  if (!SWIG_JavaArrayInDouble(jenv, &jarr3, (double **)&arg3, jarg3)) return ;
  if (!SWIG_JavaArrayInInt(jenv, &jarr4, (int **)&arg4, jarg4)) return ;
  arg5 = (int)jarg5;
  (arg1)->addPoly(arg2,arg3,arg4,arg5);
  SWIG_JavaArrayArgoutDouble(jenv, jarr3, (double *)arg3, jarg3);
  SWIG_JavaArrayArgoutInt(jenv, jarr4, (int *)arg4, jarg4);


}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1clearData(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  (arg1)->clearData();
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_delete_1MapData(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  Tangram::ClientGeoJsonSource *arg1 = (Tangram::ClientGeoJsonSource *) 0 ;
  std::shared_ptr< Tangram::ClientGeoJsonSource > *smartarg1 = 0 ;

  (void)jenv;
  (void)jcls;

  smartarg1 = *(std::shared_ptr<  Tangram::ClientGeoJsonSource > **)&jarg1;
  arg1 = (Tangram::ClientGeoJsonSource *)(smartarg1 ? smartarg1->get() : 0);
  (void)arg1; delete smartarg1;
}


SWIGEXPORT jint JNICALL Java_com_mapzen_tangram_TangramJNI_addDataSource(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jint jresult = 0 ;
  std::shared_ptr< Tangram::DataSource > arg1 ;
  std::shared_ptr< Tangram::DataSource > *argp1 ;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  argp1 = *(std::shared_ptr< Tangram::DataSource > **)&jarg1;
  if (argp1) arg1 = *argp1;
  result = (int)Tangram::addDataSource(arg1);
  jresult = (jint)result;
  return jresult;
}


SWIGEXPORT void JNICALL Java_com_mapzen_tangram_TangramJNI_clearDataSource(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jboolean jarg2, jboolean jarg3) {
  Tangram::DataSource *arg1 = 0 ;
  bool arg2 ;
  bool arg3 ;

  (void)jenv;
  (void)jcls;
  (void)jarg1_;

  arg1 = (Tangram::DataSource *)((*(std::shared_ptr<  Tangram::DataSource > **)&jarg1) ? (*(std::shared_ptr<  Tangram::DataSource > **)&jarg1)->get() : 0);
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "Tangram::DataSource & reference is null");
    return ;
  }
  arg2 = jarg2 ? true : false;
  arg3 = jarg3 ? true : false;
  Tangram::clearDataSource(*arg1,arg2,arg3);
}


SWIGEXPORT jlong JNICALL Java_com_mapzen_tangram_TangramJNI_MapData_1SWIGSmartPtrUpcast(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    jlong baseptr = 0;
    std::shared_ptr< Tangram::ClientGeoJsonSource > *argp1;
    (void)jenv;
    (void)jcls;
    argp1 = *(std::shared_ptr< Tangram::ClientGeoJsonSource > **)&jarg1;
    *(std::shared_ptr< Tangram::DataSource > **)&baseptr = argp1 ? new std::shared_ptr< Tangram::DataSource >(*argp1) : 0;
    return baseptr;
}

#ifdef __cplusplus
}
#endif
