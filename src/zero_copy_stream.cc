#include "zero_copy_stream.h"

namespace maid {

RingInputStream::RingInputStream()
    :bytes_retired_(0)
{
}

RingInputStream::~RingInputStream()
{
}

std::string* RingInputStream::ReleaseCleared()
{
    if (cleared_buffer_.empty()) {
        return new std::string();
    }

    std::string* buffer = *cleared_buffer_.cbegin();
    cleared_buffer_.RemoveLast();

    buffer->clear();
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
        result = streams_.Get(i)->Next(data, size);
        if (result) {
            break;
        }

        // That stream is done.  Advance to the next one.
        bytes_retired_ += (*streams_.begin())->ByteCount();

        // reuse cleared buffer
        std::string* buffer = reading_stream_[*streams_.begin()];
        cleared_buffer_.Add(buffer);

        i++;
    }

    int num = i;
    while(--i >= 0) {
        auto* stream = streams_.Get(i);
        std::string* buffer = reading_stream_[stream];
        cleared_buffer_.Add(buffer);

        GOOGLE_CHECK(buffer->size() == stream->ByteCount());

        reading_stream_.erase(stream);
        delete stream;
    }

    streams_.erase(streams_.begin(), streams_.begin() + num);

    return result;
}

void RingInputStream::BackUp(int count)
{
    if (!streams_.empty()) {
        (*streams_.begin())->BackUp(count);
    } else {
        GOOGLE_LOG(DFATAL) << "Can't BackUp() after failed Next().";
    }
}

bool RingInputStream::Skip(int count)
{
    bool result = false;
    int i = 0;
    while (i < streams_.size()) {
        // Assume that ByteCount() can be used to find out how much we actually
        // skipped when Skip() fails.
        int64_t target_byte_count = streams_.Get(i)->ByteCount() + count;
        result = streams_.Get(i)->Skip(count);
        if (result) {
            break;
        }

        // Hit the end of the stream.  Figure out how many more bytes we still have
        // to skip.
        int64_t final_byte_count = streams_.Get(i)->ByteCount();
        GOOGLE_DCHECK_LT(final_byte_count, target_byte_count);

        // That stream is done.  Advance to the next one.
        bytes_retired_ += final_byte_count;

        i++;
    }

    int num = i;
    while (--i >= 0) {
        auto* stream = streams_.Get(i);
        std::string* buffer = reading_stream_[stream];

        cleared_buffer_.Add(buffer);
        reading_stream_.erase(stream);
        delete stream;
    }

    streams_.erase(streams_.begin(), streams_.begin() + num);

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
