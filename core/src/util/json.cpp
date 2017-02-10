#include "util/json.h"

#include "rapidjson/encodedstream.h"
#include "rapidjson/error/en.h"
#include "rapidjson/memorystream.h"

namespace Tangram {

    JsonDocument JsonParseBytes(const char* _bytes, size_t _length, const char** _error, size_t* _errorOffset) {

        JsonDocument document;
        rapidjson::MemoryStream mstream(_bytes, _length);
        rapidjson::EncodedInputStream<rapidjson::UTF8<char>, rapidjson::MemoryStream> istream(mstream);
        document.ParseStream(istream);

        *_error = nullptr;
        *_errorOffset = 0;
        if (document.HasParseError()) {
            *_error = rapidjson::GetParseError_En(document.GetParseError());
            *_errorOffset = document.GetErrorOffset();
        }

        return document;

    }

}
