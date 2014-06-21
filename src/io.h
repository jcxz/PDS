#ifndef IO_H
#define IO_H

#include "debug.h"

#include <cerrno>       // errno
#include <cstring>      // strerror()
#include <stdexcept>

#include <unistd.h>     // close(), sysconf()
#include <fcntl.h>      // open(), O_RDONLY
#include <sys/types.h>  // caddr_t
#include <sys/stat.h>   // stat()
#include <sys/mman.h>   // mmap(), munmap()


namespace io {

// A helper class to map flow files to memory
class FileMapper
{
  public:
    FileMapper(void)
      : m_file_data(nullptr)
      , m_size(0)
      , m_fd(-1)
    {
    }

    explicit FileMapper(const char *filename)
      : m_file_data(nullptr)
      , m_size(0)
      , m_fd(-1)
    {
      if ((filename != nullptr) && (!map(filename)))
      {
        throw std::runtime_error("Failed to map the file");
      }
    }

    FileMapper(FileMapper && other)
      : m_file_data(other.m_file_data)
      , m_size(other.m_size)
      , m_fd(other.m_fd)
    {
      other.m_file_data = nullptr;
      //other.m_size = 0;
      other.m_fd = -1;
    }

    ~FileMapper(void) { if (hasMapping()) unmap(); }

    FileMapper & operator=(FileMapper && other)
    {
      std::swap(m_file_data, other.m_file_data);
      std::swap(m_size, other.m_size);
      std::swap(m_fd, other.m_fd);
      return *this;
    }

    template<typename T>
    T *data(void) const { return (T*) (m_file_data); }
    size_t size(void) const { return m_size; }
    int fildes(void) const { return m_fd; }

    bool hasMapping(void) const { return m_fd != -1; }

    bool remap(const char *filename)
    {
      if ((hasMapping()) && (!unmap())) return false;
      return map(filename);
    }

    bool unmap(void)
    {
      errno = 0;

      // unmap the file
      if (::munmap((caddr_t) m_file_data, m_size) == -1)
      {
        FLOW_ERROR("Failed to unmap the file: %s\n", strerror(errno));
        return false;
      }

      // close the file
      if (::close(m_fd) == -1)
      {
        FLOW_WARN("An error occured while closing the file descriptor: %s\n", strerror(errno));
      }

      FLOW_DBG("Unmapped %d:%p:%u\n", m_fd, m_file_data, m_size);

      m_file_data = nullptr;
      m_size = 0;
      m_fd = -1;

      return true;
    }

  private:
    bool map(const char *filename)
    {
      errno = 0;

      // open the file
      m_fd = ::open(filename, O_RDONLY);
      if (m_fd == -1)
      {
        FLOW_ERROR("Failed to open file %s: %s\n", (filename ? filename : "NULL"), strerror(errno));
        return false;
      }

      // find out the file size
      struct stat buf;
      if (::fstat(m_fd, &buf) == -1)
      {
        FLOW_ERROR("Failed to find out the size of the file: %s\n", strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return false;
      }

      m_size = buf.st_size;

      // map the opened file
      m_file_data = ::mmap((caddr_t) 0, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0); // MAP_PRIVATE | MAP_POPULATE
      if (m_file_data == MAP_FAILED)
      {
        FLOW_ERROR("Failed to map the file: %s\n", strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return false;
      }

      FLOW_DBG("Mapped %d:%p:%u\n", m_fd, m_file_data, m_size);

      return true;
    }

  private:
    FileMapper(const FileMapper & );
    FileMapper & operator=(const FileMapper & );

  private:
    void *m_file_data;
    size_t m_size;
    int m_fd;
};

// funkcia, ktora omacá kazdu stranku v namapovanom subore, aby tak vynutila
// nacitanie jej obsahu do pamate
// ten atribut je tu len pre istotu, keby volatile nefungoval ...
void __attribute__((optimize("O0"))) touchPages(const unsigned char *file_data, size_t size);

} // end of namespace io

#endif
