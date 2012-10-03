 #include <unistd.h>
  int main(int argc, char *argv[]) {
    const char *script = "/root/bin/gpfsdf";
    execv(script, argv);
  }
