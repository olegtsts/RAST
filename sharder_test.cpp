#include "message_passing_tree.h"
#include "sharder.h"
#include <thread>
#include <chrono>

class IntMessage: public MessageBase {
public:
    IntMessage(const int a)
    : a(a)
    {}
    int a;
};

class MessageProcessorA : public MessageProcessorBase {
public:
    using MessageProcessorBase::MessageProcessorBase;

    template <typename Sender>
    bool Ping(const Sender& sender) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    }
};

class MessageProcessorB : public MessageProcessorBase {
public:
    using MessageProcessorBase::MessageProcessorBase;

    template <typename Sender>
    bool Ping(const Sender& sender) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return true;
    }
};

class MessageProcessorC : public MessageProcessorBase {
public:
    using MessageProcessorBase::MessageProcessorBase;

    template <typename Sender>
    bool Ping(const Sender& sender) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return true;
    }

    template <typename Sender>
    void Receive(const ReceivingFrom<MessageProcessorA>&, const IntMessage& value, const Sender& sender) {
    }

    template <typename Sender>
    void Receive(const ReceivingFrom<MessageProcessorB>&, const IntMessage& value, const Sender& sender) {
    }
};

int main() {
    DynamicallyShardedMessagePassingPool<
        Edge<MessageProcessorA, MessageProcessorC, IntMessage>,
        Edge<MessageProcessorB, MessageProcessorC, IntMessage>> dsmpp;
    dsmpp.Run();
    return 0;
}

