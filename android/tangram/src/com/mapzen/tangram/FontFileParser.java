package com.mapzen.tangram;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.ArrayList;
import java.util.Vector;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.util.Xml;

class FontFileParser {

    private Map<String, String> fontDict = new HashMap<String, String>();
    private Map<Integer, Vector<String>> fallbackFontDict = new HashMap<Integer, Vector<String>>();

    private static String systemFontPath = "/system/fonts/";
    // Android version >= 5.0
    private static String fontXMLPath = "/system/etc/fonts.xml";
    // Android version < 5.0
    private static String fontXMLFallbackPath = "/etc/system_fonts.xml";

    private void processDocumentFallback(XmlPullParser parser) throws XmlPullParserException, IOException {
        parser.nextTag();
        parser.require(XmlPullParser.START_TAG, null, "familyset");

        ArrayList<String> namesets = new ArrayList<>();
        ArrayList<String> filesets = new ArrayList<>();

        while (parser.next() != XmlPullParser.END_DOCUMENT) {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }

            if ("family".equals(parser.getName())) {
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

                            String name = parser.nextText();
                            namesets.add(name.toLowerCase());
                        }
                        continue;
                    }

                    if ("fileset".equals(parser.getName())) {
                        while (parser.next() != XmlPullParser.END_TAG) {
                            if (parser.getEventType() != XmlPullParser.START_TAG) {
                                continue;
                            }

                            String file = parser.nextText();
                            filesets.add(file);
                        }
                    } else {
                       skip(parser);
                    }

                    for (String file : filesets) {
                        for (String name : namesets) {
                            String fullFilename = systemFontPath + file;
                            String style = "normal";

                            // The file structure in `/etc/system_fonts.xml` is quite undescriptive
                            // which makes it hard to make a matching from a font style to a font file
                            // e.g. italic -> font file, instead we extract this information from the
                            // font file name itself
                            String[] fileSplit = file.split("-");
                            if (fileSplit.length > 1) {
                                style = fileSplit[fileSplit.length - 1].toLowerCase();
                                // Remove extension .ttf
                                style = style.substring(0, style.lastIndexOf('.'));

                                if (style.equals("regular")) {
                                    style = "normal";
                                }
                            }

                            // Same here, font boldness is non-available for android < 5.0 file
                            // description, we default to integer boldness of 400 by default
                            String key = name + "_400_" + style;
                            fontDict.put(key, fullFilename);
                        }
                    }
                }
            } else {
                skip(parser);
            }
        }
    }

    private void processDocument(XmlPullParser parser) throws XmlPullParserException, IOException {

        ArrayList<String> familyWeights = new ArrayList<String>();

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
                String name = parser.getAttributeValue(null, "name");

                // fallback fonts
                if (name == null) {
                    while (parser.next() != XmlPullParser.END_TAG) {
                        if (parser.getEventType() != XmlPullParser.START_TAG) {
                            continue;
                        }
                        String tag = parser.getName();
                        if ("font".equals(tag)) {
                            String weightStr = parser.getAttributeValue(null, "weight");
                            if (weightStr != null) { familyWeights.add(weightStr); }
                            weightStr = (weightStr == null) ? "400" : weightStr;

                            String filename = parser.nextText();

                            Integer weight = new Integer(weightStr);
                            if (!fallbackFontDict.containsKey(weight)) {
                                fallbackFontDict.put(weight, new Vector<String>());
                            }

                            // Don't use UI fonts
                            if (filename.indexOf("UI-") >= 0) {
                                continue;
                            }

                            String fullFileName = systemFontPath + filename;

                            fallbackFontDict.get(weight).add(fullFileName);
                        } else {
                            skip(parser);
                        }
                    }
                    continue;
                }
                while (parser.next() != XmlPullParser.END_TAG) {
                    if (parser.getEventType() != XmlPullParser.START_TAG) {
                        continue;
                    }
                    String tag = parser.getName();
                    if ("font".equals(tag)) {
                        String weightStr = parser.getAttributeValue(null, "weight");
                        if (weightStr != null) { familyWeights.add(weightStr); }
                        weightStr = (weightStr == null) ? "400" : weightStr;

                        String styleStr = parser.getAttributeValue(null, "style");
                        styleStr = (styleStr == null) ? "normal" : styleStr;

                        String filename = parser.nextText();
                        String fullFileName = systemFontPath + filename;

                        String key = name + "_" + weightStr + "_" + styleStr;
                        fontDict.put(key, fullFileName);
                    } else {
                        skip(parser);
                    }
                }
            } else if ("alias".equals(parser.getName())) {
                // Parse this alias to font to fileName
                String aliasName = parser.getAttributeValue(null, "name");
                String toName = parser.getAttributeValue(null, "to");
                String weightStr = parser.getAttributeValue(null, "weight");
                ArrayList<String> aliasWeights = new ArrayList<String>();
                String fontFilename;

                if (weightStr == null) {
                    aliasWeights = familyWeights;
                } else {
                    aliasWeights.add(weightStr);
                }

                for (String weight : aliasWeights) {
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

    private void skip(XmlPullParser parser) throws XmlPullParserException, IOException {
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
        InputStream in = null;
        boolean fallbackXML = false;
        final File fontFile = new File(fontXMLPath);

        String fileXml = "";

        if (fontFile.exists()) {
            fileXml = fontFile.getAbsolutePath();
        } else {
            File fontFileFallback = new File(fontXMLFallbackPath);
            if (fontFileFallback.exists()) {
                fileXml = fontFileFallback.getAbsolutePath();
            }
            fallbackXML = true;
        }

        if("".equals(fileXml)) {
            return;
        }

        try {
            in = new FileInputStream(fileXml);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        }

        XmlPullParser parser = Xml.newPullParser();

        try {
            parser.setInput(in, null);

            if (fallbackXML) {
                processDocumentFallback(parser);
            } else {
                processDocument(parser);
            }
        } catch(XmlPullParserException e) {
            e.printStackTrace();
        } catch(IOException e) {
            e.printStackTrace();
        }

        try {
            in.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public String getFontFile( String _key ) {
        if (fontDict.containsKey(_key)) {
            return fontDict.get(_key);
        } else {
            return "";
        }
    }

    /*
     * Returns the next available font fallback, or empty string if not found
     * The integer value determines the fallback priority (lower is higher)
     * The weightHint value determines the closest fallback hint for boldness
     * See /etc/fonts/font_fallback for documentation
     */
    public String getFontFallback(int importance, int weightHint) {
        Iterator it = fallbackFontDict.entrySet().iterator();
        Integer diffWeight = Integer.MAX_VALUE;
        String fallback = "";

        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            Integer diff = Math.abs((Integer) pair.getKey() - weightHint);

            if (diff < diffWeight) {
                Vector<String> fallbacks = (Vector<String>) pair.getValue();

                if (importance < fallbacks.size()) {
                    fallback = fallbacks.elementAt(importance);
                    diffWeight = diff;
                }
            }
        }

        return fallback;
    }

}

