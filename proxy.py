# Originally from http://sharebear.co.uk/blog/2009/09/17/very-simple-python-caching-proxy/
#
# Usage:
# A call to http://localhost:8000/example.com/foo.html will cache the file
# at http://example.com/foo.html on disc and not redownload it again.
# To clear the cache simply do a `rm *.cached`. To stop the server simply
# send SIGINT (Ctrl-C). It does not handle any headers or post data.
#
# see also: https://pymotw.com/2/BaseHTTPServer/

import BaseHTTPServer
# import hashlib
import os
import urllib2

from BaseHTTPServer import HTTPServer
from SocketServer import ThreadingMixIn
import tempfile

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    pass

class CacheHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def ok(self):
        self.send_response(200)
        self.send_header("Content-Encoding", "gzip")
        self.end_headers()

    def do_GET(self):

        dirname = ".tiles/" + os.path.dirname(self.path)[1:]
        filename = os.path.basename(self.path)

        while not os.path.exists(dirname):
            # might be a race here
            try:
                os.makedirs(dirname)
            except:
                None

        cache_filename = dirname + "/" + filename

        if os.path.exists(cache_filename):
            data = open(cache_filename, mode='rb').readlines()
            self.ok()
            self.wfile.writelines(data)
            return

        print "fetching: %s" % (cache_filename)
        data = urllib2.urlopen("http:/" + self.path, timeout=10).readlines()
        self.ok()
        self.wfile.writelines(data)

        f = tempfile.NamedTemporaryFile(dir=os.path.dirname(cache_filename),
                                        mode='wb',
                                        delete=False)
        f.writelines(data)
        f.close()
        os.rename(f.name, cache_filename)

def run():
    if not os.path.exists(".tiles"):
        os.makedirs(".tiles")

    server_address = ('', 8000)
    httpd = ThreadedHTTPServer(server_address, CacheHandler)
    httpd.serve_forever()

if __name__ == '__main__':
    run()
