template <typename Registry, typename Entity>
class AutoRegistrar;

template <typename Registry, typename Entity>
AutoRegistrar<Registry, Entity> auto_registrator{};

template <typename Registry, typename Entity>
class AutoRegistrar {
public:
    AutoRegistrar()
    {
        Registry::template Register<Entity>();
    }

    static void AutoRegister() {
        (void)auto_registrator<Registry, Entity>;
    }
};


