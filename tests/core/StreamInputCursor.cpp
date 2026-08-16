#include "pch.h"

using namespace parsley;

static constexpr const char* LOREM_IPSUM = R"(Lorem ipsum dolor sit amet,
consectetur adipiscing elit,
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam,
quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.
Excepteur sint occaecat cupidatat non proident,
sunt in culpa qui officia deserunt mollit anim id est laborum.)";

#define REQUIRE_CHAR(c, expected) \
    { \
        char res; \
        REQUIRE(!c.eof()); \
        REQUIRE(c.get_char(res)); \
        REQUIRE(res == expected); \
    }

#define REQUIRE_LINE(c, expected) \
    { \
        StringView line; \
        REQUIRE(!c.eof()); \
        REQUIRE(c.get_line(line)); \
        REQUIRE(line == expected); \
    }

TEST_CASE("StreamInputCursor")
{
    SUBCASE("get_char()")
    {
        std::istringstream is{ "Lorem ipsum" };
        StreamInputCursor c{ is };

        REQUIRE_CHAR(c, 'L');
        REQUIRE_CHAR(c, 'o');
        REQUIRE_CHAR(c, 'r');
        REQUIRE_CHAR(c, 'e');
        REQUIRE_CHAR(c, 'm');
        REQUIRE_CHAR(c, ' ');
        REQUIRE_CHAR(c, 'i');
        REQUIRE_CHAR(c, 'p');
        REQUIRE_CHAR(c, 's');
        REQUIRE_CHAR(c, 'u');
        REQUIRE_CHAR(c, 'm');
        REQUIRE(c.eof());
    }

    SUBCASE("get_char() - empty stream")
    {
        std::istringstream is{ "" };
        StreamInputCursor c{ is };
        char res;

        REQUIRE_FALSE(c.get_char(res));
        REQUIRE(c.eof());
    }

    SUBCASE("get_line()")
    {
        std::istringstream is{ LOREM_IPSUM };
        BasicStreamInputCursor<80> c{ is };

        REQUIRE_LINE(c, "Lorem ipsum dolor sit amet,");
        REQUIRE_LINE(c, "consectetur adipiscing elit,");
        REQUIRE_LINE(c, "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
        REQUIRE_LINE(c, "Ut enim ad minim veniam,");
        REQUIRE_LINE(c, "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");
        REQUIRE_LINE(c, "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.");
        REQUIRE_LINE(c, "Excepteur sint occaecat cupidatat non proident,");
        REQUIRE_LINE(c, "sunt in culpa qui officia deserunt mollit anim id est laborum.");
        REQUIRE(c.eof());
    }

    SUBCASE("get_line() - empty stream")
    {
        std::istringstream is{ "" };
        StreamInputCursor c{ is };

        StringView string_view;

        REQUIRE_FALSE(c.get_line(string_view));
        REQUIRE(c.eof());
    }

    SUBCASE("get_line() - empty line")
    {
        std::istringstream is{ "\nTest" };
        StreamInputCursor c{ is };

        StringView sv;

        // Read the empty line
        REQUIRE(c.get_line(sv));
        REQUIRE(sv.empty());
        REQUIRE(!c.eof());

        // Read the "Test" line
        REQUIRE(c.get_line(sv));
        REQUIRE(sv == "Test");
        REQUIRE(c.eof());

        // Attempt to read one more line
        REQUIRE(!c.get_line(sv));
        REQUIRE(c.eof());
    }
}
