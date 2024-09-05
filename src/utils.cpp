#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

const int INITIAL_SYNC_PERIOD = 100;     // Start by syncing every 10 paths
const int MAX_SYNC_PERIOD = 1000;        // Never wait more than 1000 paths before syncing
const double SYNC_INCREASE_FACTOR = 1.5; // Increase sync period by 50% each time

void create_paths(std::vector<std::vector<int>> &paths, int start, int num_cities)
{
    for (int i = 0; i < num_cities; i++)
    {
        if (i == start)
            continue;
        for (int j = 0; j < num_cities; j++)
        {
            if (j == start || j == i)
                continue;
            for (int z = 0; z < num_cities; z++)
            {
                if (z == start || z == i || z == j)
                    continue;
                for (int x = 0; x < num_cities; x++)
                {
                    if (x == start || x == i || x == j || x == z)
                        continue;
                    std::vector<int> path;
                    path.push_back(start);
                    path.push_back(i);
                    path.push_back(j);
                    path.push_back(z);
                    path.push_back(x);
                    paths.push_back(path);
                }
            }
        }
    }
}

int get_distance(int city1, int city2, std::vector<int> &distances, int num_cities)
{
    if (city1 >= num_cities || city2 >= num_cities)
    {
        std::cerr << "Error: Invalid city number." << std::endl;
        return 999;
    }
    return distances[city1 * num_cities + city2];
}

void dfs(
    std::vector<int> &path,
    std::vector<int> &visited,
    int &curr_distance,
    int &min_distance,
    std::vector<int> &min_path,
    std::vector<int> &distances,
    int num_cities)
{

    // Base case: all cities have been visited
    if ((int)path.size() == num_cities)
    {
        // Add the distance from the last city back to the starting city
        int dist = curr_distance + get_distance(path[path.size() - 1], path.back(), distances, num_cities);
        if (dist < min_distance)
        {
            min_distance = dist;
            min_path = path;
        }
        return;
    }

    // Try visiting all unvisited cities
    for (int i = 0; i < num_cities; i++)
    {
        if (!visited[i])
        {
            int prev_distance = curr_distance;
            curr_distance += get_distance(path.back(), i, distances, num_cities);
            // Prune the search tree if the current distance is already greater than the minimum distance
            if (curr_distance >= min_distance)
            {
                curr_distance = prev_distance;
                continue;
            }
            visited[i] = true;
            path.push_back(i);
            dfs(path, visited, curr_distance, min_distance, min_path, distances, num_cities);
            path.pop_back();
            visited[i] = false;
            curr_distance = prev_distance;
        }
    }
}

// Function to generate an initial solution using Nearest Neighbor heuristic
std::pair<std::vector<int>, int> nearest_neighbor_tsp(std::vector<int> &distances, int num_cities)
{
    std::vector<int> path;
    std::vector<bool> visited(num_cities, false);
    int current_city = 0; // Start from city 0
    int total_distance = 0;

    path.push_back(current_city);
    visited[current_city] = true;

    for (int i = 1; i < num_cities; ++i)
    {
        int nearest_city = -1;
        int min_distance = INT_MAX;

        for (int j = 0; j < num_cities; ++j)
        {
            if (!visited[j])
            {
                int distance = get_distance(current_city, j, distances, num_cities);
                if (distance < min_distance)
                {
                    min_distance = distance;
                    nearest_city = j;
                }
            }
        }

        path.push_back(nearest_city);
        visited[nearest_city] = true;
        total_distance += min_distance;
        current_city = nearest_city;
    }

    // Add distance back to the starting city
    total_distance += get_distance(current_city, 0, distances, num_cities);

    return {path, total_distance};
}

int read_file(const std::string &filename, std::vector<int> &distances)
{
    int num_cities;
    std::ifstream infile(filename);

    if (!infile.is_open())
    {
        std::cerr << "Error: Unable to open file." << std::endl;
        return 0;
    }
    infile >> num_cities;

    distances.resize(num_cities * num_cities);
    for (int i = 0; i < num_cities; i++)
    {
        for (int j = 0; j < i; j++)
        {
            int distance;
            infile >> distance;
            distances[i * num_cities + j] = distance;
            distances[j * num_cities + i] = distance;
        }
    }

    infile.close();
    return num_cities;
}

int read_tsplib_matrix(const std::string &filename, std::vector<int> &distances)
{
    std::ifstream infile(filename);
    if (!infile.is_open())
    {
        std::cerr << "Error: Unable to open file." << std::endl;
        return 0;
    }

    std::string line;
    int num_cities = 0;

    // Parse the file header to get the number of cities (DIMENSION)
    while (std::getline(infile, line))
    {
        if (line.substr(0, 9) == "DIMENSION")
        {
            num_cities = std::stoi(line.substr(11));
        }
        else if (line == "EDGE_WEIGHT_SECTION")
        {
            break;
        }
    }

    distances.resize(num_cities * num_cities, 0);

    // Read the lower diagonal row of the distance matrix
    for (int i = 0; i < num_cities; i++)
    {
        for (int j = 0; j <= i; j++)
        {
            int distance;
            infile >> distance;
            distances[i * num_cities + j] = distance;
            distances[j * num_cities + i] = distance; // Symmetric TSP
        }
    }

    infile.close();
    return num_cities;
}

void log_event(
    const std::string &filename,
    const std::string &action,
    const std::string &hostname,
    int current_thread_id,
    double start_time,
    double end_time)
{
    std::ofstream log_file;
    log_file.open(filename, std::ios_base::app);
    log_file << std::fixed << std::setprecision(8);
    log_file << action << "," << hostname << "," << current_thread_id << "," << start_time << "," << end_time << std::endl;
    log_file.close();
}