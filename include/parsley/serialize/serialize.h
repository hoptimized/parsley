#pragma once

#include "parsley/core/InputCursor.h"
#include "parsley/core/OutputCursor.h"


#if defined(__cpp_lib_filesystem)
#include <filesystem>
#endif
#include <fstream>
#include <istream>
#include <string>

namespace parsley
{
#if defined(__cpp_lib_filesystem)
    using path_type = std::filesystem::path;
#else
    using path_type = std::string;
#endif

    template <typename Format>
    static Node read(StringView sv, typename Format::DeserializerConfig config = {})
    {
        using Deserializer = typename Format::template Deserializer<StringInputCursor>;
        StringInputCursor cursor{ sv };
        return Deserializer{ std::move(config) }.read(cursor);
    }

    template <typename Format>
    static Node read(std::istream& stream, typename Format::DeserializerConfig config = {})
    {
        using Deserializer = typename Format::template Deserializer<StreamInputCursor>;
        StreamInputCursor cursor{ stream };
        return Deserializer{ std::move(config) }.read(cursor);
    }

    template <typename Format>
    static Node read_file(const path_type& path, typename Format::DeserializerConfig config = {})
    {
        std::ifstream stream{ path };
        return read<Format>(stream, std::move(config));
    }

#if !defined(__cpp_lib_filesystem)
    template <typename Format>
    static Node read_file(const char* path, typename Format::DeserializerConfig config = {})
    {
        std::ifstream stream{ path };
        return read<Format>(stream, std::move(config));
    }
#endif

    template <typename Format>
    static std::string write(const Node& node, typename Format::SerializerConfig config = {})
    {
        using Serializer = typename Format::template Serializer<StringOutputCursor>;
        std::string res;
        StringOutputCursor cursor{ res };
        Serializer{ std::move(config) }.write(node, cursor);
        return res;
    }

    template <typename Format>
    static void write(const Node& node, std::ostream& stream, typename Format::SerializerConfig config = {})
    {
        using Serializer = typename Format::template Serializer<StreamOutputCursor>;
        StreamOutputCursor cursor{ stream };
        Serializer{ std::move(config) }.write(node, cursor);
    }

    template <typename Format>
    static void write_file(const Node& node, const path_type& path, typename Format::SerializerConfig config = {})
    {
        std::ofstream stream{ path };
        write<Format>(node, stream, std::move(config));
    }
}
