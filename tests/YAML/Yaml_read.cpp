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
        REQUIRE(n == "test");
    }

    SUBCASE("deserialize - root is sequence")
    {
        Node n = parsley::read<YAML>("- foo\n- bar");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0] == "foo");
        REQUIRE(n[1] == "bar");
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
        REQUIRE(n[0] == "a");
        REQUIRE(n[1].is_null());
        REQUIRE(n[2] == "b");

        n = parsley::read<YAML>("- a\n- b\n-");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 3);
        REQUIRE(n[0] == "a");
        REQUIRE(n[1] == "b");
        REQUIRE(n[2].is_null());
    }

    SUBCASE("deserialize - sequence of scalars")
    {
        Node n = parsley::read<YAML>("- a\n- b");
        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
    }

    /*SUBCASE("deserialize - nested sequences")
    {
        Node n = parsley::read<YAML>(R"(---
-
  - a
  -
    - b
    - c
- - d
  - e
- - - f
- g
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 4);
        REQUIRE(n[0].is_list());
        REQUIRE(n[0].size() == 2);
        REQUIRE(n[0][0] == "a");
        REQUIRE(n[0][1].is_list());
        REQUIRE(n[0][1].size() == 2);
        REQUIRE(n[0][1][0] == "b");
        REQUIRE(n[0][1][1] == "c");
        REQUIRE(n[1][0] == "d");
        REQUIRE(n[1][1] == "e");
        REQUIRE(n[2][0][0] == "f");
        REQUIRE(n[3] == "g");
    }*/

    SUBCASE("deserialize - root is mapping")
    {
        Node n = parsley::read<YAML>("foo: 1\nbar: 2");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 2);
        REQUIRE(n["foo"] == 1);
        REQUIRE(n["bar"] == 2);
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
        REQUIRE(n["a"] == 1);
        REQUIRE(n["b"].is_null());
        REQUIRE(n["c"] == 2);

        n = parsley::read<YAML>("a: 1\nb: 2\nc:");

        REQUIRE(n.is_map());
        REQUIRE(n.size() == 3);
        REQUIRE(n["a"] == 1);
        REQUIRE(n["b"] == 2);
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
        REQUIRE(n["a"]["b"] == 1);
        REQUIRE(n["a"]["c"].is_map());
        REQUIRE(n["a"]["c"].size() == 2);
        REQUIRE(n["a"]["c"]["d"] == 2);
        REQUIRE(n["a"]["c"]["e"] == 3);
        REQUIRE(n["f"] == 4);
    }

    SUBCASE("deserialize - sequence item with single inline key")
    {
        Node n = parsley::read<YAML>("- a: 1");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 1);
        REQUIRE(n[0].is_map());
        REQUIRE(n[0].size() == 1);
        REQUIRE(n[0]["a"] == 1);
    }

    SUBCASE("deserialize - sequence of single-key mappings")
    {
        Node n = parsley::read<YAML>(R"(---
- a: 1
- b: 2
)");

        REQUIRE(n.is_list());
        REQUIRE(n.size() == 2);
        REQUIRE(n[0]["a"] == 1);
        REQUIRE(n[1]["b"] == 2);
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
        REQUIRE(n[0]["a"] == 1);
        REQUIRE(n[0]["b"] == 2);
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
        REQUIRE(n[0]["a"] == 1);
        REQUIRE(n[0]["b"] == 2);
        REQUIRE(n[1]["a"] == 3);
        REQUIRE(n[1]["b"] == 4);
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
        REQUIRE(n[0]["name"] == "ingest");
        REQUIRE(n[0]["retries"].is_map());
        REQUIRE(n[0]["retries"]["max"] == 3);
        REQUIRE(n[0]["retries"]["delay"] == 5);
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
        REQUIRE(n["workers"][0]["name"] == "ingest");
        REQUIRE(n["workers"][0]["queue"] == "events");
        REQUIRE(n["workers"][1]["name"] == "cleanup");
        REQUIRE(n["workers"][1]["queue"] == "jobs");
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
        REQUIRE(n["name"] == "done");
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
        REQUIRE(n["workers"][0]["name"] == "ingest");
        REQUIRE(n["workers"][0]["queue"] == "events");
        REQUIRE(n["name"] == "done");
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

        REQUIRE(n["name"] == "demo-app");
        REQUIRE(n["version"] == "1.0");

        REQUIRE(n["server"]["host"] == "localhost");
        REQUIRE(n["server"]["ports"][0] == 8080);
        REQUIRE(n["server"]["ports"][1] == 8081);
        REQUIRE(n["server"]["tls"]["enabled"] == true);
        REQUIRE(n["server"]["tls"]["cert"] == "server.pem");

        REQUIRE(n["database"]["engine"] == "postgres");
        REQUIRE(n["database"]["connection"]["host"] == "db.local");
        REQUIRE(n["database"]["connection"]["user"] == "app");
        REQUIRE(n["database"]["connection"]["options"]["pool_size"] == 10);

        REQUIRE(n["workers"][0]["name"] == "ingest");
        REQUIRE(n["workers"][0]["queue"] == "events");
        REQUIRE(n["workers"][0]["retries"]["max"] == 3);
        REQUIRE(n["workers"][0]["retries"]["delay"] == 5);

        REQUIRE(n["workers"][1]["name"] == "cleanup");
        REQUIRE(n["workers"][1]["queue"] == "jobs");
        REQUIRE(n["workers"][1]["retries"]["max"] == 2);
        REQUIRE(n["workers"][1]["retries"]["delay"] == 10);
    }

    SUBCASE("deserialize - full document, from file")
    {
#if defined(__cpp_lib_filesystem)
        std::filesystem::path path = "tests/data/doc.yml";
#else
        std::string path = "tests/data/doc.yml";
#endif

        Node n = parsley::read_file<YAML>(path);

        REQUIRE(n["name"] == "demo-app");
        REQUIRE(n["version"] == "1.0");

        REQUIRE(n["server"]["host"] == "localhost");
        REQUIRE(n["server"]["ports"][0] == 8080);
        REQUIRE(n["server"]["ports"][1] == 8081);
        REQUIRE(n["server"]["tls"]["enabled"] == true);
        REQUIRE(n["server"]["tls"]["cert"] == "server.pem");

        REQUIRE(n["database"]["engine"] == "postgres");
        REQUIRE(n["database"]["connection"]["host"] == "db.local");
        REQUIRE(n["database"]["connection"]["user"] == "app");
        REQUIRE(n["database"]["connection"]["options"]["pool_size"] == 10);

        REQUIRE(n["workers"][0]["name"] == "ingest");
        REQUIRE(n["workers"][0]["queue"] == "events");
        REQUIRE(n["workers"][0]["retries"]["max"] == 3);
        REQUIRE(n["workers"][0]["retries"]["delay"] == 5);

        REQUIRE(n["workers"][1]["name"] == "cleanup");
        REQUIRE(n["workers"][1]["queue"] == "jobs");
        REQUIRE(n["workers"][1]["retries"]["max"] == 2);
        REQUIRE(n["workers"][1]["retries"]["delay"] == 10);
    }

    SUBCASE("deserialize - mixed newline characters")
    {
        Node n1 = parsley::read<YAML>("---\na: 1\r\nb: 2\rc: 3");
        REQUIRE(n1.is_map());
        REQUIRE(n1.size() == 3);
        REQUIRE(n1["a"] == 1);
        REQUIRE(n1["b"] == 2);
        REQUIRE(n1["c"] == 3);

        std::istringstream stream { "---\na: 1\r\nb: 2\rc: 3" };
        Node n2 = parsley::read<YAML>(stream);
        REQUIRE(n2.is_map());
        REQUIRE(n2.size() == 3);
        REQUIRE(n2["a"] == 1);
        REQUIRE(n2["b"] == 2);
        REQUIRE(n2["c"] == 3);
    }

    SUBCASE("deserialize - end marker")
    {
        Node n1 = parsley::read<YAML>("---\na: 1\nb: 2\n...\nc: 3"); // shouldn't read "c: 3"
        REQUIRE(n1.is_map());
        REQUIRE(n1.size() == 2);
        REQUIRE(n1["a"] == 1);
        REQUIRE(n1["b"] == 2);
    }

    SUBCASE("deserialize - plain multi-line scalar")
    {
        REQUIRE(parsley::read<YAML>(R"(---
this is a
  folded value that
  keeps going)") == "this is a folded value that keeps going");

        Node map = parsley::read<YAML>(R"(---
first: this is a
  folded first item
second: this is a
  folded second item)");
        REQUIRE(map["first"] == "this is a folded first item");
        REQUIRE(map["second"] == "this is a folded second item");

        Node seq = parsley::read<YAML>(R"(---
- this is a
  folded first item
- this is a
  folded second item)");
        REQUIRE(seq.size() == 2);
        REQUIRE(seq[0] == "this is a folded first item");
        REQUIRE(seq[1] == "this is a folded second item");
    }

    SUBCASE("deserialize - plain multi-line scalar with paragraph break")
    {
        // blank line mid-fold -> joined with '\n' instead of ' '
        Node map = parsley::read<YAML>(R"(---
summary: this is a
  folded value that
  keeps going

  and this is a new paragraph)");

        REQUIRE(map["summary"] ==
            "this is a folded value that keeps going\nand this is a new paragraph");
    }

    SUBCASE("deserialize - sequence scalar fold does not swallow next mapping item")
    {
        Node seq = parsley::read<YAML>(R"(---
- this is a
  folded scalar
- key: value)");

        REQUIRE(seq.size() == 2);
        REQUIRE(seq[0] == "this is a folded scalar");
        REQUIRE(seq[1]["key"] == "value");
    }

    SUBCASE("deserialize - quoted scalars")
    {
        Node n1 = parsley::read<YAML>(R"("quoted key": value)");
        REQUIRE(n1["quoted key"] == "value");

        Node n2 = parsley::read<YAML>(R"('single quoted': value)");
        REQUIRE(n2["single quoted"] == "value");

        Node n3 = parsley::read<YAML>(R"(plain: "quoted value: with a colon inside")");
        REQUIRE(n3["plain"] == "quoted value: with a colon inside");

        Node n4 = parsley::read<YAML>(R"(- "quoted item")");
        REQUIRE(n4[0] == "quoted item");

        Node n5 = parsley::read<YAML>(R"("has a : colon inside": still works.)");
        REQUIRE(n5["has a : colon inside"] == "still works.");
    }

    SUBCASE("deserialize - escape sequences")
    {
        Node n = parsley::read<YAML>(R"(
"\0": "\0"
"\a": "\a"
"\b": "\b"
"\t": "\t"
"\n": "\n"
"\v": "\v"
"\f": "\f"
"\r": "\r"
"\e": "\e"
"\"": "\""
"\/": "\/"
"\\": "\\"
"\N": "\N"
"\_": "\_"
"\L": "\L"
"\P": "\P"
"\x41\x42\x43": "\x41\x42\x43"
"\u4E2D": "\u4E2D"
"\U0001F60D": "\U0001F60D"
)");

        //REQUIRE(n["\0"] == "\0"); // TODO: this is a headache for later
        REQUIRE(n["\a"] == "\a");
        REQUIRE(n["\b"] == "\b");
        REQUIRE(n["\t"] == "\t");
        REQUIRE(n["\n"] == "\n");
        REQUIRE(n["\v"] == "\v");
        REQUIRE(n["\f"] == "\f");
        REQUIRE(n["\r"] == "\r");
        REQUIRE(n["\e"] == "\e");
        REQUIRE(n["\""] == "\"");
        REQUIRE(n["/"] == "/");
        REQUIRE(n["\\"] == "\\");
        REQUIRE(n["\u0085"] == "\u0085");
        REQUIRE(n["\u00A0"] == "\u00A0");
        REQUIRE(n["\u2028"] == "\u2028");
        REQUIRE(n["\u2029"] == "\u2029");
        REQUIRE(n["ABC"] == "ABC");
        REQUIRE(n["中"] == "中");
        REQUIRE(n["😍"] == "😍");
    }

    SUBCASE("deserialize - multi-line quoted scalar")
    {
        Node n = parsley::read<YAML>(R"(
"Multi-line
quoted scalar"
)");

        REQUIRE(n == "Multi-line quoted scalar");
    }
}
