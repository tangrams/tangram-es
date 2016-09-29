make linux RELEASE=1
cp -v build/linux/lib/libtangram-core.so $BATOSDIR/lib/
cp -v build/linux/lib/libtangram-core.so $BATOSDIR/bin/
cp -v core/api/tangram-*.h $BATOSDIR/include
