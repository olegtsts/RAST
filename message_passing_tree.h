#pragma once

#include <memory>
#include <algorithm>
#include <condition_variable>

#include "type_specifier.h"
#include "types.h"
#include "queue.h"

template <typename From, typename To, typename Message>
class Edge{};

class MessageProcessorBase {
public:
    template <typename Sender>
    void Ping(const Sender&) {}

    virtual ~MessageProcessorBase() noexcept {}
};

class MessageBase {
public:
    virtual ~MessageBase() {}
};

template <typename ... Args>
class Piper {
protected:
    template <typename GlobalPiper>
    class EdgeProxy {
    public:
        EdgeProxy(GlobalPiper& piper) noexcept
        : piper(piper)
        {}

        virtual void SetConditionVariable (std::condition_variable_any *) const noexcept = 0;
        virtual void NotifyAboutMessage() const = 0;
        virtual int GetFromIndex() const noexcept = 0;
        virtual int GetToIndex() const noexcept = 0;
        virtual ~EdgeProxy() noexcept {}
    protected:
        GlobalPiper& piper;
    };

    template <typename GlobalPiper>
    class MessageProcessorProxy {
    public:
        MessageProcessorProxy(GlobalPiper& piper, const int message_processor_index) noexcept
        : piper(piper)
        , message_processor_index(message_processor_index)
        {}

        template <typename MP>
        MP& GetMessageProcessor() const noexcept {
            return *dynamic_cast<MP*>(piper.message_processors[message_processor_index].get());
        }

        virtual void Ping() const = 0;
        virtual ~MessageProcessorProxy() {}
    protected:
        GlobalPiper& piper;
        int message_processor_index;
    };

    int max_message_processor_index;
    int max_edge_index;
    Vector<Vector<int>> dest_pipes;
    Vector<LockFreeQueue<MessageBase>> queues;
    Vector<std::unique_ptr<MessageProcessorBase>> message_processors;
    Vector<std::condition_variable_any *> notify_condition_variables;
public:
    Piper() noexcept
    : max_message_processor_index(0)
    , max_edge_index(0)
    {
    }

    template <typename MP>
    int GetMessageProcessorIndexImpl(const TypeSpecifier<MP>&) noexcept {
        return -1;
    }

    template <typename GlobalPiper>
    void FillEdgeProxysImpl(GlobalPiper&, Vector<std::unique_ptr<Piper::EdgeProxy<GlobalPiper>>>&) {}
    void GetEdgeIndexImpl() {}
    template <typename GlobalPiper>
    void AddMessageProcessorsImpl(GlobalPiper& ,
                                  Vector<std::unique_ptr<Piper::MessageProcessorProxy<GlobalPiper>>>&) {}

    virtual ~Piper() noexcept {}
};

template <typename T>
class ReceivingFrom {};

template <typename From, typename To, typename Message, typename ... Args>
class Piper<Edge<From, To, Message>, Args...> : public Piper<Args...> {
protected:
    template <typename GlobalPiper, typename From2>
    class SenderProxy {
    public:
        SenderProxy(GlobalPiper& piper) noexcept
        : piper(piper)
        {}

        template <typename To2, typename Message2>
        void Send(std::unique_ptr<Message2>&& message) const {
            int edge_index = piper.GetEdgeIndexImpl(TypeSpecifier<Edge<From2, To2, Message2>>());
            piper.queues[edge_index].Push(std::move(message));
            if (piper.notify_condition_variables[edge_index] != nullptr) {
                piper.notify_condition_variables[edge_index]->notify_one();
            }
        }
    private:
        GlobalPiper& piper;
    };

    template <typename GlobalPiper, typename MP>
    class MessageProcessorProxy : public Piper<>::MessageProcessorProxy<GlobalPiper> {
    public:
        using Piper<>::MessageProcessorProxy<GlobalPiper>::MessageProcessorProxy;
        using Piper<>::MessageProcessorProxy<GlobalPiper>::piper;

        virtual void Ping() const {
            auto& message_processor = MessageProcessorProxy::template GetMessageProcessor<MP>();
            message_processor.Ping(SenderProxy<GlobalPiper, MP>(piper));
        }
    };

    template <typename GlobalPiper>
    class EdgeProxy : public Piper<>::EdgeProxy<GlobalPiper> {
    public:
        using Piper<>::EdgeProxy<GlobalPiper>::EdgeProxy;
        using Piper<>::EdgeProxy<GlobalPiper>::piper;

        virtual void NotifyAboutMessage() const {
            auto& cur_piper = *dynamic_cast<Piper *>(&piper);
            To* message_processor = dynamic_cast<To*>(cur_piper.message_processors[cur_piper.cur_to_index].get());
            auto& current_queue = cur_piper.queues[cur_piper.cur_edge_index];
            current_queue.PopWithHeadDataCallback([&message_processor, this] (const MessageBase& message_base) {
                 const Message* message = dynamic_cast<const Message*>(&message_base);
                 message_processor->Receive(ReceivingFrom<From>(), *message, SenderProxy<GlobalPiper, To>(piper));
            });
        }

        virtual void SetConditionVariable(std::condition_variable_any * cv) const noexcept {
            auto& cur_piper = *dynamic_cast<Piper *>(&piper);
            cur_piper.notify_condition_variables[cur_piper.cur_edge_index] = cv;
        }

        virtual int GetFromIndex() const noexcept {
            auto& cur_piper = *dynamic_cast<Piper *>(&piper);
            return cur_piper.cur_from_index;
        }

        virtual int GetToIndex() const noexcept {
            auto& cur_piper = *dynamic_cast<Piper *>(&piper);
            return cur_piper.cur_to_index;
        }

