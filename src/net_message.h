#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

namespace net
{

// Message header is sent at the start of all messages.
// It contains the id of the message and the size of the message.
// Size of this struct is constant (8 bytes).
template <typename T>
struct message_header
{
    T id{};
    uint32_t size = 0; // size_t is not used because it is platform dependent.
};

template <typename T>
struct message
{
    message_header<T> header{};
    std::vector<uint8_t> body;

    // Returns the size of the entire message in bytes.
    size_t size() const { return sizeof(message_header<T>) + body.size(); }

    // Overload the << operator for std::cout compatibility.
    friend std::ostream& operator<<(std::ostream& os, const message<T>& msg)
    {
        os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
        return os;
    }

    // Pushes any POD-like data into the message buffer.
    // POD = Plain Old Data. It is a C++ term.
    // It is a data type that is represented in the same way in the memory as it is in the source code.
    // Advantages of this method:
    // 1. Automatically allocates memory for the data being pushed.
    // Disadvantages of this method:
    // 1. Performance overhead. It is not the most efficient way to send data.
    template <typename DataType>
    friend message<T>& operator<<(message<T>& msg, const DataType& data)
    {
        // Check that the type of the data being pushed is trivially copyable.
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

        // Cache the current size of the vector.
        size_t i = msg.body.size();

        // Resize the vector by the size of the data being pushed.
        msg.body.resize(msg.body.size() + sizeof(DataType));

        // Copy the data into the newly allocated vector space.
        std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

        // Recalculate the message size.
        msg.header.size = msg.size();

        // Return the target message so it can be "chained".
        return msg;
    }

    // Pulls any POD-like data from the message buffer.
    // POD = Plain Old Data. It is a C++ term.
    // It is a data type that is represented in the same way in the memory as it is in the source code.
    template <typename DataType>
    friend message<T>& operator>>(message<T>& msg, DataType& data)
    {
        // Check that the type of the data being pushed is trivially copyable.
        static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

        // Cache the location towards the end of the vector where the pulled data starts.
        size_t i = msg.body.size() - sizeof(DataType);

        // Copy the data from the vector into the target data location.
        std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

        // Shrink the vector to remove read bytes, and reset the message size.
        msg.body.resize(i);

        // Recalculate the message size.
        msg.header.size = msg.size();

        // Return the target message so it can be "chained".
        return msg;
    }
};

} // namespace net
