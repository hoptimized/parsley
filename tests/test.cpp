#include <catch2/catch_test_macros.hpp>
#include <parsley/parsley.h>

using namespace parsley;

TEST_CASE("Node - construction and assignment")
{
    SECTION("construct from scalar types")
    {
        Node n;
        REQUIRE(n.is_null());

        Node s = "hello";
        REQUIRE(s.as<std::string>() == "hello");
        REQUIRE(s.is_scalar());

        Node i = 42;
        REQUIRE(i.as<int>() == 42);
        REQUIRE(i.is_scalar());

        Node d = 3.14;
        REQUIRE(d.as<double>() == 3.14);
        REQUIRE(d.is_scalar());

        Node b = true;
        REQUIRE(b.as<bool>() == true);
        REQUIRE(b.is_scalar());
    }

    // TODO: move this to a transfer test unit
    SECTION("construct from vector")
    {
        Node node = std::vector<int>{1, 3, 3, 7};

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 4);

        REQUIRE(node[0].as<int>() == 1);
        REQUIRE(node[1].as<int>() == 3);
        REQUIRE(node[2].as<int>() == 3);
        REQUIRE(node[3].as<int>() == 7);
    }

    SECTION("assignment overwrites existing value")
    {
        Node node = "hello";
        node = "world";
        REQUIRE(node.as<std::string>() == "world");

        node = 42;
        REQUIRE(node.as<int>() == 42);
    }

    SECTION("assignment replaces structure entirely, not merges")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        node = 99;

        REQUIRE(node.is_scalar());
        REQUIRE(node.as<int>() == 99);
    }

    SECTION("move construction and move assignment")
    {
        Node node;
        node["foo"] = 1;

        Node moved(std::move(node));
        REQUIRE(moved["foo"].as<int>() == 1);

        Node target;
        target = std::move(moved);
        REQUIRE(target["foo"].as<int>() == 1);
    }
}

TEST_CASE("Node - operator[]")
{
    SECTION("null: failed access leaves node untouched (skip-index case)")
    {
        Node node;

        REQUIRE_THROWS_AS(node[5], std::runtime_error);
        REQUIRE(node.is_null());
    }

    SECTION("scalar: cannot be indexed")
    {
        Node node = 42;

        REQUIRE_THROWS_AS(node[0], std::runtime_error);
        REQUIRE_THROWS_AS(node["foo"], std::runtime_error);

        REQUIRE(node.is_scalar());
        REQUIRE(node.as<int>() == 42);
    }

    SECTION("scalar (const): cannot be indexed")
    {
        Node node = 42;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);

        REQUIRE(node.is_scalar());
        REQUIRE(node.as<int>() == 42);
    }

    SECTION("list: access existing element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);

        REQUIRE(node[0].as<int>() == 10);
        REQUIRE(node[1].as<int>() == 20);
    }

    SECTION("list: index at size() inserts a new element")
    {
        Node node;
        node.push_back(10);

        node[1] = 20;

        REQUIRE(node.size() == 2);
        REQUIRE(node[1].as<int>() == 20);
    }

    SECTION("list: index beyond size() throws and does not mutate")
    {
        Node node;
        node.push_back(10);

        REQUIRE_THROWS_AS(node[5], std::runtime_error);
        REQUIRE(node.size() == 1);
    }

    SECTION("list: negative index throws")
    {
        Node node;
        node.push_back(10);

        REQUIRE_THROWS_AS(node[-1], std::runtime_error);
    }

    SECTION("map: access existing key")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node["foo"].as<int>() == 1);
    }

    SECTION("map: access missing key inserts a new element")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        REQUIRE(node.size() == 2);
        REQUIRE(node["bar"].as<int>() == 2);
    }

    SECTION("map: reassigning an existing key overwrites, does not duplicate")
    {
        Node node;
        node["version"] = 1;
        node["version"] = 2;

        REQUIRE(node.size() == 1);
        REQUIRE(node["version"].as<int>() == 2);
    }

    SECTION("map: cannot be accessed by integral key")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE_THROWS_AS(node[0], std::runtime_error);
    }

    SECTION("conversion: null converts to list on integral access")
    {
        Node node;
        node[0] = 1;

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0].as<int>() == 1);
    }

    SECTION("conversion: null converts to map on string access")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
    }

    SECTION("conversion: list converts to map on string access, preserving old values under stringified indices")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);

        node["foo"] = 3;

        REQUIRE(node.is_map());
        REQUIRE(node.size() == 3);
        REQUIRE(node["0"].as<int>() == 1);
        REQUIRE(node["1"].as<int>() == 2);
        REQUIRE(node["foo"].as<int>() == 3);
    }

    SECTION("list (const): access existing list element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);
        const Node& cnode = node;

        REQUIRE(cnode[0].as<int>() == 10);
        REQUIRE(cnode[1].as<int>() == 20);
    }

    SECTION("list (const): access out-of-range index throws and does not mutate")
    {
        Node node;
        node.push_back(10);
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[5], std::runtime_error);
        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0].as<int>() == 10);
    }
    
    SECTION("list (const): string key on a list throws and does not convert it to a map")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);
        REQUIRE(node.is_list());
        REQUIRE(node.size() == 2);
        REQUIRE(node[0].as<int>() == 1);
        REQUIRE(node[1].as<int>() == 2);
    }

    SECTION("map (const): access existing map key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE(cnode["foo"].as<int>() == 1);
    }

    SECTION("map (const): access missing key throws and does not mutate")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode["bar"], std::runtime_error);
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
    }

    SECTION("map (const): cannot be accessed by integral key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
    }

    SECTION("null (const): access on null node throws and does not convert it")
    {
        Node node;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE(node.is_null());

        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);
        REQUIRE(node.is_null());
    }
}

