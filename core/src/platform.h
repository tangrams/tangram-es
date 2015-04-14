#pragma once

#include <string>
#include <sstream>
#include <memory>
#include <functional>

#include "tileID.h"

#ifdef PLATFORM_ANDROID
struct _JNIEnv;
typedef _JNIEnv JNIEnv;
class _jobject;
typedef _jobject* jobject;
void cacheJniEnv(JNIEnv* _jniEnv);
void cacheTangramInstance(jobject _tangramInstance);
void setAssetManager(jobject _assetManager);
void networkDataBridge(std::string _rawData, int _tileIDx, int _tileIDy, int _tileIDz, int _dataSourceID);
#endif

/* Print a formatted message to the console
 *
 * Uses printf syntax to write a string to stderr (or logcat, on Android)
 */
void logMsg(const char* fmt, ...);

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

bool streamFromHttpASync(const std::string& _url, const TileID& _tileID, const int _dataSourceID);

void cancelNetworkRequest(const std::string& _url);

void setNetworkRequestCallback(std::function<void(std::string, TileID, int)>&& _callback);

