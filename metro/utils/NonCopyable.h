#pragma once

class NonCopyable
{
  public:
    NonCopyable() = default;
    NonCopyable(NonCopyable &&) = default;
    
  private:
    NonCopyable(const NonCopyable &) = delete;
    const NonCopyable &operator=(const NonCopyable &) = delete;
};