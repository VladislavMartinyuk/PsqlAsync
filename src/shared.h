#ifndef SHARED_H
#define SHARED_H

namespace async_psql::details {

template <typename T> class NoCopyble {
public:
    NoCopyble(const NoCopyble &) = delete;
    NoCopyble &operator=(const NoCopyble &) = delete;

protected:
    NoCopyble() = default;
    ~NoCopyble() = default;
};

} // namespace async_psql::details

#endif // SHARED_H
