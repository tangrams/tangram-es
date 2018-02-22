package com.mapzen.tangram;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileNotFoundException;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.ArrayList;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.os.Build;
import android.util.ArrayMap;
import android.util.SparseArray;
import android.util.Xml;
import android.support.annotation.NonNull;

class FontFileParser {

    private Map<String, String> fontDict;
    private SparseArray<List<String>> fallbackFontDict;

    private static final String systemFontPath = "/system/fonts/";
    // Android version >= 5.0
    private static final String fontXMLPath = "/system/etc/fonts.xml";

    // Android version < 5.0
    private static final String oldFontXMLPath = "/system/etc/system_fonts.xml";
    private static final String oldFontXMLFallbackPath = "/system/etc/fallback_fonts.xml";

    public FontFileParser() {
        if (Build.VERSION.SDK_INT > 18) {
            fontDict = new ArrayMap<>();
        }
        else {
            fontDict = new HashMap<>();
        }
        fallbackFontDict = new SparseArray<>();
    }

    private void addFallback(final Integer weight, final String filename) {
        final String fullFileName = systemFontPath + filename;
        if (!new File(fullFileName).exists()) {
            return;
        }

        if (fallbackFontDict.indexOfKey(weight) < 0) {
            fallbackFontDict.put(weight, new ArrayList<String>());
        }

        fallbackFontDict.get(weight).add(fullFileName);
    }


