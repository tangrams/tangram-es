#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include "tangram.h"
#include "text/textBuffer.h"

TEST_CASE( "Basic word break utf8 string", "[Core][TextBuffer]" ) {
    auto breaks = Tangram::TextBuffer::findWordBreaks("The quick brown fox");

    REQUIRE(breaks.size() == 3);
    REQUIRE(breaks[0].start == 4);
    REQUIRE(breaks[0].end == 8);
    REQUIRE(breaks[2].start == 16);
    REQUIRE(breaks[2].end == 18);
}

TEST_CASE( "Find breaks on non-utf8 string", "[Core][TextBuffer]" ) {
    auto breaks = Tangram::TextBuffer::findWordBreaks("사용할 수있는 구절 많은");

    REQUIRE(breaks.size() == 3);
    REQUIRE(breaks[0].start == 4);
    REQUIRE(breaks[0].end == 6);
    REQUIRE(breaks[2].start == 11);
    REQUIRE(breaks[2].end == 12);
}

TEST_CASE( "Find breaks on non-utf8 string with CR", "[Core][TextBuffer]" ) {
    auto breaks = Tangram::TextBuffer::findWordBreaks("Вяш выро\nконтынтёонэж\nад");

    REQUIRE(breaks.size() == 3);
    REQUIRE(breaks[0].start == 4);
    REQUIRE(breaks[0].end == 7);
    REQUIRE(breaks[1].start == 9);
    REQUIRE(breaks[1].end == 20);
}

TEST_CASE( "Find breaks on one word non-utf8 string with CR", "[Core][TextBuffer]" ) {
    auto breaks = Tangram::TextBuffer::findWordBreaks("Вяш\n");

    REQUIRE(breaks.size() == 0);
}

TEST_CASE( "Find breaks in one character long strings", "[Core][TextBuffer]" ) {
    auto breaks = Tangram::TextBuffer::findWordBreaks("A\nB C D");

    REQUIRE(breaks.size() == 3);
    REQUIRE(breaks[0].start == 2);
    REQUIRE(breaks[0].end == 2);
    REQUIRE(breaks[2].start == 6);
    REQUIRE(breaks[2].end == 6);
}
