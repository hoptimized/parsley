# Parsley

A fast C++ library for parsing and serializing JSON, YAML, and more.

> #### Warning
> Parsley is currently in pre-release development and should be considered experimental.<br/>
> Some important features are still missing. See the [Roadmap](#roadmap) for details.

## Usage

Parsley uses a `Node` type as its data model. Nodes can represent scalars, sequences, and mappings, and can be read from or written to supported serialization formats.

### Writing Structured Data

Create a document by assigning values directly to nodes:

```C++
parsley::Node n;

// Scalar values
n["name"] = "demo-app";
n["version"] = "1.0";
n["debug"] = true;

// Nested maps
n["server"]["host"] = "localhost";
n["server"]["tls"]["enabled"] = true;
n["server"]["tls"]["cert"] = "server.pem";

// Sequences
n["server"]["ports"].push_back(8080);
n["server"]["ports"].push_back(8081);

// More nesting
n["database"]["engine"] = "postgres";
n["database"]["connection"]["host"] = "db.local";
n["database"]["connection"]["options"]["pool_size"] = 10;

// Implicitly created sequences
n["workers"][0]["name"] = "ingest";
n["workers"][0]["queue"] = "events";
n["workers"][0]["retries"]["max"] = 3;
n["workers"][1]["name"] = "cleanup";
n["workers"][1]["queue"] = "jobs";
n["workers"][1]["retries"]["max"] = 2;

// Serialize the same data to different formats
parsley::write_file<YAML>(n, "data.yml");
parsley::write_file<JSON>(n, "data.json");
```

### Reading Structured Data

Read a document into a `Node` and access its values using the same interface:

```C++
parsley::Node node = parsley::read_file<parsley::YAML>("data.yml");

std::string name = node["name"].as<std::string>();
std::string version = node["version"].as<std::string>();

std::string host = node["server"]["host"].as<std::string>();
int first_port = node["server"]["ports"][0].as<int>();
int second_port = node["server"]["ports"][1].as<int>();

bool tls_enabled = node["server"]["tls"]["enabled"].as<bool>();

int pool_size = node["database"]["connection"]["options"]["pool_size"].as<int>();
```

`as<T>()` provides a convenient way to convert a node to a C++ value.

### Custom Data Types

Parsley can serialize user-defined C++ types by providing a `parsley::Transfer<T>` specialization.

For example, consider a customer with one or more addresses:

```C++
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
```

Define how each type is read from and written to a `Node`:

```C++
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
```

Once a transfer is defined, custom types can be serialized and deserialized just like built-in types:

```C++
Customer customer{
    "John",
    "Doe",
    {
        { "1600 Pennsylvania Avenue NW", 20500, "USA" },
        { "350 Fifth Avenue", 10118, "USA" }
    }
};

// Serialize the custom type to YAML
std::string yaml = parsley::write<YAML>(customer);
```

The resulting YAML is:

```YAML
---
first_name: John
last_name: Doe
addresses:
  - street: 1600 Pennsylvania Avenue NW
    zip: 20500
    country: USA
  - street: 350 Fifth Avenue
    zip: 10118
    country: USA
```

```C++
// Deserialize it back into the custom type
Customer parsed = parsley::read<YAML>(yaml).as<Customer>();
```

## Documentation

API documentation is available in the [Doxygen documentation](https://hoptimized.github.io/parsley/).

## Roadmap

Parsley is still under active development. Planned work includes:

### YAML

Missing YAML features to be added:
- Inline comments, such as `key: value # comment`
- Single-quoted scalars
- Double-quoted scalars
- Escape sequences
- Multi-line scalars
- Block scalars (`|` and `>`)
- Flow style collections (`{...}` and `[...]`)
- Tags
- Anchors and aliases

### JSON

JSON parsing.

### Data Model

The current data model uses `unique_ptr`-based ownership between nodes. This blocks the implementation of some YAML features, particularly anchors and aliases, and also results in a less efficient memory layout.

The data model will be reworked internally to address these issues without changing the public API.
