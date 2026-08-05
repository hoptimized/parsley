#pragma once

#include <cstdlib>
#include <cstring>

namespace parsley
{
    class StringInputCursor
    {
    public:
        explicit StringInputCursor(StringView data) : data_(data)
        {
        }

        bool get_char(char& c)
        {
            if (pos_ >= data_.size())
                return false;

            c = data_[pos_++];
            return true;
        }

        bool get_line(StringView& line)
        {
            if (pos_ >= data_.size())
                return false;

            size_t start = pos_;
            bool found_newline = false;
            while (pos_ < data_.size())
            {
                if (data_[pos_] == '\n' || data_[pos_] == '\r')
                {
                    found_newline = true;
                    break;
                }
                ++pos_;
            }

            line = data_.substr(start, pos_ - start);

            // Consume '\n' or '\r\n', excluded from the returned line
            if (found_newline)
                consume_newline();

            return found_newline || line.size() > 0;
        }

        bool eof() const { return pos_ >= data_.size(); }

    private:
        void consume_newline()
        {
            char new_line_char = data_[pos_++]; // consume '\n' or '\r'
            if (new_line_char == '\r' && !eof() && data_[pos_] == '\n')
                ++pos_; // consume second char of "\r\n"
        }

        StringView data_;
        size_t pos_ = 0;
    };

    template <size_t BUFFER_SIZE = 32 * 1024>
    class BasicStreamInputCursor
    {
    public:
        explicit BasicStreamInputCursor(std::istream& in) : in_(&in)
        {
        }

        ~BasicStreamInputCursor()
        {
            if (buffer_ != nullptr)
                std::free(buffer_);
        }

        bool get_char(char& out_char)
        {
            start_read();

            // Ensure the buffer contains at least one more byte
            if (!ensure_buffer())
                return false;

            // Get the char at the current position
            out_char = buffer_[pos_in_buffer(read_end_pos_)];
            
            // Advance by one byte
            read_end_pos_++;

            return true;
        }

        bool get_line(StringView& line)
        {
            start_read();

            if (eof())
                return false;
            
            bool found_newline = false;

            // Advance the read window until we find a newline or fail to advance in the buffer.
            while (ensure_buffer())
            {
                char c = buffer_[pos_in_buffer(read_end_pos_)];
                if (c == '\n' || c == '\r')
                {
                    found_newline = true;
                    break;
                }
                
                ++read_end_pos_;
            }
            
            // Extract the current line into a StringView
            char* read_window_start = buffer_ + pos_in_buffer(read_start_pos_);
            size_t read_window_length = read_end_pos_ - read_start_pos_;
            line = StringView{ read_window_start, read_window_length };

            // Consume '\n' or '\r\n', excluded from the returned line
            if (found_newline)
                consume_newline();

            return found_newline || line.size() > 0;
        }

        bool eof() const
        {
            const size_t end_of_content = buffer_start_pos_ + buffer_size_;
            return stream_exhausted_ && read_end_pos_ == end_of_content; 
        }

    private:
        void start_read()
        {
            // Starting a new read invalidates any data previously handed to callers.
            // The new read window opens at the end of the old window.
            read_start_pos_ = read_end_pos_;
        }

        // Ensures the buffer has at least one more byte to read. May fill the buffer.
        bool ensure_buffer()
        {            
            if (read_end_pos_ < buffer_start_pos_ + buffer_size_)
                return true; // There's still more bytes in the buffer

            if (!stream_exhausted_)
                return fill_buffer(); // Try to fill the buffer from the stream

            return false; // Unable to provide more bytes to read
        }

        // Slurps more bytes from the stream to fill the buffer.
        bool fill_buffer()
        {
            const size_t protected_chars = get_protected_chars();
            if (protected_chars > 0)
            {
                // Move the protected chars at the end of the buffer (needed for the current line)
                // to the beginning of the buffer to preserve them.
                std::memmove(buffer_, buffer_ + buffer_capacity_ - protected_chars, protected_chars);
            }

            // If we read a line that exceeds the full buffer size, we must grow the buffer.
            const bool full_buffer_protected = protected_chars == buffer_capacity_;
            if (full_buffer_protected)
                grow_buffer();

            buffer_start_pos_ = read_end_pos_ - protected_chars;

            // Slurp bytes from the stream until the buffer is full or the stream ends
            if (!in_->read(buffer_ + protected_chars, buffer_capacity_ - protected_chars))
                stream_exhausted_ = true;
            
            size_t read_count = in_->gcount();
            buffer_size_ = read_count + protected_chars;

            return read_count > 0;
        }

        void grow_buffer()
        {
            size_t old_capacity = buffer_capacity_;
            char* old_buffer = buffer_;

            // Grow buffer by BUFFER_SIZE (n + 1)
            size_t new_capacity = BUFFER_SIZE * ((old_capacity / BUFFER_SIZE) + 1);
            char* new_buffer = static_cast<char*>(std::malloc(new_capacity));

            if (old_buffer != nullptr)
            {
                // Migrate data from the old buffer
                std::memcpy(new_buffer, buffer_, buffer_capacity_);
                std::free(buffer_);
            }

            buffer_ = new_buffer;
            buffer_capacity_ = new_capacity;
        }

        void consume_newline()
        {
            // consume '\n' or '\r'
            char c = buffer_[pos_in_buffer(read_end_pos_++)];

            if (c == '\r' && get_char(c) && c != '\n') // consume the next char
                --read_end_pos_; // next char wasn't part of a newline sequence, revert
        }

        size_t pos_in_buffer(size_t pos_in_stream)
        {
            return pos_in_stream - buffer_start_pos_;
        }

        // Returns the number of chars in our current read window (i.e. current line).
        // When we fill the buffer with new data, these chars must be preserved.
        size_t get_protected_chars()
        {
            return read_end_pos_ - read_start_pos_;
        }

        std::istream* in_;
        char* buffer_ = nullptr;
        bool stream_exhausted_ = false;
        size_t buffer_size_ = 0; // number of chars written into the buffer
        size_t buffer_capacity_ = 0; // total capacity of the buffer, includes uninitialized chars
        size_t buffer_start_pos_ = 0; // buffers starts at this position in the stream
        size_t read_start_pos_ = 0; // position in the stream where our current read begins
        size_t read_end_pos_ = 0; // position in the stream where our current read ends
    };

    class StreamInputCursor : public BasicStreamInputCursor<>
    {
    public:
        explicit StreamInputCursor(std::istream& in) : BasicStreamInputCursor(in) {}
    };
}