    private void processDocumentPreLollipop(@NonNull final XmlPullParser parser) throws XmlPullParserException, IOException {
        parser.nextTag();
        parser.require(XmlPullParser.START_TAG, null, "familyset");

        final List<String> namesets = new ArrayList<>();
        final List<String> filesets = new ArrayList<>();

        while (parser.next() != XmlPullParser.END_DOCUMENT) {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }

            if (!"family".equals(parser.getName())) {
                skip(parser);
                continue;
            }

            namesets.clear();
            filesets.clear();

            while (parser.next() != XmlPullParser.END_TAG) {
                if (parser.getEventType() != XmlPullParser.START_TAG) {
                    continue;
                }

                if ("nameset".equals(parser.getName())) {
                    while (parser.next() != XmlPullParser.END_TAG) {
                        if (parser.getEventType() != XmlPullParser.START_TAG) {
                            continue;
                        }

                        final String name = parser.nextText();
                        namesets.add(name.toLowerCase());
                    }
                    continue;
                }

                if ("fileset".equals(parser.getName())) {
                    while (parser.next() != XmlPullParser.END_TAG) {
                        if (parser.getEventType() != XmlPullParser.START_TAG) {
                            continue;
                        }
                        final String filename = parser.nextText();
                        // Don't use UI fonts
                        if (filename.contains("UI-")) {
                            continue;
                        }
                        // Sorry - not yet supported
                        if (filename.contains("Emoji")) {
                            continue;
                        }
                        filesets.add(filename);
                    }
                } else {
                    skip(parser);
                }

                // fallback_fonts.xml entries have no names
                if (namesets.isEmpty()) { namesets.add("sans-serif"); }

                for (final String filename : filesets) {
                    for (final String fontname : namesets) {

                        String style = "normal";
                        // The file structure in `/etc/system_fonts.xml` is quite undescriptive
                        // which makes it hard to make a matching from a font style to a font file
                        // e.g. italic -> font file, instead we extract this information from the
                        // font file name itself
                        final String[] fileSplit = filename.split("-");
                        if (fileSplit.length > 1) {
                            style = fileSplit[fileSplit.length - 1].toLowerCase();
                            // Remove extension .ttf
                            style = style.substring(0, style.lastIndexOf('.'));

                            if ("regular".equals(style)) {
                                style = "normal";
                            }
                        }

                        // Same here, font boldness is non-available for android < 5.0 file
                        // description, we default to integer boldness of 400 by default
                        fontDict.put(fontname + "_400_" + style, systemFontPath + filename);

                        if ("sans-serif".equals(fontname) && "normal".equals(style)) {
                            addFallback(400, filename);
                        }
                    }
                }
            }
        }
    }

    private void processDocument(@NonNull final XmlPullParser parser) throws XmlPullParserException, IOException {
        final List<String> familyWeights = new ArrayList<>();

        parser.nextTag();
        // Parse Families
        parser.require(XmlPullParser.START_TAG, null, "familyset");
        while (parser.next() != XmlPullParser.END_DOCUMENT) {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }
            if ("family".equals(parser.getName())) {
                familyWeights.clear();
                // Parse this family:
                final String name = parser.getAttributeValue(null, "name");
                //Unused : final String lang = parser.getAttributeValue(null, "lang");

                // fallback fonts
                if (name == null) {
                    while (parser.next() != XmlPullParser.END_TAG) {
                        if (parser.getEventType() != XmlPullParser.START_TAG) {
                            continue;
                        }
                        final String tag = parser.getName();
                        if ("font".equals(tag)) {
                            String weightStr = parser.getAttributeValue(null, "weight");
                            if (weightStr == null) {
                                weightStr = "400";
                            } else {
                                familyWeights.add(weightStr);
                            }

                            final String filename = parser.nextText();

                            // Don't use UI fonts
                            if (filename.contains("UI-")) {
                                continue;
                            }
                            // Sorry - not yet supported
                            if (filename.contains("Emoji")) {
                                continue;
                            }

                            addFallback(Integer.valueOf(weightStr), filename);
                        } else {
                            skip(parser);
                        }
                    }

                } else {
                    while (parser.next() != XmlPullParser.END_TAG) {
                        if (parser.getEventType() != XmlPullParser.START_TAG) {
                            continue;
                        }
                        final String tag = parser.getName();
                        if ("font".equals(tag)) {
                            String weightStr = parser.getAttributeValue(null, "weight");
                            if (weightStr == null) {
                                weightStr = "400";
                            } else {
                                familyWeights.add(weightStr);
                            }

                            String styleStr = parser.getAttributeValue(null, "style");
                            if (styleStr == null) {
                                styleStr = "normal";
                            }

                            final String filename = parser.nextText();
                            final String key = name + "_" + weightStr + "_" + styleStr;
                            fontDict.put(key, systemFontPath + filename);

                            if ("sans-serif".equals(name) && "normal".equals(styleStr)) {
                                addFallback(Integer.valueOf(weightStr), filename);
                            }

                        } else {
                            skip(parser);
                        }
                    }
                }
            } else if ("alias".equals(parser.getName())) {
                // Parse this alias to font to fileName
                final String aliasName = parser.getAttributeValue(null, "name");
                final String toName = parser.getAttributeValue(null, "to");
                final String weightStr = parser.getAttributeValue(null, "weight");
                final List<String> aliasWeights;
                String fontFilename;

                if (weightStr == null) {
                    aliasWeights = familyWeights;
                } else {
                    aliasWeights = Collections.singletonList(weightStr);
                }

                for (final String weight : aliasWeights) {
                    // Only 2 styles possible based on /etc/fonts.xml
                    // Normal style
                    fontFilename = fontDict.get(toName + "_" + weight + "_normal");
                    fontDict.put(aliasName + "_" + weight + "_normal", fontFilename);
                    // Italic style
                    fontFilename = fontDict.get(toName + "_" + weight + "_italic");
                    fontDict.put(aliasName + "_" + weight + "_italic", fontFilename);
                }
            } else {
                skip(parser);
            }
        }
    }

    private void skip(@NonNull final XmlPullParser parser) throws XmlPullParserException, IOException {
        int depth = 1;
        while (depth > 0) {
            switch (parser.next()) {
                case XmlPullParser.START_TAG:
                    depth++;
                    break;
                case XmlPullParser.END_TAG:
                    depth--;
                    break;
            }
        }
    }

    public void parse() {
        File fontFile = new File(fontXMLPath);

        if (fontFile.exists()) {
            parse(fontFile.getAbsolutePath(), false);
            return;
        }

        fontFile = new File(oldFontXMLPath);
        if (fontFile.exists()) {
            parse(fontFile.getAbsolutePath(), true);
        }
        fontFile = new File(oldFontXMLFallbackPath);
        if (fontFile.exists()) {
            parse(fontFile.getAbsolutePath(), true);
        }
    }

    private void parse(final String fileXml, final boolean oldXML) {
        InputStream in;

        try {
            in = new FileInputStream(fileXml);
        } catch (final FileNotFoundException e) {
            e.printStackTrace();
            return;
        }

        final XmlPullParser parser = Xml.newPullParser();

        try {
            parser.setInput(in, null);

            if (oldXML) {
                processDocumentPreLollipop(parser);
            } else {
                processDocument(parser);
            }
        } catch(final XmlPullParserException e) {
            e.printStackTrace();
        } catch(final IOException e) {
            e.printStackTrace();
        }

        try {
            in.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public String getFontFile(final String _key) {
        return fontDict.containsKey(_key) ? fontDict.get(_key) : "";
    }

    /*
     * Returns the next available font fallback, or empty string if not found
     * The integer value determines the fallback priority (lower is higher)
     * The weightHint value determines the closest fallback hint for boldness
     * See /etc/fonts/font_fallback for documentation
     */
    public String getFontFallback(final int importance, final int weightHint) {
        int diffWeight = Integer.MAX_VALUE;
        String fallback = "";

        for (int i = 0; i < fallbackFontDict.size(); ++i) {
            int diff = Math.abs(fallbackFontDict.keyAt(i) - weightHint);

            if (diff < diffWeight) {
                final List<String> fallbacks = fallbackFontDict.valueAt(i);

                if (importance < fallbacks.size()) {
                    fallback = fallbacks.get(importance);
                    diffWeight = diff;
                }
            }
        }

        return fallback;
    }

}

