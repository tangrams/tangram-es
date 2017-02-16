//
//  TGFontConverter.mm
//  TangramMap
//
//  Created by Karim Naaji on 11/01/16.
//  Copyright (c) 2017 Mapzen. All rights reserved.
//

#import "TGFontConverter.h"

#import <cstdio>

struct FontHeader {
    int32_t version;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
};

typedef struct FontHeader FontHeader;

struct TableEntry {
    uint32_t tag;
    uint32_t checkSum;
    uint32_t offset;
    uint32_t length;
};

typedef struct TableEntry TableEntry;

static uint32_t calcTableCheckSum(const uint32_t* table, uint32_t numberOfBytesInTable)
{
    uint32_t sum = 0;
    uint32_t nLongs = (numberOfBytesInTable + 3) / 4;
    while (nLongs-- > 0) {
       sum += CFSwapInt32HostToBig(*table++);
    }
    return sum;
}

static uint32_t calcTableDataRefCheckSum(CFDataRef dataRef)
{
    const uint32_t* dataBuff = (const uint32_t *)CFDataGetBytePtr(dataRef);
    uint32_t dataLength = (uint32_t)CFDataGetLength(dataRef);
    return calcTableCheckSum(dataBuff, dataLength);
}

@implementation TGFontConverter

// References:
// https://skia.googlesource.com/skia/+/master/src/ports/SkFontHost_mac.cpp
// https://gist.github.com/Jyczeal/1892760

+ (std::vector<char>)fontDataForCGFont:(CGFontRef)cgFont
{
    if (!cgFont) {
        return {};
    }

    CFRetain(cgFont);

    CFArrayRef tags = CGFontCopyTableTags(cgFont);
    int tableCount = CFArrayGetCount(tags);

    std::vector<size_t> tableSizes;
    tableSizes.resize(tableCount);
    std::vector<CFDataRef> dataRefs;
    dataRefs.resize(tableCount);

    BOOL containsCFFTable = NO;

    size_t totalSize = sizeof(FontHeader) + sizeof(TableEntry) * tableCount;

    for (int index = 0; index < tableCount; ++index) {
        size_t tableSize = 0;
        intptr_t aTag = (intptr_t)CFArrayGetValueAtIndex(tags, index);

        if (aTag == 'CFF ' && !containsCFFTable) {
            containsCFFTable = YES;
        }

        dataRefs[index] = CGFontCopyTableForTag(cgFont, aTag);

        if (dataRefs[index] != NULL) {
            tableSize = CFDataGetLength(dataRefs[index]);
        }

        totalSize += (tableSize + 3) & ~3;

        tableSizes[index] = tableSize;
    }

    std::vector<char> data;
    data.resize(totalSize);
    unsigned char* stream = reinterpret_cast<unsigned char*>(data.data());

    char* dataStart = (char*)stream;
    char* dataPtr = dataStart;

    // Write font header (also called sfnt header, offset subtable)
    FontHeader* offsetTable = (FontHeader*)dataPtr;

    // Compute font header entries
    // c.f: Organization of an OpenType Font in:
    // https://www.microsoft.com/typography/otspec/otff.htm
    {
        // (Maximum power of 2 <= numTables) x 16
        uint16_t entrySelector = 0;
        // Log2(maximum power of 2 <= numTables).
        uint16_t searchRange = 1;

        while (searchRange < tableCount >> 1) {
            entrySelector++;
            searchRange <<= 1;
        }
        searchRange <<= 4;

        // NumTables x 16-searchRange.
        uint16_t rangeShift = (tableCount << 4) - searchRange;

        // OpenType Font contains CFF Table use 'OTTO' as version, and with .otf extension
        // otherwise 0001 0000
        offsetTable->version = containsCFFTable ? 'OTTO' : CFSwapInt16HostToBig(1);
        offsetTable->numTables = CFSwapInt16HostToBig((uint16_t)tableCount);
        offsetTable->searchRange = CFSwapInt16HostToBig((uint16_t)searchRange);
        offsetTable->entrySelector = CFSwapInt16HostToBig((uint16_t)entrySelector);
        offsetTable->rangeShift = CFSwapInt16HostToBig((uint16_t)rangeShift);
    }

    dataPtr += sizeof(FontHeader);

    // Write tables
    TableEntry* entry = (TableEntry*)dataPtr;
    dataPtr += sizeof(TableEntry) * tableCount;

    for (int index = 0; index < tableCount; ++index) {

        intptr_t aTag = (intptr_t)CFArrayGetValueAtIndex(tags, index);
        CFDataRef tableDataRef = dataRefs[index];

        if (tableDataRef == NULL) { continue; }

        size_t tableSize = CFDataGetLength(tableDataRef);

        memcpy(dataPtr, CFDataGetBytePtr(tableDataRef), tableSize);

        entry->tag = CFSwapInt32HostToBig((uint32_t)aTag);
        entry->checkSum = CFSwapInt32HostToBig(calcTableCheckSum((uint32_t *)dataPtr, tableSize));

        uint32_t offset = dataPtr - dataStart;
        entry->offset = CFSwapInt32HostToBig((uint32_t)offset);
        entry->length = CFSwapInt32HostToBig((uint32_t)tableSize);
        dataPtr += (tableSize + 3) & ~3;
        ++entry;
        CFRelease(tableDataRef);
    }

    CFRelease(cgFont);

    return data;
}

@end
