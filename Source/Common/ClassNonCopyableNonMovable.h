#pragma once

// This class is not copyable or movable
// use this for things like Vulkan resources, file handles, etc. where
// the resource is owned by the class and should definitely not be copied
// and where you want to prevent moving because you don't want to mess
// around with defining move semantics for the class.
//
// inherit from this class if you want to force a paradigm of always
// using smart pointers, for example, to manage the lifetime of the resource:
// you can use the smart pointer's move semantics to transfer ownership.
class ClassNonCopyableNonMovable
{
public:
    ClassNonCopyableNonMovable() = default;
    ClassNonCopyableNonMovable(const ClassNonCopyableNonMovable &) = delete;
    ClassNonCopyableNonMovable &operator=(const ClassNonCopyableNonMovable &) = delete;
    ClassNonCopyableNonMovable(ClassNonCopyableNonMovable &&) = delete;
    ClassNonCopyableNonMovable &operator=(ClassNonCopyableNonMovable &&) = delete;
};