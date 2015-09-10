package com.mapzen.tangram;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.util.Xml;
import android.util.Log;

class FontFileParser {

    private Map<String, String> fontDict = new HashMap<String, String>();

    private void processDocument(XmlPullParser parser) throws XmlPullParserException, IOException {

        parser.nextTag();
        // Parse Families
        parser.require(XmlPullParser.START_TAG, null, "familyset");
        while (parser.next() != XmlPullParser.END_DOCUMENT) {
            if (parser.getEventType() != XmlPullParser.START_TAG) {
                continue;
            }
            if ("family".equals(parser.getName())) {
                // Parse this family:
                String name = parser.getAttributeValue(null, "name");
                while (parser.next() != XmlPullParser.END_TAG) {
                    if (parser.getEventType() != XmlPullParser.START_TAG) {
                        continue;
                    }
                    String tag = parser.getName();
                    if ("font".equals(tag)) {
                        String weightStr = parser.getAttributeValue(null, "weight");
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
                weightStr = (weightStr == null) ? "400" : weightStr;
                String fontFilename = fontDict.get(toName + "_" + weightStr + "_normal"); // alias style is default: normal (always)
                fontDict.put(aliasName + "_" + weightStr, fontFilename);
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

    public FontFileParser() {

        InputStream in = null;
        final File fontFile = new File("/system/etc/fonts.xml");

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

}

