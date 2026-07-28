#include <catch2/catch_test_macros.hpp>
#include <parsley.h>

using namespace parsley;

TEST_CASE("experiments")
{

}

TEST_CASE("Node - scalar values")
{
    SECTION("string")
    {
        Node node = "hello";
        
        REQUIRE(node.as<std::string>() == "hello");

        node = "world";

        REQUIRE(node.as<std::string>() == "world");
    }

    SECTION("integer")
    {
        Node node = 42;

        REQUIRE(node.as<int>() == 42);

        node = -123;

        REQUIRE(node.as<int>() == -123);
    }

    SECTION("floating point")
    {
        Node node = 3.14;

        REQUIRE(node.as<double>() == 3.14);

        node = 9.99;

        REQUIRE(node.as<double>() == 9.99);
    }

    SECTION("boolean")
    {
        Node node = true;

        REQUIRE(node.as<bool>() == true);

        node = false;

        REQUIRE(node.as<bool>() == false);
    }
}

TEST_CASE("Node - sequence values")
{
    SECTION("initialization")
    {
        Node node = std::vector<int>{1, 3, 3, 7};

        REQUIRE(node[0].as<int>() == 1);
        REQUIRE(node[1].as<int>() == 3);
        REQUIRE(node[2].as<int>() == 3);
        REQUIRE(node[3].as<int>() == 7);

        auto values = node.as<std::vector<int>>();

        REQUIRE(values.size() == 4);
        REQUIRE(values == std::vector<int>{1, 3, 3, 7});
    }

    SECTION("insertion")
    {
        Node node;
        node.push_back(10);
        node.push_back(20);
        node.push_back(30);

        REQUIRE(node[0].as<int>() == 10);
        REQUIRE(node[1].as<int>() == 20);
        REQUIRE(node[2].as<int>() == 30);
    }
}

TEST_CASE("Node - object values")
{
    SECTION("initialization")
    {
        Node node;
        node["foo"] = 1;
        node["bar"] = 2;

        REQUIRE(node["foo"].as<int>() == 1);
        REQUIRE(node["bar"].as<int>() == 2);
    }

    SECTION("Node - object mutation")
    {
        Node node;

        node["name"] = "parsley";
        node["version"] = 1;

        REQUIRE(node["name"].as<std::string>() == "parsley");
        REQUIRE(node["version"].as<int>() == 1);

        node["version"] = 2;

        REQUIRE(node["version"].as<int>() == 2);
    }
}

TEST_CASE("Node - nested objects")
{
    Node node;

    node["database"]["host"] = "localhost";
    node["database"]["port"] = 5432;

    REQUIRE(node["database"]["host"].as<std::string>() == "localhost");
    REQUIRE(node["database"]["port"].as<int>() == 5432);
}


TEST_CASE("Node - mixed object and sequence")
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
