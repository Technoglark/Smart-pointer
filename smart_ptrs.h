#pragma once

#include <cstddef>
#include <string>

class WeakPtr;

class SharedPtr {
   public:
    SharedPtr() = default;

    explicit SharedPtr(std::string* ptr) : ptr_(ptr) {
        if (ptr != nullptr) {
            block_ = new ControlBlock();  // NOLINT(cppcoreguidelines-owning-memory)
            ++block_->shared_count_;
        }
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_ != nullptr) {
            ++block_->shared_count_;
        }
    }

    explicit SharedPtr(const WeakPtr& ptr);

    SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_ != nullptr) {
                ++block_->shared_count_;
            }
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            block_ = other.block_;
            other.block_ = nullptr;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    std::string& operator*() const {
        return *ptr_;
    }

    std::string* operator->() const {
        return ptr_;
    }

    ~SharedPtr() {
        Release();
    }

    [[nodiscard]] std::string* Get() const {
        return ptr_;
    }

    void Reset(std::string* new_ptr = nullptr) {
        Release();
        if (new_ptr != nullptr) {
            ptr_ = new_ptr;
            block_ = new ControlBlock();  // NOLINT(cppcoreguidelines-owning-memory)
            block_->shared_count_ = 1;
        } else {
            ptr_ = nullptr;
            block_ = nullptr;
        }
    }

   private:
    class ControlBlock {
       public:
        size_t shared_count_ = 0;
        size_t weak_count_ = 0;
    };

    std::string* ptr_ = nullptr;
    ControlBlock* block_ = nullptr;
    friend class WeakPtr;

    void Release() {
        if (block_ != nullptr) {
            if (--block_->shared_count_ == 0) {
                delete ptr_;
                ptr_ = nullptr;

                if (block_->weak_count_ == 0) {
                    delete block_;
                    block_ = nullptr;
                }
            }
        }
    }
};

class WeakPtr {
   public:
    WeakPtr() = default;

    explicit WeakPtr(std::string* ptr) : ptr_(ptr) {
        if (block_ != nullptr) {
            block_->weak_count_ = 1;
        }
    }

    WeakPtr(const WeakPtr& other) : block_(other.block_), ptr_(other.ptr_) {
        if (block_ != nullptr) {
            ++block_->weak_count_;
        }
    }

    WeakPtr(WeakPtr&& other) noexcept : block_(other.block_), ptr_(other.ptr_) {
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    }

    explicit WeakPtr(const SharedPtr& shared) : block_(shared.block_), ptr_(shared.ptr_) {
        if (block_ != nullptr) {
            ++block_->weak_count_;
        }
    }

    ~WeakPtr() {
        Release();
    }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            Release();
            ptr_ = other.ptr_;
            block_ = other.block_;

            if (block_ != nullptr) {
                ++block_->weak_count_;
            }
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            Release();

            ptr_ = other.ptr_;
            block_ = other.block_;

            other.ptr_ = nullptr;
            other.block_ = nullptr;
        }
        return *this;
    }

    [[nodiscard]] SharedPtr Lock() const {
        if (block_ != nullptr && block_->shared_count_ > 0) {
            return SharedPtr(*this);
        }
        return {};
    }

    [[nodiscard]] bool IsExpired() const {
        return block_ == nullptr || block_->shared_count_ == 0;
    }

   private:
    SharedPtr::ControlBlock* block_ = nullptr;
    std::string* ptr_ = nullptr;

    void Release() {
        if (block_ != nullptr) {
            --block_->weak_count_;
            if (block_->weak_count_ == 0 && block_->shared_count_ == 0) {
                delete block_;
            }
            block_ = nullptr;
        }
        ptr_ = nullptr;
    }
    friend class SharedPtr;
};

inline SharedPtr::SharedPtr(const WeakPtr& ptr) {
    if (ptr.block_ != nullptr && ptr.block_->shared_count_ > 0) {
        ptr_ = ptr.ptr_;
        block_ = ptr.block_;
        ++block_->shared_count_;
    } else {
        ptr_ = nullptr;
        block_ = nullptr;
    }
}
