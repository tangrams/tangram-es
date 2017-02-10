#include "util/zlibHelper.h"

#include <zlib.h>

#include <assert.h>

#define CHUNK 16384

namespace Tangram {
namespace zlib {

int inflate(const char* _data, size_t _size, std::vector<char>& dst) {

    int ret;
    unsigned char out[CHUNK];

    z_stream strm;
    memset(&strm, 0, sizeof(z_stream));

    ret = inflateInit2(&strm, 16+MAX_WBITS);
    if (ret != Z_OK) { return ret; }

    strm.avail_in = _size;
    strm.next_in = (Bytef*)_data;

    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;

        ret = inflate(&strm, Z_NO_FLUSH);

         /* state not clobbered */
        assert(ret != Z_STREAM_ERROR);

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
            /* fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            return ret;
        }

        size_t have = CHUNK - strm.avail_out;
        dst.insert(dst.end(), out, out+have);

    } while (ret == Z_OK);

    inflateEnd(&strm);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

}
}
