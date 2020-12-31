package com.mapzen.tangram;

class NativeMapData {

    NativeMapData(final long pointer) {
        nativePointer = pointer;
        if (nativePointer == 0) {
            throw new RuntimeException("NativeMapData received a null pointer");
        }
    }

    void remove(NativeMap nativeMap) {
        nativeMap.removeClientDataSource(nativePointer);
    }

    native void addClientDataFeature(double[] coordinates, int[] rings, String[] properties);
    native void addClientDataGeoJson(String geoJson);
    native void generateClientDataTiles();
    native void clearClientDataFeatures();

    private final long nativePointer;
}
