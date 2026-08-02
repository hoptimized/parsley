#include "pch.h"

using namespace parsley;

TEST_CASE("YAML - Serialize")
{
    SUBCASE("serialize - root is a scalar")
    {
        Node n = "hello";
        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == "---\nhello\n");
    }

    SUBCASE("serialize - root is null")
    {
        Node n;
        auto serialized = serialize(n, Format::YAML);

        REQUIRE(serialized == "---\n");
    }

    SUBCASE("serialize - root is a flat list of scalars")
    {
        Node n;
        n.push_back(1);
        n.push_back(2);
        n.push_back(3);

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
- 1
- 2
- 3
)");
    }

    SUBCASE("serialize - root is a flat map")
    {
        Node n;
        n["a"] = 1;
        n["b"] = 2;

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
a: 1
b: 2
)");
    }

    SUBCASE("serialize - map with a null value")
    {
        Node n;
        n["a"] = 1;
        n["b"];     // null
        n["c"] = 3;

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
a: 1
b: 
c: 3
)");
    }

    SUBCASE("serialize - list of lists")
    {
        Node n;
        n[0].push_back(1);
        n[0].push_back(2);
        n[1].push_back(3);
        n[1].push_back(4);

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
- - 1
  - 2
- - 3
  - 4
)");
    }

    SUBCASE("serialize - list containing a mix of scalars and maps")
    {
        Node n;
        n[0] = "plain";
        n[1]["key"] = "value";

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
- plain
- key: value
)");
    }

    SUBCASE("serialize - deeply nested single-key chain")
    {
        Node n;
        n["a"]["b"]["c"]["d"] = "leaf";

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
a:
  b:
    c:
      d: leaf
)");
    }

    SUBCASE("serialize - list of maps, each containing a nested list")
    {
        Node n;
        n[0]["name"] = "a";
        n[0]["tags"].push_back("x");
        n[0]["tags"].push_back("y");

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
- name: a
  tags:
    - x
    - y
)");
    }

    SUBCASE("serialize - full document")
    {
        Node n;

        n["name"] = "demo-app";
        n["version"] = "1.0";

        n["server"]["host"] = "localhost";
        n["server"]["ports"].push_back(8080);
        n["server"]["ports"].push_back(8081);
        n["server"]["tls"]["enabled"] = true;
        n["server"]["tls"]["cert"] = "server.pem";

        n["database"]["engine"] = "postgres";
        n["database"]["connection"]["host"] = "db.local";
        n["database"]["connection"]["user"] = "app";
        n["database"]["connection"]["options"]["pool_size"] = 10;

        n["workers"][0]["name"] = "ingest";
        n["workers"][0]["queue"] = "events";
        n["workers"][0]["retries"]["max"] = 3;
        n["workers"][0]["retries"]["delay"] = 5;

        n["workers"][1]["name"] = "cleanup";
        n["workers"][1]["queue"] = "jobs";
        n["workers"][1]["retries"]["max"] = 2;
        n["workers"][1]["retries"]["delay"] = 10;

        auto serialized = serialize(n, Format::YAML);
        REQUIRE(serialized == R"(---
name: demo-app
version: 1.0
server:
  host: localhost
  ports:
    - 8080
    - 8081
  tls:
    enabled: true
    cert: server.pem
database:
  engine: postgres
  connection:
    host: db.local
    user: app
    options:
      pool_size: 10
workers:
  - name: ingest
    queue: events
    retries:
      max: 3
      delay: 5
  - name: cleanup
    queue: jobs
    retries:
      max: 2
      delay: 10
)");
    }
}
