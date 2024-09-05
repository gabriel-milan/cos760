#include <algorithm>
#include <climits>
#include <cstring>
#include <random>
#include <unistd.h>
#include <omp.h>
#include "utils.h"

int min_distance = INT_MAX;
std::vector<int> min_path;

std::vector<int> distances;
int num_cities;

int main(int argc, char *argv[])
{
    double global_start_time, global_end_time;
    global_start_time = omp_get_wtime();
    double start_time = omp_get_wtime();

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

    int total_paths = paths.size(); // Define total_paths here

    double end_time = omp_get_wtime();
    log_event(logs_filename, "SETUP", hostname, 0, start_time, end_time);

// Explore all possible pre-paths in parallel
#pragma omp parallel
    {
        // Compute initial minimum distance and path
        int thread_id = omp_get_thread_num();
        double start_time = omp_get_wtime();
        auto [initial_path, initial_distance] = nearest_neighbor_tsp(distances, num_cities);
        int thread_min_distance = initial_distance;
        std::vector<int> thread_min_path = initial_path;
        double end_time = omp_get_wtime();
        log_event(logs_filename, "COMPUTATION", hostname, thread_id, start_time, end_time);

        int paths_processed = 0;
        int current_sync_period = INITIAL_SYNC_PERIOD;
        int paths_since_last_sync = 0;

#pragma omp for schedule(dynamic)
        for (int i = 0; i < total_paths; i++)
        {
            double computation_start = omp_get_wtime();

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
            dfs(path, visited, curr_distance, thread_min_distance, thread_min_path, distances, num_cities);

            double computation_end = omp_get_wtime();
            log_event(logs_filename, "COMPUTATION", hostname, thread_id, computation_start, computation_end);

            paths_processed++;

            paths_since_last_sync++;

            // Periodic synchronization based on calculated update frequency
            if (paths_since_last_sync >= current_sync_period)
            {
                double communication_start = omp_get_wtime();
#pragma omp critical
                {
                    if (thread_min_distance < min_distance)
                    {
                        min_distance = thread_min_distance;
                        min_path = thread_min_path;
                    }
                }
                double communication_end = omp_get_wtime();
                log_event(logs_filename, "COMMUNICATION", hostname, thread_id, communication_start, communication_end);

                // Increase the sync period for next time
                current_sync_period = std::min(
                    static_cast<int>(current_sync_period * SYNC_INCREASE_FACTOR),
                    MAX_SYNC_PERIOD);

                paths_since_last_sync = 0;
            }
        }

        // Final update of global minimum
        double communication_start = omp_get_wtime();
#pragma omp critical
        {
            if (thread_min_distance < min_distance)
            {
                min_distance = thread_min_distance;
                min_path = thread_min_path;
            }
        }
        double communication_end = omp_get_wtime();
        log_event(logs_filename, "COMMUNICATION", hostname, thread_id, communication_start, communication_end);
    }

    global_end_time = omp_get_wtime();
    double elapsed_time = global_end_time - global_start_time;

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