TEST_CASE("Node - chained / nested access")
{
    SECTION("nested map access via chained operator[]")
    {
        Node node;

        node["database"]["host"] = "localhost";
        node["database"]["port"] = 5432;

        REQUIRE(node["database"]["host"].as<std::string>() == "localhost");
        REQUIRE(node["database"]["port"].as<int>() == 5432);
    }

    SECTION("mixed list and map access via chained operator[]")
    {
        Node node;

        node["users"][0]["name"] = "Alice";
        node["users"][0]["age"] = 30;

        node["users"][1]["name"] = "Bob";
        node["users"][1]["age"] = 25;

        REQUIRE(node["users"][0]["name"].as<std::string>() == "Alice");
        REQUIRE(node["users"][0]["age"].as<int>() == 30);

        REQUIRE(node["users"][1]["name"].as<std::string>() == "Bob");
        REQUIRE(node["users"][1]["age"].as<int>() == 25);
    }

    SECTION("nested chained access via const reference")
    {
        Node node;
        node["users"][0]["name"] = "Alice";

        const Node& cnode = node;

        REQUIRE(cnode["users"][0]["name"].as<std::string>() == "Alice");
    }
}

TEST_CASE("Node - push_back")
{
    SECTION("push_back onto null converts to list")
    {
        Node node;
        node.push_back(10);

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0].as<int>() == 10);
    }

    SECTION("push_back onto existing list appends")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);
        node.push_back(30);

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 3);
        REQUIRE(node[0].as<int>() == 10);
        REQUIRE(node[1].as<int>() == 20);
        REQUIRE(node[2].as<int>() == 30);
    }

    SECTION("push_back onto scalar throws")
    {
        Node node = 42;
        REQUIRE_THROWS_AS(node.push_back(1), std::runtime_error);
    }

    SECTION("push_back onto map throws")
    {
        Node node;
        node["foo"] = 1;
        REQUIRE_THROWS_AS(node.push_back(1), std::runtime_error);
    }
}

TEST_CASE("Node - clear")
{
    SECTION("clear resets list to null")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);

        node.clear();

        REQUIRE(node.is_null());
    }

    SECTION("clear resets map to null")
    {
        Node node;
        node["foo"] = 1;

        node.clear();

        REQUIRE(node.is_null());
    }

    SECTION("clear resets scalar to null")
    {
        Node node = 42;

        node.clear();

        REQUIRE(node.is_null());
    }

    SECTION("clear on already-null node is a no-op")
    {
        Node node;

        node.clear();

        REQUIRE(node.is_null());
    }
}

