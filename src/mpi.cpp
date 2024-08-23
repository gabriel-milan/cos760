#include <algorithm>
#include <chrono>
#include <climits>
#include <cstring>
#include <fstream>
#include <random>
#include <thread>
#include <mpi.h>
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

    // Initialize MPI
    MPI_Init(NULL, NULL);
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

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

    // Define variables
    int buffer;
    double global_start_time = MPI_Wtime();
    int local_min_distance = INT_MAX;
    int global_min_distance;
    std::vector<int> local_min_path;
    std::vector<std::vector<int>> paths;

    double start_setup = MPI_Wtime();
    // Setup: read input file
    num_cities = read_file(input_filename, distances);

    // Setup: Generate random starting city
    int first_city;
    if (rank == 0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, num_cities - 1);
        first_city = 0;
    }
    double end_setup = MPI_Wtime();
    log_event(logs_filename, "SETUP", hostname, rank, start_setup, end_setup);

    // Communication: Broadcast the starting city
    double start_broadcast_starting_city = MPI_Wtime();
    MPI_Bcast(&first_city, 1, MPI_INT, 0, MPI_COMM_WORLD);
    double end_broadcast_starting_city = MPI_Wtime();
    log_event(logs_filename, "COMMUNICATION", hostname, rank, start_broadcast_starting_city, end_broadcast_starting_city);

    // Computation: Create all possible pre-paths to be explored and shuffle them
    double start_prepaths = MPI_Wtime();
    create_paths(paths, first_city, num_cities);
    std::mt19937 g(num_procs);
    std::shuffle(paths.begin(), paths.end(), g);
    double end_prepaths = MPI_Wtime();
    log_event(logs_filename, "COMPUTATION", hostname, rank, start_prepaths, end_prepaths);

    // Orchestration: Divide the pre-paths between the processes
    double start_divide_prepaths = MPI_Wtime();
    int chunk = paths.size() / num_procs;
    int start = rank * chunk;
    int end = (rank + 1) * chunk - 1;
    int rest = paths.size() % num_procs;
    int iter = 0;
    if (rank == num_procs - 1)
    {
        end = paths.size() - 1;
    }
    double end_divide_prepaths = MPI_Wtime();
    log_event(logs_filename, "ORCHESTRATION", hostname, rank, start_divide_prepaths, end_divide_prepaths);

    // Explore the pre-paths assigned to the process
    for (int i = start; i <= end; i++)
    {
        double start_dfs = MPI_Wtime();
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

        double end_dfs = MPI_Wtime();
        log_event(logs_filename, "COMPUTATION", hostname, rank, start_dfs, end_dfs);

        if (i < (int)paths.size() - rest - 1 && iter % 720 == 72)
        {
            double start_sync = MPI_Wtime();
            // Do a blocking communication to update the local_min_distance
            buffer = local_min_distance;
            MPI_Allreduce(&buffer, &global_min_distance, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
            // Update the local_min_distance
            if (global_min_distance < local_min_distance)
            {
                local_min_distance = global_min_distance;
            }
            double end_sync = MPI_Wtime();
            log_event(logs_filename, "COMMUNICATION", hostname, rank, start_sync, end_sync);
        }

        iter++;
    }

    // Recalculate the local_min_distance for the local_min_path
    double start_local_min_distance = MPI_Wtime();
    local_min_distance = 0;
    for (int j = 0; j < num_cities - 1; j++)
    {
        local_min_distance += get_distance(local_min_path[j], local_min_path[j + 1], distances, num_cities);
    }
    local_min_distance += get_distance(local_min_path[num_cities - 1], local_min_path[0], distances, num_cities);
    double end_local_min_distance = MPI_Wtime();
    log_event(logs_filename, "COMPUTATION", hostname, rank, start_local_min_distance, end_local_min_distance);

    double start_final_sync = MPI_Wtime();
    // Gather minimum distances and paths from all ranks
    std::vector<int> global_min_path;
    if (rank == 0)
    {
        global_min_path.resize(num_cities);
    }
    int *min_distances = new int[num_procs];
    MPI_Allgather(&local_min_distance, 1, MPI_INT, min_distances, 1, MPI_INT, MPI_COMM_WORLD);

    // Find rank with global minimum distance
    int global_min_rank = std::distance(min_distances, std::min_element(min_distances, min_distances + num_procs));

    // Receive global minimum path from rank with global minimum distance
    if (rank == 0)
    {
        if (global_min_rank != 0)
        {
            MPI_Recv(global_min_path.data(), num_cities, MPI_INT, global_min_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else
        {
            global_min_path = local_min_path;
        }
        global_min_distance = min_distances[global_min_rank];
    }
    else if (rank == global_min_rank)
    {
        MPI_Send(local_min_path.data(), num_cities, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Free min_distances array
    delete[] min_distances;
    double end_final_sync = MPI_Wtime();
    log_event(logs_filename, "COMMUNICATION", hostname, rank, start_final_sync, end_final_sync);

    // Print final results
    if (rank == 0)
    {
        double global_end_time = MPI_Wtime();
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
    }

    MPI_Finalize();

    return 0;
}