        virtual ~EdgeProxy() noexcept {}
    };
public:
    using Piper<Args...>::max_message_processor_index;
    using Piper<Args...>::GetMessageProcessorIndexImpl;
    using Piper<Args...>::max_edge_index;
    using Piper<Args...>::dest_pipes;
    using Piper<Args...>::queues;
    using Piper<Args...>::message_processors;
    using Piper<Args...>::notify_condition_variables;
    using Piper<Args...>::GetEdgeIndexImpl;

    Piper() noexcept
    : Piper<Args...>()
    {
    }

    template <typename GlobalPiper, typename MP>
    void AddMessageProcessorIfNotExists(int& target_index,
                                        GlobalPiper& piper,
                                        Vector<std::unique_ptr<Piper<>::MessageProcessorProxy<GlobalPiper>>>& message_processor_handlers) {
        target_index = Piper<Args...>::GetMessageProcessorIndexImpl(TypeSpecifier<MP>());
        if (target_index == -1) {
            target_index = max_message_processor_index++;
            dest_pipes.push_back(Vector<int>());
            message_processors.push_back(std::make_unique<MP>());
            message_processor_handlers.push_back(std::make_unique<MessageProcessorProxy<GlobalPiper, MP>>(piper, target_index));
        }
    }

    template <typename GlobalPiper>
    void AddMessageProcessorsImpl(GlobalPiper& piper,
                                  Vector<std::unique_ptr<Piper<>::MessageProcessorProxy<GlobalPiper>>>& message_processor_handlers) {
        Piper<Args...>::template AddMessageProcessorsImpl<GlobalPiper>(piper, message_processor_handlers);
        AddMessageProcessorIfNotExists<GlobalPiper, From>(cur_from_index, piper, message_processor_handlers);
        AddMessageProcessorIfNotExists<GlobalPiper, To>(cur_to_index, piper, message_processor_handlers);
        cur_edge_index = max_edge_index++;
        dest_pipes[cur_to_index].push_back(cur_edge_index);
        notify_condition_variables.push_back(nullptr);
        queues = Vector<LockFreeQueue<MessageBase>>(max_edge_index);
    }

    template <typename GlobalPiper>
    void FillEdgeProxysImpl(GlobalPiper& piper, Vector<std::unique_ptr<Piper<>::EdgeProxy<GlobalPiper>>>& edge_handers) {
        Piper<Args...>::template FillEdgeProxysImpl<GlobalPiper>(piper, edge_handers);
        edge_handers.push_back(std::make_unique<EdgeProxy<GlobalPiper>>(piper));
    }

    int GetMessageProcessorIndexImpl(const TypeSpecifier<From>&) noexcept {
        return cur_from_index;
    }
    int GetMessageProcessorIndexImpl(const TypeSpecifier<To>&) noexcept {
        return cur_to_index;
    }
    int GetEdgeIndexImpl(const TypeSpecifier<Edge<From, To, Message>>&) noexcept {
        return cur_edge_index;
    }

    virtual ~Piper() {}
private:
    int cur_to_index;
    int cur_from_index;
    int cur_edge_index;
};

template <typename ... Args>
class MessagePassingTree : public Piper<Args...> {
private:
    using GlobalPiper = Piper<Args...>;
public:
    using GlobalPiper::dest_pipes;

    MessagePassingTree()
    : GlobalPiper()
    {
        GlobalPiper::template AddMessageProcessorsImpl<GlobalPiper>(*this, message_processor_handlers);
        GlobalPiper::template FillEdgeProxysImpl<GlobalPiper>(*this, edge_handers);
    }

    const Piper<>::EdgeProxy<GlobalPiper>* GetEdgeProxy(const size_t index) noexcept {
        return edge_handers[index].get();
    }

    const Piper<>::MessageProcessorProxy<GlobalPiper>* GetMessageProcessorProxy(const int index) noexcept {
        return message_processor_handlers[index].get();
    }

    size_t GetEdgesCount() const noexcept {
        return edge_handers.size();
    }

    Vector<int> GetIncomingEdges(int message_processor_index) const noexcept {
        Vector<int> incoming_edges;
        for (int i = 0; i < static_cast<int>(edge_handers.size()); ++i) {
            if (edge_handers[i]->GetToIndex() == message_processor_index) {
                incoming_edges.push_back(i);
            }
        }
        return incoming_edges;
    }

    Vector<int> GetOutgoingEdges(int message_processor_index) const noexcept {
        Vector<int> outgoing_edges;
        for (int i = 0; i < static_cast<int>(edge_handers.size()); ++i) {
            if (edge_handers[i]->GetFromIndex() == message_processor_index) {
                outgoing_edges.push_back(i);
            }
        }
        return outgoing_edges;
    }

    Vector<int> GetConnectingEdges(int first_mp_index, int second_mp_index) const noexcept {
        Vector<int> connecting_edges;
        for (int i = 0; i < static_cast<int>(edge_handers.size()); ++i) {
            if (edge_handers[i]->GetFromIndex() == first_mp_index && edge_handers[i]->GetToIndex() == second_mp_index) {
                connecting_edges.push_back(i);
            }
        }
        return connecting_edges;
    }

    void OutputDestPipes() const noexcept {
        for (auto& pipe: dest_pipes) {
            std::for_each(pipe.begin(), pipe.end(), [](const int num) {std::cout << " " << num;});
            std::cout << std::endl;
        }
    }
private:
    Vector<std::unique_ptr<Piper<>::EdgeProxy<GlobalPiper>>> edge_handers;
    Vector<std::unique_ptr<Piper<>::MessageProcessorProxy<GlobalPiper>>> message_processor_handlers;
};

