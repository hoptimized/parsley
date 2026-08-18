#include "pch.h"

using namespace parsley;

struct Address
{
    std::string street;
    int zip_code;
    std::string country;
};

struct Customer
{
    std::string first_name;
    std::string second_name;
    std::vector<Address> addresses;
};

template <>
struct parsley::Transfer<Address>
{
    static void read(const Node& node, Address& val)
    {
        val.street = node["street"].as<std::string>();
        val.zip_code = node["zip"].as<int>();
        val.country = node["country"].as<std::string>();
    }

    static void write(Node& node, const Address& val)
    {
        node["street"] = val.street;
        node["zip"] = val.zip_code;
        node["country"] = val.country;
    }
};

template <>
struct parsley::Transfer<Customer>
{
    static void read(const Node& node, Customer& val)
    {
        val.first_name = node["first_name"].as<std::string>();
        val.second_name = node["second_name"].as<std::string>();
        val.addresses = node["addresses"].as<std::vector<Address>>();
    }

    static void write(Node& node, const Customer& val)
    {
        node["first_name"] = val.first_name;
        node["second_name"] = val.second_name;
        node["addresses"] = val.addresses;
    }
};

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

    SUBCASE("Custom data type")
    {
        Customer customer{
            "John",
            "Doe",
            {
                { "1600 Pennsylvania Avenue NW", 20500, "USA" },
                { "350 Fifth Avenue", 10118, "USA" }
            }
        };

        std::string stringified = parsley::write<YAML>(customer);

        REQUIRE(stringified == R"(---
first_name: John
second_name: Doe
addresses:
  - street: 1600 Pennsylvania Avenue NW
    zip: 20500
    country: USA
  - street: 350 Fifth Avenue
    zip: 10118
    country: USA
)");

        Customer parsed = parsley::read<YAML>(stringified).as<Customer>();

        REQUIRE(parsed.first_name == "John");
        REQUIRE(parsed.second_name == "Doe");
        REQUIRE(parsed.addresses[0].street == "1600 Pennsylvania Avenue NW");
        REQUIRE(parsed.addresses[0].zip_code == 20500);
        REQUIRE(parsed.addresses[0].country == "USA");
        REQUIRE(parsed.addresses[1].street == "350 Fifth Avenue");
        REQUIRE(parsed.addresses[1].zip_code == 10118);
        REQUIRE(parsed.addresses[1].country == "USA");
    }
}
