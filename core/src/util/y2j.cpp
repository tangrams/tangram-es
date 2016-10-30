#include "y2j.h"
#include "yaml.h"
#include <vector>

#ifndef Y2J_DEBUG
#define Y2J_DEBUG 1
#endif

namespace y2j {

struct Collection {
    size_t count = 0;
    bool isMapping = false;
};

template<typename Handler>
struct Generator {

    Generator(const char* bytes, size_t length, const char** errorMessage, size_t* errorLine) :
        errorMessage(errorMessage),
        errorLine(errorLine) {
        yaml_parser_initialize(&parser);
        yaml_parser_set_input_string(&parser, (const unsigned char*)bytes, length);
        yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
        yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    }

    ~Generator() {
        yaml_event_delete(&event);
        yaml_parser_delete(&parser);
    }

    yaml_parser_t parser;
    yaml_event_t event;
    Collection collection;
    std::vector<Collection> collectionStack;
    const char** errorMessage;
    size_t* errorLine;

    size_t getSeqLength() {
        return collection.count;
    }

    size_t getMapLength() {
        return collection.count / 2;
    }

    bool entryIsMapKey() {
        return collection.isMapping && collection.count % 2 == 0;
    }

    void pushCollection(bool isMapping) {
        ++collection.count;
        collectionStack.push_back(collection);
        collection.count = 0;
        collection.isMapping = isMapping;
    }

    void popCollection() {
        collection = collectionStack.back();
        collectionStack.pop_back();
    }

    bool operator()(Handler& handler) {

        bool ok = true;

        while (ok && event.type != YAML_STREAM_END_EVENT) {

            yaml_event_delete(&event);

            if (!yaml_parser_parse(&parser, &event)) {
                if (errorMessage) {
                    *errorMessage = parser.problem;
                }
                if (errorLine) {
                    *errorLine = parser.context_mark.line;
                }
                ok = false;
                break;
            }

            #if Y2J_DEBUG
            printEvent(event);
            #endif

            switch (event.type) {
            case YAML_NO_EVENT:
            case YAML_STREAM_START_EVENT:
            case YAML_STREAM_END_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_DOCUMENT_END_EVENT:
                break;
            case YAML_SEQUENCE_START_EVENT:
                // FIXME: If a sequence or mapping is found in a key, add a string version of this node instead.
                ok = handler.StartArray();
                pushCollection(false);
                break;
            case YAML_SEQUENCE_END_EVENT:
                ok = handler.EndArray(getSeqLength());
                popCollection();
                break;
            case YAML_MAPPING_START_EVENT:
                // FIXME: If a sequence or mapping is found in a key, add a string version of this node instead.
                ok = handler.StartObject();
                pushCollection(true);
                break;
            case YAML_MAPPING_END_EVENT:
                ok = handler.EndObject(getMapLength());
                popCollection();
                break;
            case YAML_ALIAS_EVENT:
                // FIXME: Support aliased nodes.
                break;
            case YAML_SCALAR_EVENT:
                if (entryIsMapKey()) {
                    ok = handler.Key((char*)event.data.scalar.value, event.data.scalar.length, true);
                } else {
                    ok = parseScalar(handler, event);
                }
                collection.count++;
                break;
            }
        }

        return ok;
    }

    bool parseScalar(Handler& handler, yaml_event_t event) {

        const char* value = (char*)event.data.scalar.value;
        size_t length = event.data.scalar.length;
        bool parsed = false;
        bool ok = true;

        switch (value[0]) {
        case '~':
        case 'n':
        case 'N':
            ok = parseNull(handler, value, length, &parsed);
            break;
        case 't':
        case 'T':
            ok = parseTrue(handler, value, length, &parsed);
            break;
        case 'f':
        case 'F':
            ok = parseFalse(handler, value, length, &parsed);
            break;
        default:
            ok = parseNumber(handler, value, length, &parsed);
            break;
        }

        if (ok && !parsed) {
            ok = handler.String(value, length, true);
        }
        return ok;
    }

    bool parseNull(Handler& handler, const char* value, size_t length, bool* parsed) {
        if ((length == 1 && value[0] == '~') ||
            (length == 4 && (strcmp(value, "null") == 0 || strcmp(value, "Null") == 0 || strcmp(value, "NULL") == 0))) {
            *parsed = true;
            return handler.Null();
        }
        return true;
    }

    bool parseTrue(Handler& handler, const char* value, size_t length, bool* parsed) {
        if (length == 4 && (strcmp(value, "true") == 0 || strcmp(value, "True") == 0 || strcmp(value, "TRUE") == 0)) {
            *parsed = true;
            return handler.Bool(true);
        }
        return true;
    }

    bool parseFalse(Handler& handler, const char* value, size_t length, bool* parsed) {
        if (length == 5 && (strcmp(value, "false") == 0 || strcmp(value, "False") == 0 || strcmp(value, "FALSE") == 0)) {
            *parsed = true;
            return handler.Bool(false);
        }
        return true;
    }

    bool parseNumber(Handler& handler, const char* value, size_t length, bool* parsed) {
        // TODO: Optimize parsing of doubles and integers.
        char* pos = (char*)value;
        long i = strtol(value, &pos, 10);
        if (pos == value + length) {
            *parsed = true;
            return handler.Int(i);
        }
        pos = (char*)value;
        double d = strtod(value, &pos);
        if (pos == value + length) {
            *parsed = true;
            return handler.Double(d);
        }
        return true;
    }

    #if Y2J_DEBUG
    void printEvent(yaml_event_t& event) {
        static int indent = 0;
        if (event.type == YAML_DOCUMENT_START_EVENT) { indent = 0; }
        if (event.type == YAML_SEQUENCE_END_EVENT || event.type == YAML_MAPPING_END_EVENT) { indent -= 2; }
        printf("%*s", indent, "");
        switch (event.type) {
        case YAML_NO_EVENT: printf("No event!\n"); break;
        case YAML_STREAM_START_EVENT: printf("Start Stream\n"); break;
        case YAML_STREAM_END_EVENT: printf("End Stream\n"); break;
        case YAML_DOCUMENT_START_EVENT: printf("Start Document\n"); break;
        case YAML_DOCUMENT_END_EVENT: printf("End Document\n"); break;
        case YAML_SEQUENCE_START_EVENT: printf("[\n"); indent += 2; break;
        case YAML_SEQUENCE_END_EVENT: printf("] (members: %lu)\n", getSeqLength()); break;
        case YAML_MAPPING_START_EVENT: printf("{\n"); indent += 2; break;
        case YAML_MAPPING_END_EVENT: printf("} (members: %lu)\n", getMapLength()); break;
        case YAML_ALIAS_EVENT: printf("Alias (anchor %s)\n", event.data.alias.anchor); break;
        case YAML_SCALAR_EVENT: printf(entryIsMapKey() ? "\"%s\":\n" : "\"%s\"\n", event.data.scalar.value); break;
        }
    }
    #endif
};

JsonDocument yamlParseBytes(const char* bytes, size_t length, const char** errorMessage, size_t* errorOffset) {

    Generator<JsonDocument> generator(bytes, length, errorMessage, errorOffset);
    JsonDocument document;
    document.Populate(generator);
    return document;
}

} // namespace y2j
