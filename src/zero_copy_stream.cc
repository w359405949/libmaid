#include "zero_copy_stream.h"

namespace maid {

RingInputStream::RingInputStream()
    :bytes_retired_(0)
{
}

RingInputStream::~RingInputStream()
{
    for (auto stream : streams_) {
        delete stream;
    }
    streams_.Clear();

    for (auto buffer : cleared_buffer_) {
        delete buffer;
    }

    cleared_buffer_.Clear();

    reading_stream_.clear();

    bytes_retired_ = 0;
}

std::string* RingInputStream::ReleaseCleared()
{
    std::string* buffer = nullptr;
    if (!cleared_buffer_.empty()) {
        buffer = *cleared_buffer_.cbegin();
        cleared_buffer_.erase(cleared_buffer_.cbegin());
        buffer->clear();
    } else {
        buffer = new std::string();
    }

    return buffer;
}

void RingInputStream::AddBuffer(std::string* buffer)
{
    auto* stream = new google::protobuf::io::ArrayInputStream(buffer->data(), buffer->size());
    reading_stream_[stream] = buffer;
    streams_.Add(stream);
}

bool RingInputStream::Next(const void** data, int* size)
{
    bool result = false;

    int i = 0;
    while (i < streams_.size()) {
        auto* stream = streams_.Get(i);

        result = stream->Next(data, size);
        if (result) {
            break;
        }

        bytes_retired_ += stream->ByteCount();

        std::string* buffer = reading_stream_[stream];
        GOOGLE_CHECK(buffer->size() == stream->ByteCount());

        cleared_buffer_.Add(buffer);
        reading_stream_.erase(stream);
        delete stream;

        i++;
    }

    streams_.erase(streams_.begin(), streams_.begin() + i);

    return result;
}

void RingInputStream::BackUp(int count)
{
    if (!streams_.empty()) {
        streams_.Get(0)->BackUp(count);
    } else {
        GOOGLE_LOG(DFATAL) << "Can't BackUp() after failed Next().";
    }
}

bool RingInputStream::Skip(int count)
{
    GOOGLE_CHECK(false) << "who called skip";

    bool result = false;
    int i = 0;
    while (i < streams_.size()) {
        auto* stream = streams_.Get(i);

        // Assume that ByteCount() can be used to find out how much we actually
        // skipped when Skip() fails.
        int64_t target_byte_count = stream->ByteCount() + count;
        result = stream->Skip(count);
        if (result) {
            break;
        }

        // Hit the end of the stream.  Figure out how many more bytes we still have
        // to skip.
        int64_t final_byte_count = streams_.Get(i)->ByteCount();
        GOOGLE_DCHECK_LT(final_byte_count, target_byte_count);

        bytes_retired_ += final_byte_count;

        std::string* buffer = reading_stream_[stream];

        cleared_buffer_.Add(buffer);
        reading_stream_.erase(stream);
        delete stream;

        i++;
    }

    streams_.erase(streams_.begin(), streams_.begin() + i);

    return result;
}

int64_t RingInputStream::ByteCount() const
{
    if (streams_.empty()) {
        return bytes_retired_;
    } else {
        return bytes_retired_ + (*streams_.begin())->ByteCount();
    }
}

}
