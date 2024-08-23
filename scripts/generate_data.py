from pathlib import Path
from sys import argv

import matplotlib.pyplot as plt
import numpy as np

CITY_COORDINATES_FILE = "city_coordinates.txt"
TSP_INPUT_FILE = "tsp_input.txt"


def generate_data(*, output_path: str | Path, num_cities: int) -> None:
    """
    Generate random city coordinates and distances for the Travelling Salesman Problem (TSP).

    Args:
        output_path (str or Path): Path to save the generated data.
        num_cities (int): Number of cities to generate.
    """
    # Check if output_path exists. If it does, make sure it's a directory.
    output_path = Path(output_path)
    if output_path.exists():
        if not output_path.is_dir():
            raise ValueError("output_path must be a directory")
    else:
        output_path.mkdir(parents=True)

    # Generate random coordinates for each city
    filename_xy = output_path / CITY_COORDINATES_FILE
    positions = np.random.rand(num_cities, 2) * 100
    with open(filename_xy, "w") as f_xy:
        for pos in positions:
            f_xy.write(f"{pos[0]} {pos[1]}\n")

    # Calculate the distances and save them to the TSP file
    filename_tsp = output_path / TSP_INPUT_FILE
    with open(filename_tsp, "w") as f_tsp:
        f_tsp.write(f"{num_cities}\n")
        for i in range(1, num_cities):
            distances = []
            for j in range(i):
                distance = int(np.linalg.norm(positions[i] - positions[j]))
                distances.append(str(distance))
            f_tsp.write(" ".join(distances) + "\n")


def plot_map(*, path: str | Path, plot_distances: bool = False):
    """
    Plot the cities and distances on a map.

    Args:
        path (str or Path): Path to the city coordinates and TSP distances files.
    """
    # Get filenames
    path = Path(path)
    filename_xy = path / CITY_COORDINATES_FILE
    filename_tsp = path / TSP_INPUT_FILE

    # Read the XY coordinates
    with open(filename_xy, "r") as f:
        positions = np.array([list(map(float, line.split())) for line in f])

    num_cities = len(positions)

    # Initialize the distance matrix
    distances = np.zeros((num_cities, num_cities), dtype=int)

    # Read the TSP distances
    with open(filename_tsp, "r") as f_tsp:
        num_cities_tsp = int(f_tsp.readline().strip())

        for i in range(1, num_cities_tsp):
            row = list(map(int, f_tsp.readline().strip().split()))
            for j in range(i):
                distances[i, j] = row[j]  # Fill the lower triangle
                distances[j, i] = row[j]  # Symmetric distance for the upper triangle

    # Create the plot
    plt.figure(figsize=(8, 8))

    # Plot the cities as points
    for i in range(num_cities):
        plt.scatter(positions[i][0], positions[i][1], s=100, c="blue")
        plt.text(positions[i][0], positions[i][1], f"City {i}", fontsize=12, ha="right")

    # Draw lines between cities to represent distances
    if plot_distances:
        for i in range(num_cities):
            for j in range(i):
                distance = distances[i, j]
                plt.plot(
                    [positions[i][0], positions[j][0]],
                    [positions[i][1], positions[j][1]],
                    "k-",
                    lw=1,
                )
                mid_x = (positions[i][0] + positions[j][0]) / 2
                mid_y = (positions[i][1] + positions[j][1]) / 2
                plt.text(mid_x, mid_y, f"{distance}", fontsize=10, color="red")

    plt.title("Travelling Salesman Problem - City Map")
    plt.grid(True)
    plt.show()


if __name__ == "__main__":
    if len(argv) != 3:
        print(f"Usage: python {argv[0]} <output_path> <num_cities>")
    else:
        output_path = argv[1]
        num_cities = int(argv[2])
        generate_data(output_path=output_path, num_cities=num_cities)
        plot_map(path=output_path, plot_distances=True)
