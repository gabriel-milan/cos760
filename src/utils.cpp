#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

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