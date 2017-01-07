#include "urlClient.h"
#include "log.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <codecvt>

#include <zlib.h>
#include <Windows.h>
#include <Winhttp.h>
#pragma comment (lib, "Winhttp.lib")

namespace Tangram {

struct gzFile {
  z_stream strm;
  int ret;
};

enum UrlGetResult {
  URL_GET_OK = 0,
  URL_GET_ERROR = 1,
  URL_GET_ABORTED = 2,
};

/* Map the Windows error number in ERROR to a locale-dependent error
message string and return a pointer to it.  Typically, the values
for ERROR come from GetLastError.

The string pointed to shall not be modified by the application,
but may be overwritten by a subsequent call to strwinerror

The strwinerror function does not change the current setting
of GetLastError.
*/

static std::string strwinerror(DWORD error)
{
  char buf[1024];

  wchar_t *msgbuf;
  DWORD lasterr = GetLastError();
  DWORD chars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
    | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    NULL,
    error,
    0, /* Default language */
    (LPWSTR)&msgbuf,
    0,
    NULL);
  if (chars != 0) {
    /* If there is an \r\n appended, zap it.  */
    if (chars >= 2
      && msgbuf[chars - 2] == '\r' && msgbuf[chars - 1] == '\n') {
      chars -= 2;
      msgbuf[chars] = 0;
    }

    if (chars > sizeof(buf) - 1) {
      chars = sizeof(buf) - 1;
      msgbuf[chars] = 0;
    }

    wcstombs(buf, msgbuf, chars + 1);
    LocalFree(msgbuf);
  } else {
    sprintf(buf, "unknown win32 error (%ld)", error);
  }

  SetLastError(lasterr);
  return buf;
}

void gzread(gzFile &gz, unsigned char* in, uInt len, std::vector<char> &out_stream)
{
  if (len == 0) {
    return;
  }
  z_stream *strm = &gz.strm;
  strm->next_in = in;
  strm->avail_in = len;
  do {
    if (strm->avail_out == 0) {
      strm->avail_out = strm->total_out;
      out_stream.resize(strm->total_out << 2);
    }
    strm->next_out = (Bytef *)(out_stream.data() + strm->total_out);
    gz.ret = inflate(strm, Z_NO_FLUSH);
  } while (gz.ret == Z_OK && strm->avail_in > 0);
}

bool ClientHttpQueryHeader(HINTERNET hRequest, int headerCode, std::wstring &result) {
  result.resize(8192);
  DWORD length = result.size() * 2;
  bool succeed = WinHttpQueryHeaders(hRequest, headerCode, 0, (LPVOID)result.data(), &length, 0) != FALSE;
  if (succeed) {
    result.resize(length / 2);
  } else {
    result.resize(0);
  }
  return succeed;
}

