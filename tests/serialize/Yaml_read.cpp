#include "parsley/serialize/YamlDeserializer.h"
#include "pch.h"

using namespace parsley;

TEST_CASE("YAML - Deserialize")
{
    SUBCASE("deserialize - root is null")
    {
        Node n = parsley::read<YAML>("");
        REQUIRE(n.is_null());
    }

    SUBCASE("deserialize - root is scalar")
    {
        Node n = parsley::read<YAML>("test");

        REQUIRE(n.is_scalar());
        REQUIRE(n.as<StringView>() == "test");
    }

    SUBCASE("deserialize - root is sequence")
    {
        Node n = parsley::read<YAML>("- foo\n- bar");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0].as<StringView>() == "foo");
        REQUIRE(n[1].as<StringView>() == "bar");
    }

    SUBCASE("deserialize - sequence with null children")
    {
        Node n = parsley::read<YAML>("-");
        
        REQUIRE(n.is_list());
        REQUIRE(n.size() == 1);
        REQUIRE(n[0].is_null());

        n = parsley::read<YAML>("- a\n- \n- b");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 3);
        REQUIRE(n[0].as<StringView>() == "a");
        REQUIRE(n[1].is_null());
        REQUIRE(n[2].as<StringView>() == "b");

        n = parsley::read<YAML>("- a\n- b\n-");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 3);
        REQUIRE(n[0].as<StringView>() == "a");
        REQUIRE(n[1].as<StringView>() == "b");
        REQUIRE(n[2].is_null());
    }

    SUBCASE("deserialize - sequence of scalars")
    {
        Node n = parsley::read<YAML>("- a\n- b");
        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
    }

    SUBCASE("deserialize - nested sequences")
    {
        Node n = parsley::read<YAML>(R"(---
-
  - a
  -
    - b
    - c
- d)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0].is_list());
        REQUIRE(n[0].size() == 2);
        REQUIRE(n[0][0].as<StringView>() == "a");
        REQUIRE(n[0][1].is_list());
        REQUIRE(n[0][1].size() == 2);
        REQUIRE(n[0][1][0].as<StringView>() == "b");
        REQUIRE(n[0][1][1].as<StringView>() == "c");
        REQUIRE(n[1].as<StringView>() == "d");
    }

    SUBCASE("deserialize - root is mapping")
    {
        Node n = parsley::read<YAML>("foo: 1\nbar: 2");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 2);
        REQUIRE(n["foo"].as<int>() == 1);
        REQUIRE(n["bar"].as<int>() == 2);
    }

    SUBCASE("deserialize - mapping with null children")
    {
        Node n = parsley::read<YAML>("a:");
        
        REQUIRE(n.is_map());
        REQUIRE(n.size() == 1);
        REQUIRE(n["a"].is_null());

        n = parsley::read<YAML>("a: 1\nb:\nc: 2");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 3);
        REQUIRE(n["a"].as<int>() == 1);
        REQUIRE(n["b"].is_null());
        REQUIRE(n["c"].as<int>() == 2);

        n = parsley::read<YAML>("a: 1\nb: 2\nc:");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 3);
        REQUIRE(n["a"].as<int>() == 1);
        REQUIRE(n["b"].as<int>() == 2);
        REQUIRE(n["c"].is_null());
    }

    SUBCASE("deserialize - nested mappings")
    {
        Node n = parsley::read<YAML>(R"(---
a:
  b: 1
  c:
    d: 2
    e: 3
f: 4)");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 2);
        REQUIRE(n["a"].is_map());
        REQUIRE(n["a"].size() == 2);
        REQUIRE(n["a"]["b"].as<int>() == 1);
        REQUIRE(n["a"]["c"].is_map());
        REQUIRE(n["a"]["c"].size() == 2);
        REQUIRE(n["a"]["c"]["d"].as<int>() == 2);
        REQUIRE(n["a"]["c"]["e"].as<int>() == 3);
        REQUIRE(n["f"].as<int>() == 4);
    }

    SUBCASE("deserialize - sequence item with single inline key")
    {
        Node n = parsley::read<YAML>("- a: 1");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 1);
        REQUIRE(n[0].is_map());
        REQUIRE(n[0].size() == 1);
        REQUIRE(n[0]["a"].as<int>() == 1);
    }

    SUBCASE("deserialize - sequence of single-key mappings")
    {
        Node n = parsley::read<YAML>(R"(---
- a: 1
- b: 2
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0]["a"].as<int>() == 1);
        REQUIRE(n[1]["b"].as<int>() == 2);
    }

    SUBCASE("deserialize - sequence item with multi-key mapping")
    {
        Node n = parsley::read<YAML>(R"(---
- a: 1
  b: 2
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 1);
        REQUIRE(n[0].is_map());
        REQUIRE(n[0].size() == 2);
        REQUIRE(n[0]["a"].as<int>() == 1);
        REQUIRE(n[0]["b"].as<int>() == 2);
    }

    SUBCASE("deserialize - sequence of multi-key mappings")
    {
        Node n = parsley::read<YAML>(R"(---
- a: 1
  b: 2
- a: 3
  b: 4
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0]["a"].as<int>() == 1);
        REQUIRE(n[0]["b"].as<int>() == 2);
        REQUIRE(n[1]["a"].as<int>() == 3);
        REQUIRE(n[1]["b"].as<int>() == 4);
    }

    SUBCASE("deserialize - sequence item mapping with nested mapping value")
    {
        Node n = parsley::read<YAML>(R"(---
- name: ingest
  retries:
    max: 3
    delay: 5
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 1);
        REQUIRE(n[0]["name"].as<StringView>() == "ingest");
        REQUIRE(n[0]["retries"].is_map());
        REQUIRE(n[0]["retries"]["max"].as<int>() == 3);
        REQUIRE(n[0]["retries"]["delay"].as<int>() == 5);
    }

    SUBCASE("deserialize - mapping containing sequence of multi-key mappings")
    {
        Node n = parsley::read<YAML>(R"(---
workers:
  - name: ingest
    queue: events
  - name: cleanup
    queue: jobs
)");

        REQUIRE(n.is_map());
        REQUIRE(n["workers"].is_list());
        REQUIRE(n["workers"].size() == 2);
        REQUIRE(n["workers"][0]["name"].as<StringView>() == "ingest");
        REQUIRE(n["workers"][0]["queue"].as<StringView>() == "events");
        REQUIRE(n["workers"][1]["name"].as<StringView>() == "cleanup");
        REQUIRE(n["workers"][1]["queue"].as<StringView>() == "jobs");
    }

    SUBCASE("deserialize - mapping containing sequence of scalars, sibling key after")
    {
        Node n = parsley::read<YAML>(R"(---
items:
  - a
  - b
name: done
)");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 2);
        REQUIRE(n["items"].is_list());
        REQUIRE(n["items"].size() == 2);
        REQUIRE(n["name"].as<StringView>() == "done");
    }

    SUBCASE("deserialize - sequence of mappings, sibling key after")
    {
        Node n = parsley::read<YAML>(R"(---
workers:
  - name: ingest
    queue: events
name: done
)");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 2);
        REQUIRE(n["workers"][0]["name"].as<StringView>() == "ingest");
        REQUIRE(n["workers"][0]["queue"].as<StringView>() == "events");
        REQUIRE(n["name"].as<StringView>() == "done");
    }

    SUBCASE("deserialize - full document")
    {
        std::istringstream iss{R"(---
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
)"};

        Node n = parsley::read<YAML>(iss);

        REQUIRE(n["name"].as<StringView>() == "demo-app");
        REQUIRE(n["version"].as<StringView>() == "1.0");

        REQUIRE(n["server"]["host"].as<StringView>() == "localhost");
        REQUIRE(n["server"]["ports"][0].as<uint16_t>() == 8080);
        REQUIRE(n["server"]["ports"][1].as<uint16_t>() == 8081);
        REQUIRE(n["server"]["tls"]["enabled"].as<bool>() == true);
        REQUIRE(n["server"]["tls"]["cert"].as<StringView>() == "server.pem");

        REQUIRE(n["database"]["engine"].as<StringView>() == "postgres");
        REQUIRE(n["database"]["connection"]["host"].as<StringView>() == "db.local");
        REQUIRE(n["database"]["connection"]["user"].as<StringView>() == "app");
        REQUIRE(n["database"]["connection"]["options"]["pool_size"].as<uint16_t>() == 10);

        REQUIRE(n["workers"][0]["name"].as<StringView>() == "ingest");
        REQUIRE(n["workers"][0]["queue"].as<StringView>() == "events");
        auto val = n["workers"][0]["retries"]["max"].as<uint8_t>();
        REQUIRE(n["workers"][0]["retries"]["max"].as<uint8_t>() == 3);
        REQUIRE(n["workers"][0]["retries"]["delay"].as<uint8_t>() == 5);

        REQUIRE(n["workers"][1]["name"].as<StringView>() == "cleanup");
        REQUIRE(n["workers"][1]["queue"].as<StringView>() == "jobs");
        REQUIRE(n["workers"][1]["retries"]["max"].as<uint8_t>() == 2);
        REQUIRE(n["workers"][1]["retries"]["delay"].as<uint8_t>() == 10);
    }

    SUBCASE("deserialize - full document, from file")
    {
#if defined(__cpp_lib_filesystem)
        std::filesystem::path path = "tests/data/doc.yml";
#else
        std::string path = "tests/data/doc.yml";
#endif

        Node n = parsley::read_file<YAML>(path);

        REQUIRE(n["name"].as<StringView>() == "demo-app");
        REQUIRE(n["version"].as<StringView>() == "1.0");

        REQUIRE(n["server"]["host"].as<StringView>() == "localhost");
        REQUIRE(n["server"]["ports"][0].as<uint16_t>() == 8080);
        REQUIRE(n["server"]["ports"][1].as<uint16_t>() == 8081);
        REQUIRE(n["server"]["tls"]["enabled"].as<bool>() == true);
        REQUIRE(n["server"]["tls"]["cert"].as<StringView>() == "server.pem");

        REQUIRE(n["database"]["engine"].as<StringView>() == "postgres");
        REQUIRE(n["database"]["connection"]["host"].as<StringView>() == "db.local");
        REQUIRE(n["database"]["connection"]["user"].as<StringView>() == "app");
        REQUIRE(n["database"]["connection"]["options"]["pool_size"].as<uint16_t>() == 10);

        REQUIRE(n["workers"][0]["name"].as<StringView>() == "ingest");
        REQUIRE(n["workers"][0]["queue"].as<StringView>() == "events");
        auto val = n["workers"][0]["retries"]["max"].as<uint8_t>();
        REQUIRE(n["workers"][0]["retries"]["max"].as<uint8_t>() == 3);
        REQUIRE(n["workers"][0]["retries"]["delay"].as<uint8_t>() == 5);

        REQUIRE(n["workers"][1]["name"].as<StringView>() == "cleanup");
        REQUIRE(n["workers"][1]["queue"].as<StringView>() == "jobs");
        REQUIRE(n["workers"][1]["retries"]["max"].as<uint8_t>() == 2);
        REQUIRE(n["workers"][1]["retries"]["delay"].as<uint8_t>() == 10);
    }
}
