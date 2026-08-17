#include "pch.h"

using namespace parsley;

TEST_CASE("Node - Transfer<T>")
{
    SUBCASE("String-like - write")
    {
        Node n1;
        Transfer<const char*>::write(n1, "c_str");
        REQUIRE(n1 == "c_str");

        Node n2;
        Transfer<StringView>::write(n2, StringView{ "StringView" });
        REQUIRE(n2 == "StringView");

        Node n3;
        Transfer<std::string>::write(n3, std::string{ "String" });
        REQUIRE(n3 == "String");
    }

    SUBCASE("String-like - write copies, does not alias source buffer")
    {
        char buf[] = "Temporary";
        Node n;
        Transfer<StringView>::write(n, StringView{ buf });

        buf[0] = 'X'; // mutate after write

        REQUIRE(n == "Temporary"); // must own a copy
    }

    SUBCASE("String-like - read")
    {
        Node n;
        n.set_scalar("Test"); // bypass Transfer::write deliberately

        std::string s;
        Transfer<std::string>::read(n, s);
        REQUIRE(s == "Test");

        StringView sv;
        Transfer<StringView>::read(n, sv);
        REQUIRE(sv == "Test");
    }

    SUBCASE("String-like - write moves from rvalue std::string")
    {
        std::string s = "Movable";
        Node n;
        Transfer<std::string>::write(n, std::move(s));

        REQUIRE(n == "Movable");
        REQUIRE(s.empty());
    }

    SUBCASE("String-like - write still copies from lvalue std::string")
    {
        std::string s = "Copied";
        Node n;
        Transfer<std::string>::write(n, s);

        REQUIRE(n == "Copied");
        REQUIRE(s == "Copied");
    }
}
