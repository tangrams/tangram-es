#pragma once

#include <string>
#include <vector>
#include <functional>

#ifdef PLATFORM_ANDROID

struct _JNIEnv;
typedef _JNIEnv JNIEnv;
class _jobject;
typedef _jobject* jobject;
class _jbyteArray;
typedef _jbyteArray* jbyteArray;
typedef long long jlong;

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager);
void onUrlSuccess(JNIEnv* jniEnv, jbyteArray jFetchedBytes, jlong jCallbackPtr);
void onUrlFailure(JNIEnv* jniEnv, jlong jCallbackPtr);

// FIXME - this just here for a borken NDK
#include <sstream>
namespace std {
template <typename T>
std::string to_string(T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}
}

#endif


#if (defined PLATFORM_IOS) && (defined __OBJC__)
#import "ViewController.h"
void init(ViewController* _controller);
#endif

#ifdef PLATFORM_OSX
void NSurlInit();
#endif

#if (defined PLATFORM_LINUX) || (defined PLATFORM_RPI)
void processNetworkQueue();
#endif

/* Print a formatted message to the console
 *
 * Uses printf syntax to write a string to stderr (or logcat, on Android)
 */
void logMsg(const char* fmt, ...);

/* Request that a new frame be rendered by the windowing system
 */
void requestRender();

/* If called with 'true', the windowing system will re-draw frames continuously;
 * otherwise new frames will only be drawn when 'requestRender' is called. 
 */
void setContinuousRendering(bool _isContinuous);

bool isContinuousRendering();

/* Read a bundled resource file as a string
 * 
 * Opens the file at the given relative path and returns a string with its contents.
 * _path is the location of the file within the core/resources folder. If the file
 * cannot be found or read, the returned string is empty. 
 */
std::string stringFromResource(const char* _path);

/* Read and allocates size bytes of memory  
 *
 * Similarly to stringFromResource, _path is the location of file within 
 * core/resources. If the file cannot be read nothing is allocated and 
 * a nullptr is returned.
 * _size is is an in/out parameter to retrieve the size in bytes of the 
 * allocated file
 */ 
unsigned char* bytesFromResource(const char* _path, unsigned int* _size);

/* Function type for receiving data from a successful network request */
using UrlCallback = std::function<void(std::vector<char>&&)>;

/* Start retrieving data from a URL asynchronously
 * 
 * When the request is finished, the callback @_callback will be
 * run with the data that was retrieved from the URL @_url
 */
bool startUrlRequest(const std::string& _url, UrlCallback _callback);

/* Stop retrieving data from a URL that was previously requested
 */
void cancelUrlRequest(const std::string& _url);
