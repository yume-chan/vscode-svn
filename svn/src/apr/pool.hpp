#ifndef ARP_CPP_POOL_H
#define ARP_CPP_POOL_H

struct apr_pool_t;

namespace apr
{
class pool
{
  public:
    static pool unmanaged(apr_pool_t *raw);

    explicit pool();

    ~pool();

    apr_pool_t *get() const;

    pool create() const;

    template <typename T>
    T *alloc() const;

    void *memcpy(void *source, size_t count) const;

  private:
    explicit pool(apr_pool_t *raw, bool managed);

    bool _managed;
    apr_pool_t *_value;
};
}

#endif
