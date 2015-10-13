
/********************* float[] support *********************/

%{
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
%}
/*@SWIG@*/         /* float[] */
/*@SWIG:arrays_java.i,38,JAVA_ARRAYS_IMPL@*/

%{
static int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input);
static void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input);
static jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz);
%}
/*@SWIG:arrays_java.i,29,JAVA_ARRAYS_DECL@*/


%typemap(jni) float[ANY], float[]               %{jfloatArray%}
%typemap(jtype) float[ANY], float[]             %{float[]%}
%typemap(jstype) float[ANY], float[]            %{float[]%}

%typemap(in) float[] (jfloat *jarr)
%{  if (!SWIG_JavaArrayInFloat(jenv, &jarr, (float **)&$1, $input)) return $null; %}

%typemap(in) float[ANY] (jfloat *jarr)
%{  if ($input && jenv->GetArrayLength($input) != $1_size) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "incorrect array size");
    return $null;
  }
  if (!SWIG_JavaArrayInFloat(jenv, &jarr, (float **)&$1, $input)) return $null; 
%}

%typemap(argout) float[ANY], float[] 
%{ SWIG_JavaArrayArgoutFloat(jenv, jarr$argnum, (float *)$1, $input); %}

%typemap(out) float[ANY]
%{$result = SWIG_JavaArrayOutFloat(jenv, (float *)$1, $1_dim0); %}

%typemap(out) float[] 
%{$result = SWIG_JavaArrayOutFloat(jenv, (float *)$1, FillMeInAsSizeCannotBeDeterminedAutomatically); %}

/* %typemap(freearg) float[ANY], float[] 
 * %{ delete [] $1; %} */

%typemap(javain) float[ANY], float[] "$javainput"

%typemap(javaout) float[ANY], float[] {
    return $jnicall;
  }

%typemap(memberin) float[ANY], float[];
%typemap(globalin) float[ANY], float[];
/*@SWIG@*/         /* float[ANY] */
/*@SWIG:arrays_java.i,143,JAVA_ARRAYS_TYPEMAPS@*/


%typemap(typecheck, precedence=      1080    )  /* Java float[] */
    float[ANY], float[]
    ""


/********************* double[] support *********************/

%{
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
%}
/*@SWIG@*/         /* double[] */
/*@SWIG:arrays_java.i,38,JAVA_ARRAYS_IMPL@*/

%{
static int SWIG_JavaArrayInDouble (JNIEnv *jenv, jdouble **jarr, double **carr, jdoubleArray input);
static void SWIG_JavaArrayArgoutDouble (JNIEnv *jenv, jdouble *jarr, double *carr, jdoubleArray input);
static jdoubleArray SWIG_JavaArrayOutDouble (JNIEnv *jenv, double *result, jsize sz);
%}
/*@SWIG:arrays_java.i,29,JAVA_ARRAYS_DECL@*/


%typemap(jni) double[ANY], double[]               %{jdoubleArray%}
%typemap(jtype) double[ANY], double[]             %{double[]%}
%typemap(jstype) double[ANY], double[]            %{double[]%}

%typemap(in) double[] (jdouble *jarr)
%{  if (!SWIG_JavaArrayInDouble(jenv, &jarr, (double **)&$1, $input)) return $null; %}

%typemap(in) double[ANY] (jdouble *jarr)
%{  if ($input && jenv->GetArrayLength($input) != $1_size) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "incorrect array size");
    return $null;
  }
  if (!SWIG_JavaArrayInDouble(jenv, &jarr, (double **)&$1, $input)) return $null; 
%}

%typemap(argout) double[ANY], double[] 
%{ SWIG_JavaArrayArgoutDouble(jenv, jarr$argnum, (double *)$1, $input); %}

%typemap(out) double[ANY]
%{$result = SWIG_JavaArrayOutDouble(jenv, (double *)$1, $1_dim0); %}

%typemap(out) double[] 
%{$result = SWIG_JavaArrayOutDouble(jenv, (double *)$1, FillMeInAsSizeCannotBeDeterminedAutomatically); %}

/* %typemap(freearg) double[ANY], double[] 
 * %{ delete [] $1; %} */

%typemap(javain) double[ANY], double[] "$javainput"

%typemap(javaout) double[ANY], double[] {
    return $jnicall;
  }

%typemap(memberin) double[ANY], double[];
%typemap(globalin) double[ANY], double[];
/*@SWIG@*/         /* double[ANY] */
/*@SWIG:arrays_java.i,143,JAVA_ARRAYS_TYPEMAPS@*/


%typemap(typecheck, precedence=      1080    )  /* Java double[] */
    double[ANY], double[]
    ""


/********************* int[] support *********************/

%{
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
%}
/*@SWIG@*/         /* int[] */
/*@SWIG:arrays_java.i,38,JAVA_ARRAYS_IMPL@*/

%{
static int SWIG_JavaArrayInInt (JNIEnv *jenv, jint **jarr, int **carr, jintArray input);
static void SWIG_JavaArrayArgoutInt (JNIEnv *jenv, jint *jarr, int *carr, jintArray input);
static jintArray SWIG_JavaArrayOutInt (JNIEnv *jenv, int *result, jsize sz);
%}
/*@SWIG:arrays_java.i,29,JAVA_ARRAYS_DECL@*/


%typemap(jni) int[ANY], int[]               %{jintArray%}
%typemap(jtype) int[ANY], int[]             %{int[]%}
%typemap(jstype) int[ANY], int[]            %{int[]%}

%typemap(in) int[] (jint *jarr)
%{  if (!SWIG_JavaArrayInInt(jenv, &jarr, (int **)&$1, $input)) return $null; %}

%typemap(in) int[ANY] (jint *jarr)
%{  if ($input && jenv->GetArrayLength($input) != $1_size) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "incorrect array size");
    return $null;
  }
  if (!SWIG_JavaArrayInInt(jenv, &jarr, (int **)&$1, $input)) return $null; 
%}

%typemap(argout) int[ANY], int[] 
%{ SWIG_JavaArrayArgoutInt(jenv, jarr$argnum, (int *)$1, $input); %}

%typemap(out) int[ANY]
%{$result = SWIG_JavaArrayOutInt(jenv, (int *)$1, $1_dim0); %}

%typemap(out) int[] 
%{$result = SWIG_JavaArrayOutInt(jenv, (int *)$1, FillMeInAsSizeCannotBeDeterminedAutomatically); %}

/* %typemap(freearg) int[ANY], int[] 
 * %{ delete [] $1; %} */

%typemap(javain) int[ANY], int[] "$javainput"

%typemap(javaout) int[ANY], int[] {
    return $jnicall;
  }

%typemap(memberin) int[ANY], int[];
%typemap(globalin) int[ANY], int[];
/*@SWIG@*/         /* int[ANY] */
/*@SWIG:arrays_java.i,143,JAVA_ARRAYS_TYPEMAPS@*/


%typemap(typecheck, precedence=      1080    )  /* Java int[] */
    int[ANY], int[]
    ""


