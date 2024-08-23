#include <algorithm>
#include <climits>
#include <cstring>
#include <fstream>
#include <random>
#include <omp.h>
#include <unistd.h>
#include "utils.h"

std::vector<int> distances;
int num_cities;

int main(int argc, char *argv[])
{
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

    // Initialize OpenMP
    int num_threads = omp_get_max_threads();
    omp_set_num_threads(num_threads); // Set number of threads based on available hardware

    // Define variables
    double global_start_time = omp_get_wtime();
    int global_min_distance = INT_MAX;
    std::vector<int> global_min_path;
    std::vector<std::vector<int>> paths;

    double start_setup = omp_get_wtime();
    // Setup: read input file
    num_cities = read_file(input_filename, distances); // read the file

    // Setup: Generate random starting city
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_cities - 1);
    int first_city = 0;
    double end_setup = omp_get_wtime();
    log_event(logs_filename, "SETUP", hostname, omp_get_thread_num(), start_setup, end_setup);

    // Computation: Create all possible pre-paths to be explored and shuffle them
    double start_prepaths = omp_get_wtime();
    create_paths(paths, first_city, num_cities);
    std::mt19937 g(num_threads);
    std::shuffle(paths.begin(), paths.end(), g);
    double end_prepaths = omp_get_wtime();
    log_event(logs_filename, "COMPUTATION", hostname, omp_get_thread_num(), start_prepaths, end_prepaths);

#pragma omp parallel
    {
        // Orchestration: Divide the pre-paths among the threads
        double start_divide_prepaths = omp_get_wtime();
        int chunk = paths.size() / num_threads;
        int rest = paths.size() % num_threads;
        int local_min_distance = INT_MAX;
        std::vector<int> local_min_path;
        int thread_number = omp_get_thread_num();
        int start = thread_number * chunk;
        int end = (thread_number + 1) * chunk - 1;
        int iter = 0;
        if (thread_number == num_threads - 1)
        {
            end = paths.size() - 1;
        }
        double end_divide_prepaths = omp_get_wtime();
        log_event(logs_filename, "ORCHESTRATION", hostname, thread_number, start_divide_prepaths, end_divide_prepaths);
#pragma omp for
        for (int i = start; i <= end; i++)
        {
            double start_dfs = omp_get_wtime();
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

            // Explore the pre-path
            dfs(path, visited, curr_distance, local_min_distance, local_min_path, distances, num_cities);

            double end_dfs = omp_get_wtime();
            log_event(logs_filename, "COMPUTATION", hostname, thread_number, start_dfs, end_dfs);

            // Update global minimum distance every once in a while
            if (i < (int)paths.size() - rest - 1 && iter % 720 == 72)
            {
                double start_sync = omp_get_wtime();
#pragma omp critical
                {
                    if (local_min_distance < global_min_distance)
                    {
                        global_min_distance = local_min_distance;
                        global_min_path = local_min_path;
                    }
                }
                double end_sync = omp_get_wtime();
                log_event(logs_filename, "COMMUNICATION", hostname, thread_number, start_sync, end_sync);
            }

            iter++;
        }

        // Recalculate the local_min_distance for the local_min_path
        double start_local_min_distance = omp_get_wtime();
        local_min_distance = 0;
        for (int j = 0; j < num_cities - 1; j++)
        {
            local_min_distance += get_distance(local_min_path[j], local_min_path[j + 1], distances, num_cities);
        }
        local_min_distance += get_distance(local_min_path[num_cities - 1], local_min_path[0], distances, num_cities);
        double end_local_min_distance = omp_get_wtime();
        log_event(logs_filename, "COMPUTATION", hostname, thread_number, start_local_min_distance, end_local_min_distance);

        double start_final_sync = omp_get_wtime();
#pragma omp critical
        {
            if (local_min_distance < global_min_distance)
            {
                global_min_distance = local_min_distance;
                global_min_path = local_min_path;
            }
        }
        double end_final_sync = omp_get_wtime();
        log_event(logs_filename, "COMMUNICATION", hostname, thread_number, start_final_sync, end_final_sync);
    }

    // Print final results
    double global_end_time = omp_get_wtime();
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << "Time: " << global_end_time - global_start_time << std::endl;
    std::cout << "Minimum distance: " << global_min_distance << std::endl;
    std::cout << "Minimum path: ";
    for (int i = 0; i < (int)global_min_path.size(); i++)
    {
        std::cout << global_min_path[i] + 1 << ", ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

    return 0;
}
