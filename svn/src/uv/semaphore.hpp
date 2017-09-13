#ifndef NODE_SVN_UV_SEMAPHORE_H
#define NODE_SVN_UV_SEMAPHORE_H

#include <uv.h>

namespace Uv
{
class Semaphore
{
  public:
    Semaphore(uint32_t count = 0)
    {
        uv_sem_init(&handle, count);
    }

    ~Semaphore()
    {
        uv_sem_destroy(&handle);
    }

    void post()
    {
        uv_sem_post(&handle);
    }

    void wait()
    {
        uv_sem_wait(&handle);
    }

  private:
    uv_sem_t handle;
};
}

#endif
