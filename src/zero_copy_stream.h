#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/map.h>

namespace maid {

class RingInputStream : public google::protobuf::io::ZeroCopyInputStream
{
public:
    RingInputStream();
    ~RingInputStream();

    std::string* ReleaseCleared();
    void AddBuffer(std::string* buffer);

public:
    // implements ZeroCopyInputStream ----------------------------------
    bool Next(const void** data, int* size);
    void BackUp(int count);
    bool Skip(int count);
    int64_t ByteCount() const;

private:
    google::protobuf::RepeatedField<google::protobuf::io::ArrayInputStream*> streams_; // order
    google::protobuf::Map<google::protobuf::io::ArrayInputStream*, std::string*> reading_stream_;
    google::protobuf::RepeatedField<std::string*> cleared_buffer_;

    int64_t bytes_retired_;  // Bytes read from previous streams.
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RingInputStream);
};

}