static UrlGetResult urlGet(UrlClient::Task& task, std::string &errorMessage, int &httpStatus) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

  URL_COMPONENTS uc;
  wchar_t Scheme[256];
  wchar_t HostName[256];
  wchar_t UserName[256];
  wchar_t Password[256];
  wchar_t UrlPath[1024];
  wchar_t ExtraInfo[256];
  std::vector<char> &stream = task.response.data;
  uc.dwStructSize = sizeof(uc);
  uc.lpszScheme = Scheme;
  uc.lpszHostName = HostName;
  uc.lpszUserName = UserName;
  uc.lpszPassword = Password;
  uc.lpszUrlPath = UrlPath;
  uc.lpszExtraInfo = ExtraInfo;

  uc.dwSchemeLength = _countof(Scheme);
  uc.dwHostNameLength = _countof(HostName);
  uc.dwUserNameLength = _countof(UserName);
  uc.dwPasswordLength = _countof(Password);
  uc.dwUrlPathLength = _countof(UrlPath);
  uc.dwExtraInfoLength = _countof(ExtraInfo);
  std::wstring wurl = converter.from_bytes(task.request.url);
  if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), ICU_ESCAPE, &uc)) {
    return URL_GET_ERROR;
  }
  DWORD flags = 0;
  switch (uc.nScheme) {
  case INTERNET_SCHEME_FTP:
    break;
  case INTERNET_SCHEME_HTTP:
    break;
  case INTERNET_SCHEME_HTTPS:
    flags |= WINHTTP_FLAG_SECURE;
    break;
  default:
    return URL_GET_ERROR;
  }

  HINTERNET session = WinHttpOpen(L"Tangram UrlWorker", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (session == NULL) {
    return URL_GET_ERROR;
  }

  HINTERNET hConnect = WinHttpConnect(session, uc.lpszHostName, uc.nPort, 0);
  if (hConnect == NULL) {
    WinHttpCloseHandle(session);
    return URL_GET_ERROR;
  }
  std::wstring ObjectName = uc.lpszUrlPath;
  std::wstring strExtraInfo = uc.lpszExtraInfo;
  if (!strExtraInfo.empty()) {
    ObjectName = ObjectName + strExtraInfo;
  }

  HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", ObjectName.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (hRequest == NULL) {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(session);
    return URL_GET_ERROR;
  }

  // send request
  bool noError = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != 0;
  bool urlGetAborted = false;
  noError = noError && (WinHttpReceiveResponse(hRequest, 0) != 0);
  std::wstring encoding;
  noError = noError && ClientHttpQueryHeader(hRequest, WINHTTP_QUERY_CONTENT_ENCODING, encoding);
  bool zipError = false;
  if (noError) {
    DWORD dwSize;
    bool isGZIP = std::wstring(encoding) == L"gzip";
    gzFile gz;
    if (isGZIP) {
      gz.strm.zalloc = Z_NULL;
      gz.strm.zfree = Z_NULL;
      gz.strm.opaque = Z_NULL;
      gz.strm.next_in = Z_NULL;
      gz.strm.avail_in = 0;
      gz.ret = inflateInit2(&gz.strm, 47);
      stream.resize(8);
      gz.strm.next_out = Z_NULL;
      gz.strm.avail_out = stream.size();
    }
    unsigned char buffer[1024];
    do
    {
      if (task.response.canceled) {
        urlGetAborted = true;
        break;
      }
      dwSize = sizeof(buffer);

      if (WinHttpReadData(hRequest, (LPVOID)buffer,
        dwSize, &dwSize)) {
        if (isGZIP) {
          gzread(gz, buffer, dwSize, stream);
          if (gz.ret == Z_STREAM_END) {
            stream.resize(gz.strm.total_out);
            break;
          } else if (gz.ret != Z_OK) {
            zipError = true;
            errorMessage = gz.strm.msg;
            break;
          }
        } else {
          size_t oldSize = stream.size();
          stream.resize(oldSize + dwSize);
          memcpy(stream.data() + oldSize, buffer, dwSize);
        }
      } else {
        noError = false;
      }
      // Free the memory allocated to the buffer.
    } while (noError && dwSize > 0);
    if (isGZIP) {
      gz.ret = inflateEnd(&gz.strm);
    }
  }
  if (noError) {
    std::wstring statusCode;
    noError = noError && ClientHttpQueryHeader(hRequest, WINHTTP_QUERY_STATUS_CODE, statusCode);
    if (noError) {
      swscanf(statusCode.data(), L"%d", &httpStatus);
    }
  }

  // close request
  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(session);
  if (noError) {
    if (urlGetAborted) {
      return URL_GET_ABORTED;
    }
    return URL_GET_OK;
  }
  if (!zipError) {
    errorMessage = strwinerror(GetLastError());
  }
  return URL_GET_ERROR;
}

void UrlClient::fetchLoop(uint32_t index) {
    assert(m_tasks.size() > index);
    Task& task = m_tasks[index];
    LOGD("winHttpLoop %u starting", index);
    // Loop until the session is destroyed.
    std::string httpErrorString;
    while (m_keepRunning) {
        bool haveRequest = false;
        // Wait until the condition variable is notified.
        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            if (m_requests.empty()) {
                LOGD("winHttpLoop %u waiting", index);
                m_requestCondition.wait(lock);
            }
            LOGD("winHttpLoop %u notified", index);
            // Try to get a request from the list.
            if (!m_requests.empty()) {
                // Take the first request from our list.
                task.request = m_requests.front();
                m_requests.erase(m_requests.begin());
                haveRequest = true;
            }
        }
        if (haveRequest) {
            // Configure the easy handle.
            const char* url = task.request.url.data();
            LOGD("winHttpLoop %u starting request for url: %s", index, url);
            int httpStatus = 0;
            UrlGetResult result = urlGet(task, httpErrorString, httpStatus);
            // Handle success or error.
            if (result == URL_GET_OK && httpStatus >= 200 && httpStatus < 300) {
                LOGD("winHttpLoop %u succeeded with http status: %d for url: %s", index, httpStatus, url);
                task.response.successful = true;
            } else if (result == URL_GET_ABORTED) {
                LOGD("winHttpLoop %u request aborted for url: %s", index, url);
                task.response.successful = false;
            } else {
                LOGE("winHttpLoop %u failed: '%s' with http status: %d for url: %s", index, httpErrorString, httpStatus, url);
                task.response.successful = false;
            }
            if (!task.response.successful) {
              task.response.data.clear();
            }
            if (task.request.callback) {
                LOGD("winHttpLoop %u performing request callback", index);
                task.request.callback(std::move(task.response.data));
            }
        }
        // Reset the response.
        task.response.data.clear();
        task.response.canceled = false;
        task.response.successful = false;
    }
    LOGD("winHttpLoop %u exiting", index);
}

} // namespace Tangram
