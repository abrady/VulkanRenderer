#pragma once

// This class is movable but not copyable
// It is a common pattern for RAII classes that manage resources
class ClassMovableOnly
{
public:
    ClassMovableOnly() = default;

    // Delete copy constructor and copy assignment operator
    ClassMovableOnly(const ClassMovableOnly &) = delete;
    ClassMovableOnly &operator=(const ClassMovableOnly &) = delete;

    // Allow move constructor and move assignment operator

    // Move constructor
    ClassMovableOnly(ClassMovableOnly &&other) noexcept : vk(other.vk)
    {
        moveFrom(std::move(other));
    }

    // Move assignment operator
    ClassMovableOnly &operator=(ClassMovableOnly &&other) noexcept
    {
        if (this != &other)
        {
            release();
            moveFrom(std::move(other));
        }
        return *this;
    }

protected:
    // Virtual functions for resource management
    virtual void release() = 0;                          // Release the resource
    virtual void moveFrom(ClassMovableOnly &&other) = 0; // Move resource logic
};