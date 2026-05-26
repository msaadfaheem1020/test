#include "engine.h"
#include "utils/logger.h"

int main() {
  Logger::init();
  Engine().run();
}
