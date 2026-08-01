#include <doctest.h>
#include <parsley/parsley.h>

using namespace parsley;

TEST_CASE("Node - construction and assignment")
{
    SUBCASE("construct from scalar types")
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
    SUBCASE("construct from vector")
    {
        Node node = std::vector<int>{1, 3, 3, 7};

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 4);

        REQUIRE(node[0].as<int>() == 1);
        REQUIRE(node[1].as<int>() == 3);
        REQUIRE(node[2].as<int>() == 3);
        REQUIRE(node[3].as<int>() == 7);
    }

    SUBCASE("assignment overwrites existing value")
    {
        Node node = "hello";
        node = "world";
        REQUIRE(node.as<std::string>() == "world");

        node = 42;
        REQUIRE(node.as<int>() == 42);
    }

    SUBCASE("assignment replaces structure entirely, not merges")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        node = 99;

        REQUIRE(node.is_scalar());
        REQUIRE(node.as<int>() == 99);
    }

    SUBCASE("move construction and move assignment")
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

TEST_CASE("Node - chained / nested access")
{
    SUBCASE("nested map access via chained operator[]")
    {
        Node node;

        node["database"]["host"] = "localhost";
        node["database"]["port"] = 5432;

        REQUIRE(node["database"]["host"].as<std::string>() == "localhost");
        REQUIRE(node["database"]["port"].as<int>() == 5432);
    }

    SUBCASE("mixed list and map access via chained operator[]")
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

    SUBCASE("nested chained access via const reference")
    {
        Node node;
        node["users"][0]["name"] = "Alice";

        const Node& cnode = node;

        REQUIRE(cnode["users"][0]["name"].as<std::string>() == "Alice");
    }
}

TEST_CASE("Node - push_back")
{
    SUBCASE("push_back onto null converts to list")
    {
        Node node;
        node.push_back(10);

        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0].as<int>() == 10);
    }

    SUBCASE("push_back onto existing list appends")
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

    SUBCASE("push_back onto scalar throws")
    {
        Node node = 42;
        REQUIRE_THROWS_AS(node.push_back(1), std::runtime_error);
    }

    SUBCASE("push_back onto map throws")
    {
        Node node;
        node["foo"] = 1;
        REQUIRE_THROWS_AS(node.push_back(1), std::runtime_error);
    }
}

TEST_CASE("Node - clear")
{
    SUBCASE("clear resets list to null")
    {
        Node node;
        node.push_back(1);
        node.push_back(2);

        node.clear();

        REQUIRE(node.is_null());
    }

    SUBCASE("clear resets map to null")
    {
        Node node;
        node["foo"] = 1;

        node.clear();

        REQUIRE(node.is_null());
    }

    SUBCASE("clear resets scalar to null")
    {
        Node node = 42;

        node.clear();

        REQUIRE(node.is_null());
    }

    SUBCASE("clear on already-null node is a no-op")
    {
        Node node;

        node.clear();

        REQUIRE(node.is_null());
    }
}

TEST_CASE("Node - identity")
{
    SUBCASE("default-constructed node is null")
    {
        Node node;

        REQUIRE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_list());
        REQUIRE_FALSE(node.is_map());
    }

    SUBCASE("scalar node reports is_scalar only")
    {
        Node node = 42;

        REQUIRE(node.is_scalar());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_list());
        REQUIRE_FALSE(node.is_map());
    }

    SUBCASE("list node reports is_list only")
    {
        Node node;
        node.push_back(1);

        REQUIRE(node.is_list());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_map());
    }

    SUBCASE("map node reports is_map only")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node.is_map());
        REQUIRE_FALSE(node.is_null());
        REQUIRE_FALSE(node.is_scalar());
        REQUIRE_FALSE(node.is_list());
    }

    SUBCASE("type() returns the correct enum value")
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

    SUBCASE("is() correctly identifies node type")
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
    SUBCASE("null node has size 0 and is empty")
    {
        Node node;

        REQUIRE(node.size() == 0);
        REQUIRE(node.empty());
    }

    SUBCASE("scalar node has size 1 and is not empty")
    {
        Node node = 42;

        REQUIRE(node.size() == 1);
        REQUIRE_FALSE(node.empty());
    }

    SUBCASE("list size reflects element count")
    {
        Node node;
        REQUIRE(node.empty());

        node.push_back(1);
        node.push_back(2);

        REQUIRE(node.size() == 2);
        REQUIRE_FALSE(node.empty());
    }

    SUBCASE("map size reflects key count")
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
    SUBCASE("round-trip scalar types")
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

    SUBCASE("round-trip std::vector<int>")
    {
        Node node = std::vector<int>{1, 3, 3, 7};

        auto values = node.as<std::vector<int>>();

        REQUIRE(values == std::vector<int>{1, 3, 3, 7});
    }

    SUBCASE("round-trip empty vector")
    {
        Node node = std::vector<int>{};

        auto values = node.as<std::vector<int>>();

        REQUIRE(values.empty());
    }

    SUBCASE("as<T>() on scalar node without matching Transfer falls back to stream extraction")
    {
        Node node = "42";

        REQUIRE(node.as<int>() == 42);
    }
}

TEST_CASE("Node - low-level scalar access")
{
    SUBCASE("set_scalar / get_scalar round-trip")
    {
        Node node;
        node.set_scalar("raw value");

        REQUIRE(node.get_scalar() == "raw value");
        REQUIRE(node.is_scalar());
    }

    SUBCASE("get_scalar on non-scalar node throws")
    {
        Node node;
        REQUIRE_THROWS_AS(node.get_scalar(), std::runtime_error);

        node.push_back(1);
        REQUIRE_THROWS_AS(node.get_scalar(), std::runtime_error);
    }
}
