// -*- mode: c++ -*-

#ifndef FILE_BACKED_ARRAY_H
#define FILE_BACKED_ARRAY_H

#include <cstdlib>

template<class T,
         // 100M
         size_t kFlushThreshold = 100000000 / sizeof(T)>
class file_backed_mmap_array {
public:
    file_backed_mmap_array() {
    }

    file_backed_mmap_array(const file_backed_mmap_array& other) = delete;

    file_backed_mmap_array(file_backed_mmap_array&& other)
        : buffer_(std::move(other.buffer_)),
          frozen_(other.frozen_),
          size_(other.size_),
          fd_(other.fd_),
          array_(other.array_) {
        other.array_ = NULL;
        other.fd_ = -1;
    }

    void operator=(file_backed_mmap_array&& other) {
        maybe_close();
        buffer_ = std::move(other.buffer_);
        frozen_ = other.frozen_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        fd_ = other.fd_;
        array_ = other.array_;
        other.array_ = NULL;
        other.fd_ = -1;
    }

    ~file_backed_mmap_array() {
        maybe_close();
    }

    void maybe_unmap(bool truncate) {
        if (fd_ >= 0 && array_) {
            munmap((void*) array_, capacity_ * sizeof(T));
            if (truncate) {
                ftruncate(fd_, 0);
            }
        }
        array_ = NULL;
    }

    void maybe_close() {
        if (fd_ >= 0) {
            maybe_unmap(true);
            close(fd_);
            fd_ = -1;
        }
    }

    void open() {
        assert(fd_ == -1);
        char* fname = strdup("file-backed-tmp-XXXXXX");
        fd_ = mkstemp(fname);
        assert(fd_ >= 0);
        unlink(fname);
    }

    void flush() {
        if (buffer_.size() && fd_ >= 0) {
            size_t bytes = sizeof(T) * buffer_.size();
            assert(write(fd_, (char*) &buffer_[0], bytes) == bytes);
            buffer_.clear();
        }
    }

    bool empty() const { return size_ == 0; }

    T* begin() {
        assert(frozen_);
        return array_;
    }
    T* end() {
        assert(frozen_);
        return array_ + size_;
    }

    const T* begin() const {
        assert(frozen_);
        return array_;
    }
    const T* end() const {
        assert(frozen_);
        return array_ + size_;
    }

    T& operator[](size_t index) {
        assert(frozen_);
        return array_[index];
    }

    const T& operator[](size_t index) const {
        assert(frozen_);
        return array_[index];
    }

    size_t size() const { return size_; }
    size_t capacity() const {
        assert(frozen_);
        return capacity_;
    }

    void push_back(const T& data) {
        assert(!frozen_);
        buffer_.push_back(data);
        if (buffer_.size() >= kFlushThreshold) {
            if (fd_ == -1) {
                open();
            }
            flush();
            buffer_.clear();
        }
        size_++;
    }

    void freeze() {
        maybe_map(PROT_READ, MAP_SHARED);
    }

    void snapshot() {
        maybe_map(PROT_READ | PROT_WRITE, MAP_SHARED);
    }

    void thaw() {
        assert(frozen_);
        frozen_ = false;
        maybe_unmap(false);
    }

    void reset() {
        assert(frozen_);
        frozen_ = false;
        if (fd_ >= 0) {
            lseek(fd_, 0, SEEK_SET);
        }
        maybe_unmap(true);
        size_ = 0;
        buffer_.clear();
    }

    void start_run() {
        run_starts_.push_back(size_);
    }

    void end_run() {
        run_ends_.push_back(size_);
    }

    std::pair<size_t, size_t> run(int i) const {
        return std::make_pair(run_starts_[i],
                              run_ends_[i]);
    }

    int runs() const {
        return run_ends_.size();
    }

    void resize(size_t new_size) {
        assert(frozen_);
        size_ = new_size;
    }

private:
    void maybe_map(int prot, int flags) {
        assert(!frozen_);
        if (fd_ >= 0 && size_ > 0) {
            flush();
            size_t len = sizeof(T) * size_;
            void* map = mmap(NULL, len, prot, flags, fd_, 0);
            if (map == MAP_FAILED) {
                perror("mmap");
                abort();
            }
            array_ = (T*) map;
        } else {
            array_ = &buffer_[0];
        }
        frozen_ = true;
        capacity_ = size_;
    }

    std::vector<size_t> run_starts_;
    std::vector<size_t> run_ends_;
    std::vector<T> buffer_;
    bool frozen_ = false;
    size_t size_ = 0;
    size_t capacity_ = 0;
    int fd_ = -1;
    T* array_ = NULL;
};

#endif // FILE_BACKED_ARRAY_H
