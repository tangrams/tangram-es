package com.mapzen.tangram;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.util.Xml;
import android.util.Log;

public class FontFileParser {
    private static final File fontFile = new File("/system/etc/fonts.xml");
    private static final File systemFontFile = new File("/system/etc/system_fonts.xml");
    private static final File fallbackFontFile = new File("/system/etc/fallback_fonts.xml");

    private static Map<String, String> fontDict = new HashMap<String, String>();

    private void processDocument(XmlPullParser parser) throws XmlPullParserException, IOException {

        parser.nextTag();
        // Parse Families
        parser.require(XmlPullParser.START_TAG, null, "familyset");
        while (parser.next() != XmlPullParser.END_TAG) {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }
            if (parser.getName().equals("family")) {
                // Parse this family:
                String name = parser.getAttributeValue(null, "name");
                while (parser.next() != XmlPullParser.END_TAG) {
                    if (parser.getEventType() != XmlPullParser.START_TAG) {
                        continue;
                    }
                    String tag = parser.getName();
                    if (tag.equals("font")) {
                        String weightStr = parser.getAttributeValue(null, "weight");
                        String styleStr = parser.getAttributeValue(null, "style");
                        String filename = parser.nextText();
                        String fullFilename = "/system/fonts/" + filename;
                        String key = name + "_" + weightStr + "_" + styleStr;
                        fontDict.put(key, fullFilename);
                    } else {
                        skip(parser);
                    }
                }
            } /*else if (parser.getName().equals("alias")) {
                // Parse this alias to font to fileName
                String aliasName = parser.getAttributeValue(null, "name");
                String toName = parser.getAttributeValue(null, "to");
                String weightStr = parser.getAttributeValue(null, "weight");
                if (weightStr == "") { weightStr = "400"; } // normal
                String fontFilename = fontDict.get(toName + "_" + weightStr + "_normal"); // alias style is default: normal
                fontDict.put(aliasName + "_" + weightStr, fontFilename);
            } */else {
                skip(parser);
            }
        }
    }

    private static void skip(XmlPullParser parser) throws XmlPullParserException, IOException {
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

    public void init() throws XmlPullParserException, IOException {
        String fileXml = "";

        if (fontFile.exists()) {
            fileXml = fontFile.getAbsolutePath();
        } else if (systemFontFile.exists()) {
            fileXml = systemFontFile.getAbsolutePath();
        } else if (fallbackFontFile.exists()) {
            fileXml = systemFontFile.getAbsolutePath();
        }

        if(fileXml == "") {
            return;
        }

        InputStream in = new FileInputStream(fileXml);
        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(in, null);
            processDocument(parser);
        } finally {
            in.close();
        }
    }

    public String getFontFile( String _key ) {
        if (fontDict.containsKey(_key)) {
            return fontDict.get(_key);
        } else {
            return "";
        }
    }

}

