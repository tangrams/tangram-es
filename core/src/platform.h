#pragma once

#include <string>
#include <vector>
#include <functional>

#include "tileID.h"

#ifdef PLATFORM_ANDROID

struct _JNIEnv;
typedef _JNIEnv JNIEnv;
class _jobject;
typedef _jobject* jobject;
class _jbyteArray;
typedef _jbyteArray* jbyteArray;

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager);
void networkDataBridge(JNIEnv* jniEnv, jbyteArray jFetchedBytes, int tileIDx, int tileIDy, int tileIDz, int dataSourceID);
#endif


#if (defined PLATFORM_IOS) && (defined __OBJC__)
#import "ViewController.h"
void setViewController(ViewController* _controller);
void networkDataBridge(std::vector<char>& _rawData, TileID _tileID, int _dataSource);
#endif

#ifdef PLATFORM_OSX
void NSurlInit();
#endif

#if (defined PLATFORM_LINUX) || (defined PLATFORM_RPI)
#include "netWorkerData.h"
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

bool startNetworkRequest(const std::string& _url, const TileID& _tileID, const int _dataSourceID);

void cancelNetworkRequest(const std::string& _url);

void setNetworkRequestCallback(std::function<void(std::vector<char>&&, TileID, int)>&& _callback);

