#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct DVec3 {
  double x, y, z;
};

struct Color {
  unsigned char r, g, b, a;
};

struct Body {
  DVec3 position;
  DVec3 velocity;
  DVec3 acceleration;
  double mass;
  double radius;
  Color color;
  char pad[4];
};

static_assert(sizeof(DVec3) == 24, "DVec3 size mismatch");
static_assert(sizeof(Body) == 96, "Body size mismatch");

enum class MmapStatus : int { IDLE = 0, COMPUTE = 1, RELOAD = 2, EXIT = 3 };

struct MmapHeader {
  int num_bodies;
  int pad0;
  double dt;
  double gravity;
  std::atomic<MmapStatus> status;
  char pad1[36];
};

static_assert(sizeof(MmapHeader) == 64, "MmapHeader size mismatch");

extern "C" void compute_step(Body *bodies, int n, double dt, double gravity);

int main() {
  const char *filename = "nbody_mmap.bin";

  while (true) {
    int fd = -1;
    while (fd < 0) {
      fd = open(filename, O_RDWR);
      if (fd < 0) {
        std::cerr << "Waiting for " << filename << "...\n";
        usleep(250000);
      }
    }

    MmapHeader header_tmp;
    while (true) {
      if (pread(fd, &header_tmp, sizeof(header_tmp), 0) !=
          (ssize_t)sizeof(header_tmp)) {
        usleep(100000);
        continue;
      }

      MmapStatus s = header_tmp.status.load();
      if (s != MmapStatus::RELOAD && s != MmapStatus::EXIT) {
        break;
      }

      if (s == MmapStatus::EXIT) {
        close(fd);
        return 0;
      }

      usleep(100000);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
      std::cerr << "fstat failed.\n";
      close(fd);
      usleep(500000);
      continue;
    }

    size_t map_size = st.st_size;
    void *ptr =
        mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (ptr == MAP_FAILED) {
      std::cerr << "mmap failed.\n";
      close(fd);
      usleep(500000);
      continue;
    }

    MmapHeader *header = static_cast<MmapHeader *>(ptr);
    Body *bodies = reinterpret_cast<Body *>(header + 1);

    std::cout << "riscv worker ready (" << header->num_bodies << " bodies)\n";

    bool should_reload = false;
    while (true) {
      MmapStatus s = header->status.load(std::memory_order_acquire);

      if (s == MmapStatus::COMPUTE) {
        compute_step(bodies, header->num_bodies, header->dt, header->gravity);
        header->status.store(MmapStatus::IDLE, std::memory_order_release);
      } else if (s == MmapStatus::RELOAD) {
        should_reload = true;
        break;
      } else if (s == MmapStatus::EXIT) {
        munmap(ptr, map_size);
        close(fd);
        return 0;
      } else {
        usleep(500);
      }
    }

    munmap(ptr, map_size);
    close(fd);

    if (should_reload) {
      std::cout << "Reloading dataset...\n";
      continue;
    }
  }
}
