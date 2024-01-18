/* The following code takes input data files that contain integers line-by-line.
The user specifies the file index from (1, 2, 3).
The program reads this file in and adds the numbers to a vector. Then it
gets the number of child processes the user would like to create to run this
program. The program then splits the data into evenly sized blocks and each
child process adds the numbers up from their block and pipes the result to the
parent. The parent prints each of the child results out and then prints the
total result of all the numbers added up. The system time is also recorded to
compare the speed of each file and process count configuration.*/

#include <chrono>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <vector>

using namespace std;

// Function to process the given block of data from the nums vector
int process_block(int block_size, int start_pos, int num_processes,
                  const vector<int> &nums) {
  int result = 0;
  // set up end postition of current block
  int end_pos = start_pos + block_size;
  if (end_pos > nums.size()) {
    end_pos = nums.size();
  }

  // add the numbers in current child block
  for (int i = start_pos; i < end_pos; i++) {
    result += nums[i];
  }

  return result;
}

int main() {
  int num_processes, file_index;
  // get processor count and index of file from user
  cout << "Enter number of child processes (1, 2, or 4): ";
  cin >> num_processes;
  cout << "Enter index of file to process (1, 2, or 3): ";
  cin >> file_index;

  // Open input file
  string filename = "file" + to_string(file_index) + ".dat";
  ifstream infile(filename);
  // handle errors
  if (!infile) {
    cerr << "Unable to open input file" << endl;
    return 1;
  }

  // Load input file into array
  vector<int> nums;
  string line;
  // start the time here to include the vector loading up the numbers from the
  // file
  auto start = std::chrono::high_resolution_clock::now();
  while (getline(infile, line)) {
    nums.push_back(stoi(line));
  }

  // Determine block size for each child process
  int block_size = nums.size() / num_processes;
  int extra_lines =
      nums.size() % num_processes; // handles files with odd numbers of lines

  // Create pipes for specified number of child processes
  int **pipes = new int *[num_processes];
  for (int i = 0; i < num_processes; ++i) {
    pipes[i] = new int[2];
    // handle errors
    if (pipe(pipes[i]) == -1) {
      cerr << "Unable to create pipe" << endl;
      return 1;
    }
  }

  // Create child processes
  for (int i = 0; i < num_processes; ++i) {
    pid_t pid = fork();

    // error handling
    if (pid == -1) {
      cerr << "Unable to create child process" << endl;
      return 1;
    } else if (pid == 0) {
      // Child process code
      close(pipes[i][0]); // Close read end of pipe

      int start_pos = i * block_size; // set start_pos of block
      // handles files with odd number of lines
      if (i == num_processes - 1) {
        block_size +=
            extra_lines; // assign remaining lines to last child process
      }
      // compute the sum of the child block
      int result = process_block(block_size, start_pos, num_processes, nums);

      write(pipes[i][1], &result,
            sizeof(int)); // write result to parent process
      close(pipes[i][1]); // Close write end of pipe

      exit(0);
    }
  }

  // Parent process code
  int total = 0;
  // loop to get each child result
  for (int i = 0; i < num_processes; ++i) {
    close(pipes[i][1]); // Close write end of pipe

    int child_result = 0;
    int bytes_read = 0;
    // read in child result
    while ((bytes_read = read(pipes[i][0], &child_result, sizeof(int))) > 0)
      // error handling
      if (bytes_read == -1) {
        cerr << "Error reading from pipe" << endl;
        return 1;
      }

    // print child results and add it to total
    cout << "Child " << i << " result: " << child_result << endl;
    total += child_result;

    close(pipes[i][0]); // Close read end of pipe
  }

  // Print total result
  cout << "Total: " << total << endl;

  // Wait for child processes to complete
  for (int i = 0; i < num_processes; ++i) {
    wait(NULL);
  }

  // end program timer and then calculate the total duration and print it
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start)
          .count();
  cout << "Duration: " << duration << " microseconds" << endl;

  return 0;
}