#include <string>
#include <vector>
#include <curl/curl.h>
#include "log.h"

namespace {
class urlGetCurlInitDestory{
public:
  urlGetCurlInitDestory() {
    // Initialize cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
  }
  ~urlGetCurlInitDestory() {
    curl_global_cleanup();
  }
};
urlGetCurlInitDestory urlGetCurlInitDestoryGlobal;
}

static size_t write_data(void *_buffer, size_t _size, size_t _nmemb, void *_dataPtr) {
  const size_t realSize = _size * _nmemb;
  std::vector<char>* stream = (std::vector<char>*)_dataPtr;
  size_t oldSize = stream->size();
  stream->resize(oldSize + realSize);
  memcpy(stream->data() + oldSize, _buffer, realSize);
  return realSize;
}

bool urlGet(const std::string &url, std::vector<char> &stream) {
  CURL* curlHandle = curl_easy_init();;
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &stream);
  curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0L);
  curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0L);
  curl_easy_setopt(curlHandle, CURLOPT_ENCODING, "gzip");
  CURLcode result = curl_easy_perform(curlHandle);

  long httpStatusCode = 0;
  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpStatusCode);
  curl_easy_cleanup(curlHandle);

  if (result == CURLE_OK && httpStatusCode == 200) {
    return true;
  }
  LOGE("curl_easy_perform failed: %s - %d",
       curl_easy_strerror(result), httpStatusCode);
  stream.resize(0);
  return false;
}
