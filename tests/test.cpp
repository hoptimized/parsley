#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
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
        REQUIRE(node.as<int>() == 42);
    }

    SUBCASE("scalar (const): cannot be indexed")
    {
        Node node = 42;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE_THROWS_AS(cnode["foo"], std::runtime_error);

        REQUIRE(node.is_scalar());
        REQUIRE(node.as<int>() == 42);
    }

    SUBCASE("list: access existing element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);

        REQUIRE(node[0].as<int>() == 10);
        REQUIRE(node[1].as<int>() == 20);
    }

    SUBCASE("list: index at size() inserts a new element")
    {
        Node node;
        node.push_back(10);

        node[1] = 20;

        REQUIRE(node.size() == 2);
        REQUIRE(node[1].as<int>() == 20);
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

        REQUIRE(node["foo"].as<int>() == 1);
    }

    SUBCASE("map: access missing key inserts a new element")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        REQUIRE(node.size() == 2);
        REQUIRE(node["bar"].as<int>() == 2);
    }

    SUBCASE("map: reassigning an existing key overwrites, does not duplicate")
    {
        Node node;
        node["version"] = 1;
        node["version"] = 2;

        REQUIRE(node.size() == 1);
        REQUIRE(node["version"].as<int>() == 2);
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
        REQUIRE(node[0].as<int>() == 1);
    }

    SUBCASE("conversion: null converts to map on string access")
    {
        Node node;
        node["foo"] = 1;

        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
    }

    SUBCASE("conversion: list converts to map on string access, preserving old values under stringified indices")
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

    SUBCASE("list (const): access existing list element")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);
        const Node& cnode = node;

        REQUIRE(cnode[0].as<int>() == 10);
        REQUIRE(cnode[1].as<int>() == 20);
    }

    SUBCASE("list (const): access out-of-range index throws and does not mutate")
    {
        Node node;
        node.push_back(10);
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[5], std::runtime_error);
        REQUIRE(node.is_list());
        REQUIRE(node.size() == 1);
        REQUIRE(node[0].as<int>() == 10);
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
        REQUIRE(node[0].as<int>() == 1);
        REQUIRE(node[1].as<int>() == 2);
    }

    SUBCASE("map (const): access existing map key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE(cnode["foo"].as<int>() == 1);
    }

    SUBCASE("map (const): access missing key throws and does not mutate")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode["bar"], std::runtime_error);
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
    }

    SUBCASE("map (const): cannot be accessed by integral key")
    {
        Node node;
        node["foo"] = 1;
        const Node& cnode = node;

        REQUIRE_THROWS_AS(cnode[0], std::runtime_error);
        REQUIRE(node.is_map());
        REQUIRE(node.size() == 1);
        REQUIRE(node["foo"].as<int>() == 1);
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

TEST_CASE("Node - iteration")
{
    SUBCASE("list - iteration works")
    {
        Node list = std::vector<int>{ 1, 3, 3, 7 };

        std::vector<int> seen;
        for (auto entry : list)
            seen.push_back(entry.as<int>());

        REQUIRE(seen == std::vector<int>{1, 3, 3, 7});
    }

    SUBCASE("list - empty list produces no iterations")
    {
        Node list = std::vector<int>{};
        int count = 0;
        for (auto entry : list){ (void)entry; ++count; }
        REQUIRE(count == 0);
    }

    SUBCASE("list - mutation through iterator is visible")
    {
        Node list = std::vector<int>{ 1, 2, 3 };
        for (auto entry : list)
            entry = entry.as<int>() + 10;

        std::vector<int> seen;
        for (auto entry : list)
            seen.push_back(entry.as<int>());

        REQUIRE(seen == std::vector<int>{11, 12, 13});
    }

    SUBCASE("list - key is empty for list entries")
    {
        Node list = std::vector<int>{ 42 };

        for (auto entry : list)
            REQUIRE(entry.key.empty());
    }

    SUBCASE("map - iteration works")
    {
        Node map;
        map["foo"] = 1;
        map["bar"] = 2;

        std::vector<std::pair<std::string, int>> seen;
        for (auto kvp : map)
            seen.emplace_back(std::make_pair(kvp.key, kvp.value.as<int>()));

        REQUIRE(seen[0].first == "foo");
        REQUIRE(seen[0].second == 1);
        REQUIRE(seen[1].first == "bar");
        REQUIRE(seen[1].second == 2);
    }

    SUBCASE("map - empty map produces no iterations")
    {
        Node map;
        int count = 0;
        for (auto kvp : map) { (void)kvp; ++count; }
        REQUIRE(count == 0);
    }

    SUBCASE("map - mutation through iterator is visible")
    {
        Node map;
        map["foo"] = 1;
        map["bar"] = 2;

        for (auto kvp : map)
            kvp.value = kvp.value.as<int>() * 100;

        REQUIRE(map["foo"].as<int>() == 100);
        REQUIRE(map["bar"].as<int>() == 200);
    }

    SUBCASE("const - const iteration works")
    {
        const Node list = std::vector<int>{ 5, 6 };
        int sum = 0;

        for (auto val : list)
            sum += val.as<int>();
        
        REQUIRE(sum == 11);
    }

    SUBCASE("const - begin()/end() on const Node yield ConstIterator")
    {
        const Node list = std::vector<int>{ 1, 2 };
        static_assert(std::is_same<decltype(list.begin()), Node::ConstIterator>::value, "");
        static_assert(std::is_same<decltype(list.end()), Node::ConstIterator>::value, "");
    }

    SUBCASE("const - cbegin()/cend() yield ConstIterator even on non-const Node")
    {
        Node list = std::vector<int>{ 1, 2 };
        static_assert(std::is_same<decltype(list.cbegin()), Node::ConstIterator>::value, "");
        static_assert(std::is_same<decltype(list.cend()), Node::ConstIterator>::value, "");
    }

    SUBCASE("Iterator - Iterator converts implicitly to ConstIterator")
    {
        Node list = std::vector<int>{ 1, 2 };
        Node::Iterator it = list.begin();
        Node::ConstIterator cit = it; // implicit conversion
        REQUIRE(cit->as<int>() == 1);
    }

    SUBCASE("Iterator - comparison works in both directions between Iterator and ConstIterator")
    {
        Node list = std::vector<int>{ 1, 2, 3 };

        auto it = list.begin();
        auto cit = list.cbegin();

        REQUIRE(it == cit);
        REQUIRE(cit == it);

        ++it;
        REQUIRE(it != cit);
        REQUIRE(cit != it);
    }

    SUBCASE("Iterator - mutable iterator compares equal to const end()")
    {
        Node list = std::vector<int>{ 1 };
        auto it = list.begin();
        ++it;

        REQUIRE(it == list.cend());
        REQUIRE(list.cend() == it);
    }

    SUBCASE("Iterator - comparison across maps works")
    {
        Node map;
        map["a"] = 1;

        auto it = map.begin();
        auto cit = map.cbegin();

        REQUIRE(it == cit);
        ++it;
        REQUIRE(it != cit);
        REQUIRE(it == map.cend());
    }

    SUBCASE("Iterator - arrow operator")
    {
        Node list = std::vector<int>{ 5, 6 };
        
        auto it = list.begin();
        REQUIRE(it->as<int>() == 5);

        ++it;
        REQUIRE(it->as<int>() == 6);

        ++it;
        REQUIRE(it == list.end());
    }

    SUBCASE("Iterator - arrow operator on const iterator")
    {
        const Node list = std::vector<int>{ 5, 6 };

        auto it = list.begin();
        REQUIRE(it->as<int>() == 5);

        ++it;
        REQUIRE(it->as<int>() == 6);

        ++it;
        REQUIRE(it == list.end());
    }

    SUBCASE("Iterator - postfix increment returns pre-increment position")
    {
        Node list = std::vector<int>{ 1, 2, 3 };

        auto it = list.begin();
        auto prev = it++;

        REQUIRE(prev->as<int>() == 1);
        REQUIRE(it->as<int>() == 2);
    }

    SUBCASE("Entry - exposes Node interface")
    {
        Node map;
        map["foo"] = 1;

        for (auto kvp : map)
        {
            REQUIRE(kvp.as<int>() == 1);

            kvp = 2;
            REQUIRE(kvp.as<int>() == 2);

            // TODO: add missing methods
        }
    }

    SUBCASE("Entry - ConstEntry is not assignable")
    {
        static_assert(!std::is_assignable<Node::ConstEntry&, int>::value,
            "ConstEntry should not support operator= to prevent mutation through const iteration");
    }

    SUBCASE("Entry - mutable Entry is assignable")
    {
        static_assert(std::is_assignable<Node::Entry&, int>::value,
            "Entry should support operator= for mutation through non-const iteration");
    }

    SUBCASE("Entry - Entry converts implicitly to ConstEntry, not vice versa")
    {
        static_assert(std::is_convertible<Node::Entry, Node::ConstEntry>::value, "");
        static_assert(!std::is_convertible<Node::ConstEntry, Node::Entry>::value, "");
    }

    SUBCASE("Entry - dereferencing a const iterator yields read-only access")
    {
        const Node list = std::vector<int>{ 9 };
        static_assert(std::is_same<decltype(*list.begin()), Node::ConstEntry>::value, "");
        static_assert(std::is_same<decltype(*list.end()), Node::ConstEntry>::value, "");
    }

    SUBCASE("Entry - copying a ConstEntry preserves read-only value reference")
    {
        Node list = std::vector<int>{ 9 };
        auto cit = list.cbegin();
        auto entry = *cit; // deduces Node::ConstEntry
        static_assert(std::is_same<decltype(entry), Node::ConstEntry>::value, "");
        REQUIRE(entry.as<int>() == 9);
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