TEST_CASE("Node - identity")
{
    SECTION("default-constructed node is null")
    {
        Node node;

        REQUIRE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_list());
        REQUIRE_FALSE(node.is_map());
    }

    SECTION("scalar node reports is_scalar only")
    {
        Node node = 42;

        REQUIRE(node.is_scalar());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_list());
        REQUIRE_FALSE(node.is_map());
    }

    SECTION("list node reports is_list only")
    {
        Node node;
        node.push_back(1);

        REQUIRE(node.is_list());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_map());
    }

    SECTION("map node reports is_map only")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node.is_map());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_list());
    }

    SECTION("type() returns the correct enum value")
    {
        Node n;
        REQUIRE(n.type() == NodeType::Null);

        Node s = "hello";
        REQUIRE(s.type() == NodeType::Scalar);

        Node l = std::vector<int>{ 1,2,3 };
        REQUIRE(l.type() == NodeType::List);

        Node m;
        m["foo"] = 1;
        REQUIRE(m.type() == NodeType::Map);
    }

    SECTION("is() correctly identifies node type")
    {
        Node n;
        REQUIRE(n.is(NodeType::Null));
        REQUIRE(!n.is(NodeType::Scalar));
        REQUIRE(!n.is(NodeType::List));
        REQUIRE(!n.is(NodeType::Map));

        Node s = "hello";
        REQUIRE(!s.is(NodeType::Null));
        REQUIRE(s.is(NodeType::Scalar));
        REQUIRE(!s.is(NodeType::List));
        REQUIRE(!s.is(NodeType::Map));

        Node l = std::vector<int>{ 1,2,3 };
        REQUIRE(!l.is(NodeType::Null));
        REQUIRE(!l.is(NodeType::Scalar));
        REQUIRE(l.is(NodeType::List));
        REQUIRE(!l.is(NodeType::Map));

        Node m;
        m["foo"] = 1;
        REQUIRE(!m.is(NodeType::Null));
        REQUIRE(!m.is(NodeType::Scalar));
        REQUIRE(!m.is(NodeType::List));
        REQUIRE(m.is(NodeType::Map));
    }
}

TEST_CASE("Node - capacity")
{
    SECTION("null node has size 0 and is empty")
    {
        Node node;

        REQUIRE(node.size() == 0);
        REQUIRE(node.empty());
    }

    SECTION("scalar node has size 1 and is not empty")
    {
        Node node = 42;

        REQUIRE(node.size() == 1);
        REQUIRE_FALSE(node.empty());
    }

    SECTION("list size reflects element count")
    {
        Node node;
        REQUIRE(node.empty());

        node.push_back(1);
        node.push_back(2);

        REQUIRE(node.size() == 2);
        REQUIRE_FALSE(node.empty());
    }

    SECTION("map size reflects key count")
    {
        Node node;
        node["a"] = 1;
        node["b"] = 2;
        node["c"] = 3;

        REQUIRE(node.size() == 3);
        REQUIRE_FALSE(node.empty());
    }
}

TEST_CASE("Node - as<T>()")
{
    SECTION("round-trip scalar types")
    {
        Node s = "hello";
        REQUIRE(s.as<std::string>() == "hello");

        Node i = -123;
        REQUIRE(i.as<int>() == -123);

        Node d = 9.99;
        REQUIRE(d.as<double>() == 9.99);

        Node b = false;
        REQUIRE(b.as<bool>() == false);
    }

    SECTION("round-trip std::vector<int>")
    {
        Node node = std::vector<int>{1, 3, 3, 7};

        auto values = node.as<std::vector<int>>();

        REQUIRE(values == std::vector<int>{1, 3, 3, 7});
    }

    SECTION("round-trip empty vector")
    {
        Node node = std::vector<int>{};

        auto values = node.as<std::vector<int>>();

        REQUIRE(values.empty());
    }

    SECTION("as<T>() on scalar node without matching Transfer falls back to stream extraction")
    {
        Node node = "42";

        REQUIRE(node.as<int>() == 42);
    }
}

TEST_CASE("Node - low-level scalar access")
{
    SECTION("set_scalar / get_scalar round-trip")
    {
        Node node;
        node.set_scalar("raw value");

        REQUIRE(node.get_scalar() == "raw value");
        REQUIRE(node.is_scalar());
    }

    SECTION("get_scalar on non-scalar node throws")
    {
        Node node;
        REQUIRE_THROWS_AS(node.get_scalar(), std::runtime_error);

        node.push_back(1);
        REQUIRE_THROWS_AS(node.get_scalar(), std::runtime_error);
    }
}
