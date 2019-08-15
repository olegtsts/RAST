#include <iostream>

#include "message_passing_tree.h"

class MessageProcessorA;

class MessageProcessorB;

class DoubleMessage : public MessageBase {
public:
    DoubleMessage(const double a)
    : a(a)
    {}
    double a;
};
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
    void Receive(const ReceivingFrom<MessageProcessorB>&, const DoubleMessage& value, const Sender& sender) {
        std::cout << "MessageProcessorA: I have got double value from MessageProcessorB " << value.a << std::endl;
        sender.template Send<MessageProcessorB>(std::make_unique<IntMessage>(2 + static_cast<int>(value.a)));
    }
    virtual ~MessageProcessorA(){}
};

class MessageProcessorB : public MessageProcessorBase {
public:
    using MessageProcessorBase::MessageProcessorBase;

    template <typename Sender>
    void Ping(const Sender& sender) {
        std::cout << "Called Ping to MessageProcessorB\n";
        sender.template Send<MessageProcessorA>(std::make_unique<DoubleMessage>(1.0));
    }

    template <typename Sender>
    void Receive(const ReceivingFrom<MessageProcessorA>&, const IntMessage& value, const Sender& sender) {
        std::cout << "MessageProcessorB: I have got int value from MessageProcessorA " << value.a << std::endl;
        sender.template Send<MessageProcessorA>(std::make_unique<DoubleMessage>(3 + value.a));
    }
    template <typename Sender>
    void Receive(const ReceivingFrom<MessageProcessorA>&, const DoubleMessage& value, const Sender&) {
        std::cout << "MessageProcessorB: I have got double value from MessageProcessorA"  << value.a << std::endl;
    }
    virtual ~MessageProcessorB() {}
};

int main(){
    MessagePassingTree<
        Edge<MessageProcessorA, MessageProcessorB, IntMessage>,
        Edge<MessageProcessorB, MessageProcessorA, DoubleMessage>> message_passing_tree;

    message_passing_tree.GetMessageProcessorProxy(0)->Ping();
    for (size_t i = 0; i < 10; ++i) {
        message_passing_tree.GetEdgeProxy(0)->NotifyAboutMessage();
        message_passing_tree.GetEdgeProxy(1)->NotifyAboutMessage();
    }
    message_passing_tree.OutputDestPipes();
    return 0;
}
