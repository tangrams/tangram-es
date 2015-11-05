#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include "tangram.h"
#include "text/textBuffer.h"

TEST_CASE( "Testing", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords(" varun  talwar");

    REQUIRE(words.size() == 2);
    REQUIRE(words[0].start == 1);
    REQUIRE(words[0].end== 5);
    REQUIRE(words[1].start == 8);
    REQUIRE(words[1].end== 13);
}

TEST_CASE( "Basic word break utf8 string", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords("The  quick brown fox");

    REQUIRE(words.size() == 4);
    REQUIRE(words[1].start == 5);
    REQUIRE(words[1].end == 9);
    REQUIRE(words[3].start == 17);
    REQUIRE(words[3].end == 19);
}

TEST_CASE( "Find words on non-utf8 string", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords("사용할 수있는 구절 많은");

    REQUIRE(words.size() == 4);
    REQUIRE(words[0].start == 0);
    REQUIRE(words[0].end == 2);
    REQUIRE(words[1].start == 4);
    REQUIRE(words[1].end == 6);
    REQUIRE(words[3].start == 11);
    REQUIRE(words[3].end == 12);
}

TEST_CASE( "Find words on non-utf8 string with CR", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords("Вяш выро\nконтынтёонэж\nад");

    REQUIRE(words.size() == 4);
    REQUIRE(words[0].start == 0);
    REQUIRE(words[0].end == 2);
    REQUIRE(words[1].start == 4);
    REQUIRE(words[1].end == 7);
    REQUIRE(words[2].start == 9);
    REQUIRE(words[2].end == 20);
}

TEST_CASE( "Find words on one word non-utf8 string with CR", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords("Вяш\n");

    REQUIRE(words.size() == 1);
    REQUIRE(words[0].start == 0);
    REQUIRE(words[0].end == 2);
}

TEST_CASE( "Find words in one character long strings", "[Core][TextBuffer]" ) {
    auto words = Tangram::TextBuffer::findWords("A\nB C D");

    REQUIRE(words.size() == 4);
    REQUIRE(words[0].start == 0);
    REQUIRE(words[0].end == 0);
    REQUIRE(words[1].start == 2);
    REQUIRE(words[1].end == 2);
    REQUIRE(words[2].start == 4);
    REQUIRE(words[2].end == 4);
    REQUIRE(words[3].start == 6);
    REQUIRE(words[3].end == 6);
}
