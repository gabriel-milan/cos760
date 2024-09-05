#include <algorithm>
#include <climits>
#include <cstring>
#include <random>
#include <unistd.h>
#include <mpi.h>
#include "utils.h"

int min_distance = INT_MAX;
std::vector<int> min_path;

std::vector<int> distances;
int num_cities;

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double start_time, end_time, global_start_time, global_end_time;
    global_start_time = MPI_Wtime();
    start_time = MPI_Wtime();

    std::string input_filename;
    std::string logs_filename;
    if (argc == 3)
    {
        input_filename = argv[1];
        logs_filename = argv[2];
    }
    else
    {
        if (rank == 0)
        {
            std::cout << "Usage: " << argv[0] << " <input_data_filename> <logs_filename>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }

    // Clear logs file (only on rank 0)
    if (rank == 0)
    {
        std::ofstream logs_file(logs_filename, std::ios::out);
        logs_file.close();
    }

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

    // Setup: read input file (all processes read the file)
    num_cities = read_tsplib_matrix(input_filename, distances);

    // Set starting city
    int first_city = 0;

    // Create all possible pre-paths to be explored and shuffle them
    std::vector<std::vector<int>> paths;
    create_paths(paths, first_city, num_cities);

    // Only rank 0 shuffles the paths
    if (rank == 0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(paths.begin(), paths.end(), gen);
    }
    end_time = MPI_Wtime();
    log_event(logs_filename, "SETUP", hostname, rank, start_time, end_time);

    // Broadcast the shuffled paths to all processes
    start_time = MPI_Wtime();
    int paths_size = paths.size();
    MPI_Bcast(&paths_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0)
    {
        paths.resize(paths_size);
    }

    for (int i = 0; i < paths_size; i++)
    {
        int path_size = paths[i].size();
        MPI_Bcast(&path_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (rank != 0)
        {
            paths[i].resize(path_size);
        }

        MPI_Bcast(paths[i].data(), path_size, MPI_INT, 0, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();
    log_event(logs_filename, "COMMUNICATION", hostname, rank, start_time, end_time);

    // Generate initial solution on all processes
    start_time = MPI_Wtime();
    auto [initial_path, initial_distance] = nearest_neighbor_tsp(distances, num_cities);
    int local_min_distance = initial_distance;
    std::vector<int> local_min_path = initial_path;
    end_time = MPI_Wtime();
    log_event(logs_filename, "COMPUTATION", hostname, rank, start_time, end_time);

    // Distribute paths among processes
    start_time = MPI_Wtime();
    int paths_per_process = paths.size() / size;
    int start_index = rank * paths_per_process;
    int end_index = (rank == size - 1) ? paths.size() : (rank + 1) * paths_per_process;
    end_time = MPI_Wtime();
    log_event(logs_filename, "ORCHESTRATION", hostname, rank, start_time, end_time);

    // Explore assigned paths
    int paths_processed = 0;
    int current_sync_period = INITIAL_SYNC_PERIOD;
    int paths_since_last_sync = 0;

    for (int i = start_index; i < end_index; i++)
    {
        start_time = MPI_Wtime();

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

        // Use local_min_distance as the initial upper bound
        dfs(path, visited, curr_distance, local_min_distance, local_min_path, distances, num_cities);

        end_time = MPI_Wtime();
        log_event(logs_filename, "COMPUTATION", hostname, rank, start_time, end_time);

        paths_processed++;

        paths_since_last_sync++;

        if (paths_since_last_sync >= current_sync_period)
        {
            // Perform synchronization
            start_time = MPI_Wtime();
            int local_min = local_min_distance;
            MPI_Allreduce(&local_min, &min_distance, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
            end_time = MPI_Wtime();

            // Log the communication event
            log_event(logs_filename, "COMMUNICATION", hostname, rank, start_time, end_time);

            // Increase the sync period for next time
            current_sync_period = std::min(
                static_cast<int>(current_sync_period * SYNC_INCREASE_FACTOR),
                MAX_SYNC_PERIOD);

            paths_since_last_sync = 0;
        }
    }

    // Ensure one final sync at the end
    if (paths_since_last_sync > 0)
    {
        start_time = MPI_Wtime();
        int local_min = local_min_distance;
        MPI_Allreduce(&local_min, &min_distance, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        end_time = MPI_Wtime();

        log_event(logs_filename, "COMMUNICATION", hostname, rank, start_time, end_time);
    }

    // Final gather of results from all processes
    start_time = MPI_Wtime();
    struct
    {
        int distance;
        int rank;
    } local_result, global_result;

    local_result.distance = local_min_distance;
    local_result.rank = rank;

    MPI_Allreduce(&local_result, &global_result, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

    int path_size = num_cities;
    // Broadcast the best path
    if (rank == global_result.rank)
    {
        path_size = local_min_path.size();
        min_path = local_min_path;
    }

    // Broadcast the size of the best path
    MPI_Bcast(&path_size, 1, MPI_INT, global_result.rank, MPI_COMM_WORLD);

    // Resize min_path on all processes to receive the best path
    min_path.resize(path_size);

    // Broadcast the best path
    MPI_Bcast(min_path.data(), path_size, MPI_INT, global_result.rank, MPI_COMM_WORLD);

    // Broadcast the best distance
    MPI_Bcast(&min_distance, 1, MPI_INT, global_result.rank, MPI_COMM_WORLD);

    end_time = MPI_Wtime();
    log_event(logs_filename, "COMMUNICATION", hostname, rank, start_time, end_time);

    global_end_time = MPI_Wtime();
    double total_time = global_end_time - global_start_time;

    // Print the final results (only on rank 0)
    if (rank == 0)
    {
        std::cout << "---------------------------------------------" << std::endl;
        std::cout << "Total execution time: " << total_time << " seconds" << std::endl;
        std::cout << "Minimum distance: " << min_distance << std::endl;
        std::cout << "Minimum path: ";
        for (int i = 0; i < (int)min_path.size(); i++)
        {
            std::cout << min_path[i] + 1 << " "; // +1 because cities are typically 1-indexed in output
        }
        std::cout << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
