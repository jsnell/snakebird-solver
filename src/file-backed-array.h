// -*- mode: c++ -*-

#ifndef FILE_BACKED_ARRAY_H
#define FILE_BACKED_ARRAY_H

template<class T, size_t kFlushThreshold = 1000000>
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
        maybe_unmap();
        buffer_ = std::move(other.buffer_);
        frozen_ = other.frozen_;
        size_ = other.size_;
        fd_ = other.fd_;
        array_ = other.array_;
        other.array_ = NULL;
        other.fd_ = -1;
    }

    ~file_backed_mmap_array() {
        maybe_unmap();
    }

    void maybe_unmap() {
        if (fd_ >= 0) {
            if (array_) {
                munmap((void*) array_, size_ * sizeof(T));
                array_ = NULL;
            }
            close(fd_);
            fd_ = -1;
        }
    }

    void open() {
        assert(fd_ == -1);
        fd_ = ::open("file-backed-tmp", O_CREAT | O_RDWR);
        assert(fd_ >= 0);
        unlink("file-backed-tmp");
    }

    void flush() {
        if (buffer_.size()) {
            assert(fd_ >= 0);
            size_t bytes = sizeof(T) * buffer_.size();
            assert(write(fd_, (char*) &buffer_[0], bytes) == bytes);
            buffer_.clear();
        }
    }

    bool empty() const { return size_ > 0; }

    T* begin() {
        assert(frozen_);
        return array_;
    }
    T* end() {
        assert(frozen_);
        return array_ + size_;
    }
    size_t size() const { return size_; }

    void push_back(const T& data) {
        assert(!frozen_);
        buffer_.push_back(data);
        if (buffer_.size() > kFlushThreshold) {
            if (fd_ == -1) {
                open();
            }
            flush();
            buffer_.clear();
        }
        size_++;
    }

    void freeze() {
        assert(!frozen_);
        if (fd_ >= 0) {
            flush();
            size_t len = sizeof(T) * size_;
            void* map = mmap(NULL, len, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd_, 0);
            if (map == MAP_FAILED) {
                perror("mmap");
                abort();
            }
            array_ = (T*) map;
        } else {
            array_ = &buffer_[0];
        }
        frozen_ = true;
    }

//private:
    std::vector<T> buffer_;
    bool frozen_ = false;
    size_t size_ = 0;
    int fd_ = -1;
    T* array_ = NULL;
};

#endif // FILE_BACKED_ARRAY_H
