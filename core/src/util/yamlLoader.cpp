#include "yamlLoader.h"

#include "yaml-cpp/exceptions.h"
#include "yaml-cpp/mark.h"
#include "yaml-cpp/nodebuilder.h"
#include "yaml.h"
#include "log.h"

#include <vector>
#include <map>

namespace Tangram {


YAML::Node YamlLoader::load(const char* bytes, size_t length) {

    YAML::NodeBuilder handler;

    yaml_parser_t parser;
    yaml_event_t event;

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char*)bytes, length);
    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);

    std::map<yaml_char_t*, YAML::anchor_t> anchors;
    YAML::anchor_t curAnchor = 0;

    bool ok = true;

    auto anchorID = [&](yaml_char_t* anchor) {
        if (anchor) {
            anchors[anchor] = ++curAnchor;
            return curAnchor;
        }
        return YAML::NullAnchor;
    };

    std::string tag;
    std::string value;
    YAML::Mark mark;

    while (ok && event.type != YAML_STREAM_END_EVENT) {

        if (!yaml_parser_parse(&parser, &event)) {
            LOGE("%s, line %s", parser.problem, parser.context_mark.line);
            ok = false;
            break;
        }

        mark.pos = event.start_mark.index;
        mark.line = event.start_mark.line;
        mark.column = event.start_mark.column;
        tag.clear();

        switch (event.type) {
        case YAML_STREAM_START_EVENT:
        case YAML_NO_EVENT:
        case YAML_STREAM_END_EVENT:
            // nothing allocated in event structure
            break;

        case YAML_DOCUMENT_START_EVENT:
            handler.OnDocumentStart(mark);
            yaml_event_delete(&event);
            break;

        case YAML_DOCUMENT_END_EVENT:
            handler.OnDocumentEnd();
            yaml_event_delete(&event);
            break;

        case YAML_SEQUENCE_START_EVENT: {
            if (event.data.sequence_start.tag) {
                tag = reinterpret_cast<const char*>(event.data.sequence_start.tag);
                free(event.data.sequence_start.tag);
            }

            auto style = static_cast<YAML::EmitterStyle::value>(event.data.sequence_start.style);
            handler.OnSequenceStart(mark, tag, anchorID(event.data.sequence_start.anchor), style);
            break;
        }
        case YAML_SEQUENCE_END_EVENT:
            handler.OnSequenceEnd();
            break;

        case YAML_MAPPING_START_EVENT: {
            if (event.data.mapping_start.tag) {
                tag = reinterpret_cast<const char*>(event.data.mapping_start.tag);
                free(event.data.mapping_start.tag);
            }
            auto style = static_cast<YAML::EmitterStyle::value>(event.data.mapping_start.style);
            handler.OnMapStart(mark, tag, anchorID(event.data.mapping_start.anchor), style);
            break;
        }
        case YAML_MAPPING_END_EVENT:
            handler.OnMapEnd();
            break;

        case YAML_ALIAS_EVENT: {

            auto it = anchors.find(event.data.alias.anchor);
            if (it != anchors.end()) {
                handler.OnAlias(mark, it->second);
            } else {
                // LOG undefined anchor?
                //handler.OnAlias(mark, 0);
                // push Null?
            }
            break;
        }
        case YAML_SCALAR_EVENT:
            if (event.data.scalar.tag) {
                tag = reinterpret_cast<const char*>(event.data.scalar.tag);
                free(event.data.scalar.tag);
            }

            if (event.data.scalar.value) {
                value = reinterpret_cast<const char*>(event.data.scalar.value);
                free(event.data.scalar.value);
            } else {
                value.clear();
            }

            if ((value.length() == 1 && value[0] == '~') ||
                (value.length() == 4 && (value == "null" || value == "Null" || value == "NULL")) ||
                (value.empty())) {
                handler.OnNull(mark, anchorID(event.data.scalar.anchor));
            } else {
                handler.OnScalar(mark, tag, anchorID(event.data.scalar.anchor), value);
            }
            break;
        }
    }

    for (auto& a : anchors) { free(a.first); }

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);

    if (!ok) {
        throw new YAML::ParserException(mark, "");
    }
    return handler.Root();
}

}
