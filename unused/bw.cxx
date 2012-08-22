// g++ bw.cxx -O3 -o bw

#include <fcntl.h>

#include "burrows_wheeler.hxx"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << "Usage:\n"
	 << "    ./bw file    : do transform\n"
	 << "    ./bw file.bw : do inverse transform\n";
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY, 0);
  if (fd == -1) {
    cerr << "Could not open file " << argv[1] << '\n';
    exit(1);
  }

  int file_size = lseek(fd, 0, 2);
  lseek(fd, 0, 0);
  cout << "File size = " << file_size << '\n';

  int name_lgth = strlen(argv[1]);
  if (name_lgth > 3  &&
      strcmp(".bw", &(argv[1][name_lgth - 3])) == 0) {
    // do inverse transform
    int special_pos;
    read(fd, &special_pos, 4);

    vector<char> data(file_size-4);
    read(fd, &(data[0]), file_size-4);

    vector<char> result = bw_inverse(data, special_pos);

    argv[1][name_lgth - 3] = 0;
    int f2 = creat(argv[1], 0666);
    if (f2 == -1) {
      cerr << "Could not create file " << argv[1] << '\n';
      exit(1);
    }

    write(f2, &(result[0]), result.size());

    close(f2);

  } else {

    vector<char> data(file_size);
    read(fd, &(data[0]), file_size);
    
    int special_pos;
    vector<char> result = bw_transform(data, special_pos);

    string filename = string(argv[1])+".bw";
    int f2 = creat(filename.c_str(), 0666);
    if (f2 == -1) {
      cerr << "Could not create file " << filename << '\n';
      exit(1);
    }
    
    write(f2, &special_pos, 4);
    write(f2, &(result[0]), result.size());

    close(f2);
  }

  close(fd);
}
