#include "pch.h"

using namespace parsley;

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
