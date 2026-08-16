#include "pch.h"

using namespace parsley;

TEST_CASE("Node - operator[]")
{
    SUBCASE("null: failed access leaves node untouched (skip-index case)")
    {
        Node node;

        REQUIRE_THROWS_AS(node[5], std::runtime_error);
        REQUIRE(node.is_null());
    }

    SUBCASE("scalar: cannot be indexed")
    {
        Node node = 42;

        REQUIRE_THROWS_AS(node[0], std::runtime_error);
        REQUIRE_THROWS_AS(node["foo"], std::runtime_error);

        REQUIRE(node.is_scalar());
        REQUIRE(node == 42);
    }

    SUBCASE("scalar (const): cannot be indexed")
    {
        Node node = 42;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);

        REQUIRE(node.is_scalar());
        REQUIRE(node == 42);
    }

    SUBCASE("list: access existing element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);

        REQUIRE(node[0] == 10);
        REQUIRE(node[1] == 20);
    }

    SUBCASE("list: index at size() inserts a new element")
    {
        Node node;
        node.push_back(10);

        node[1] = 20;

        REQUIRE(node.size() == 2);
        REQUIRE(node[1] == 20);
    }

    SUBCASE("list: index beyond size() throws and does not mutate")
    {
        Node node;
        node.push_back(10);

        REQUIRE_THROWS_AS(node[5], std::runtime_error);
        REQUIRE(node.size() == 1);
    }

    SUBCASE("list: negative index throws")
    {
        Node node;
        node.push_back(10);

        REQUIRE_THROWS_AS(node[-1], std::runtime_error);
    }

    SUBCASE("map: access existing key")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node["foo"] == 1);
    }

    SUBCASE("map: access missing key inserts a new element")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        REQUIRE(node.size() == 2);
        REQUIRE(node["bar"] == 2);
    }

    SUBCASE("map: reassigning an existing key overwrites, does not duplicate")
    {
        Node node;
        node["version"] = 1;
        node["version"] = 2;

        REQUIRE(node.size() == 1);
        REQUIRE(node["version"] == 2);
    }

    SUBCASE("map: cannot be accessed by integral key")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE_THROWS_AS(node[0], std::runtime_error);
    }

    SUBCASE("conversion: null converts to list on integral access")
    {
        Node node;
        node[0] = 1;

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0] == 1);
    }

    SUBCASE("conversion: null converts to map on string access")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"] == 1);
    }

    SUBCASE("conversion: list converts to map on string access, preserving old values under stringified indices")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);

        node["foo"] = 3;

        REQUIRE(node.is_map());
        REQUIRE(node.size() == 3);
        REQUIRE(node["0"] == 1);
        REQUIRE(node["1"] == 2);
        REQUIRE(node["foo"] == 3);
    }

    SUBCASE("list (const): access existing list element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);
        const Node& cnode = node;

        REQUIRE(cnode[0] == 10);
        REQUIRE(cnode[1] == 20);
    }

    SUBCASE("list (const): access out-of-range index throws and does not mutate")
    {
        Node node;
        node.push_back(10);
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[5], std::runtime_error);
        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0] == 10);
    }
    
    SUBCASE("list (const): string key on a list throws and does not convert it to a map")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);
        REQUIRE(node.is_list());
        REQUIRE(node.size() == 2);
        REQUIRE(node[0] == 1);
        REQUIRE(node[1] == 2);
    }

    SUBCASE("map (const): access existing map key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE(cnode["foo"] == 1);
    }

    SUBCASE("map (const): access missing key throws and does not mutate")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode["bar"], std::runtime_error);
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"] == 1);
    }

    SUBCASE("map (const): cannot be accessed by integral key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"] == 1);
    }

    SUBCASE("null (const): access on null node throws and does not convert it")
    {
        Node node;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE(node.is_null());

        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);
        REQUIRE(node.is_null());
    }
}
