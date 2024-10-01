#include <algorithm>
#include <climits>
#include <cstring>
#include <random>
#include <unistd.h>
#include "utils.h"

int min_distance = INT_MAX;
std::vector<int> min_path;

std::vector<int> distances;
int num_cities;

int main(int argc, char *argv[])
{
    struct timespec global_start, global_end;
    struct timespec tmp_start, tmp_end;
    double tmp_start_seconds, tmp_end_seconds;
    clock_gettime(CLOCK_MONOTONIC, &global_start);
    clock_gettime(CLOCK_MONOTONIC, &tmp_start);

    std::string input_filename;
    std::string logs_filename;
    if (argc == 3)
    {
        input_filename = argv[1];
        logs_filename = argv[2];
    }
    else
    {
        std::cout << "Usage: " << argv[0] << " <input_data_filename> <logs_filename>" << std::endl;
        return 0;
    }

    // Clear logs file
    std::ofstream logs_file(logs_filename, std::ios::out);
    logs_file.close();

    // Get hostname
    std::string hostname;
    char hostnameArr[1024];
    hostnameArr[1023] = '\0';
    if (gethostname(hostnameArr, sizeof(hostnameArr)) == 0)
    {
        hostname = std::string(hostnameArr);
    }
    else
    {
        std::cerr << "Error getting hostname" << std::endl;
        hostname = "Unknown";
    }

    // Setup: read input file
    num_cities = read_tsplib_matrix(input_filename, distances);

    // Set starting city
    int first_city = 0;

    // Create all possible pre-paths to be explored and shuffle them
    std::vector<std::vector<int>> paths;
    create_paths(paths, first_city, num_cities);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(paths.begin(), paths.end(), gen);

    clock_gettime(CLOCK_MONOTONIC, &tmp_end);
    tmp_start_seconds = tmp_start.tv_sec + tmp_start.tv_nsec / 1e9;
    tmp_end_seconds = tmp_end.tv_sec + tmp_end.tv_nsec / 1e9;
    log_event(logs_filename, "SETUP", hostname, 0, tmp_start_seconds, tmp_end_seconds);

    // Compute initial minimum distance and path
    clock_gettime(CLOCK_MONOTONIC, &tmp_start);
    std::pair<std::vector<int>, int> result = nearest_neighbor_tsp(distances, num_cities);
    std::vector<int> initial_path = result.first;
    int initial_distance = result.second;
    min_distance = initial_distance;
    min_path = initial_path;
    clock_gettime(CLOCK_MONOTONIC, &tmp_end);
    tmp_start_seconds = tmp_start.tv_sec + tmp_start.tv_nsec / 1e9;
    tmp_end_seconds = tmp_end.tv_sec + tmp_end.tv_nsec / 1e9;
    log_event(logs_filename, "COMPUTATION", hostname, 0, tmp_start_seconds, tmp_end_seconds);

    // Explore all possible pre-paths
    clock_gettime(CLOCK_MONOTONIC, &tmp_start);
    for (int i = 0; i < (int)paths.size(); i++)
    {

        std::vector<int> path = paths[i];
        std::vector<int> visited(num_cities, false);
        int curr_distance = 0;

        // Initialize the curr-distance and visited vector for the current pre-path
        for (int j = 0; j < (int)path.size(); j++)
        {
            if (j < (int)path.size() - 1)
            {
                curr_distance += get_distance(path[j], path[j + 1], distances, num_cities);
            }
            visited[path[j]] = true;
        }

        // Explore the current pre-path
        dfs(path, visited, curr_distance, min_distance, min_path, distances, num_cities);
    }
    clock_gettime(CLOCK_MONOTONIC, &tmp_end);
    tmp_start_seconds = tmp_start.tv_sec + tmp_start.tv_nsec / 1e9;
    tmp_end_seconds = tmp_end.tv_sec + tmp_end.tv_nsec / 1e9;
    log_event(logs_filename, "COMPUTATION", hostname, 0, tmp_start_seconds, tmp_end_seconds);

    clock_gettime(CLOCK_MONOTONIC, &global_end);
    double elapsed_time = global_end.tv_sec + global_end.tv_nsec / 1e9 - global_start.tv_sec - global_start.tv_nsec / 1e9;

    // Print the final results
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "Time: " << elapsed_time << " seconds" << std::endl;
    std::cout << "Minimum distance: " << min_distance << std::endl;
    std::cout << "Minimum path: ";
    for (int i = 0; i < (int)min_path.size(); i++)
    {
        std::cout << min_path[i] + 1 << ", ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

    return 0;
}