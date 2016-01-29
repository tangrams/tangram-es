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

                            String styleStr = parser.getAttributeValue(null, "style");
                            styleStr = (styleStr == null) ? "normal" : styleStr;

                            String filename = parser.nextText();

                            Integer weight = new Integer(weightStr);
                            if (!fallbackFontDict.containsKey(weight)) {
                                fallbackFontDict.put(weight, new Vector<String>());
                            }

                            fallbackFontDict.get(weight).add(filename);
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
                        String fullFilename = "/system/fonts/" + filename;

                        String key = name + "_" + weightStr + "_" + styleStr;
                        fontDict.put(key, fullFilename);
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

    public void parse(String fontXMLPath) {
        InputStream in = null;
        final File fontFile = new File(fontXMLPath);

        // TODO: Handle system_fonts parsing which has a different xml layout as compared to fonts.xml
        //       system_fonts.xml also does not seem to have a good css style font parameter mapping as is the case with
        //       fonts.xml (fonts.xml is available in android L and above)

        String fileXml = "";

        if (fontFile.exists()) {
            fileXml = fontFile.getAbsolutePath();
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
            processDocument(parser);
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

