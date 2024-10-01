import multiprocessing as mp
from pathlib import Path
from sys import argv
from matplotlib.patches import Rectangle
from matplotlib.collections import PatchCollection
import matplotlib.pyplot as plt
import pandas as pd
import time

FACECOLOR_DICT = {
    "SETUP": "blue",
    "COMMUNICATION": "red",
    "ORCHESTRATION": "orange",
    "COMPUTATION": "green",
}

HOSTNAME_COLOR_DICT = {}  # This will store the colors for each hostname

def read_results(file_path: str | Path) -> pd.DataFrame:
    df = pd.read_csv(
        file_path,
        header=None,
        names=["description", "hostname", "thread_id", "start", "end"],
    )
    min_time = df["start"].min()
    df["start"] -= min_time
    df["end"] -= min_time
    return df[df["description"].isin(FACECOLOR_DICT.keys())]

def remap_threads(df: pd.DataFrame) -> pd.DataFrame:
    # Group threads by hostname
    hostname_groups = df.groupby("hostname")["thread_id"].unique()
    
    # Create a mapping for new thread ids
    remap_dict = {}
    new_thread_id = 0
    for hostname, threads in hostname_groups.items():
        for thread in sorted(threads):  # Sort threads to keep the original order within each hostname
            remap_dict[thread] = new_thread_id
            new_thread_id += 1
    
    # Apply the remapping to the DataFrame
    df["thread_id"] = df["thread_id"].map(remap_dict)
    return df

def create_bar_data(row):
    return {
        'rect': Rectangle((row["start"], row["thread_id"] - 0.45), row["end"] - row["start"], 0.9),
        'facecolor': FACECOLOR_DICT[row["description"]],
        'hostname': row['hostname']
    }

if __name__ == "__main__":
    if len(argv) != 2:
        print(f"Usage: python {argv[0]} <logs_file>")
        exit(1)

    logs_file = Path(argv[1])
    if not logs_file.exists():
        print(f"File {logs_file} not found")
        exit(1)

    start_time = time.time()
    results = read_results(logs_file)
    results = remap_threads(results)  # Remap the threads to group by hostname
    print(f"Time to read and remap results: {time.time() - start_time:.2f} seconds")

    # Count the number of unique thread IDs after remapping
    num_threads = results["thread_id"].nunique()
    unique_hostnames = results["hostname"].unique()

    fig, ax = plt.subplots()

    start_time = time.time()
    with mp.Pool() as pool:
        bar_data = pool.map(create_bar_data, (row for _, row in results.iterrows()))
    print(f"Time to create bar data: {time.time() - start_time:.2f} seconds")

    start_time = time.time()
    patches = [data['rect'] for data in bar_data]
    facecolors = [data['facecolor'] for data in bar_data]
    collection = PatchCollection(patches, facecolors=facecolors, edgecolors='none')
    ax.add_collection(collection)
    print(f"Time to add collection to plot: {time.time() - start_time:.2f} seconds")

    # Set the y-axis limits and ticks
    ax.set_ylim(-0.5, num_threads - 0.5)
    ax.set_yticks(range(num_threads))
    ax.set_yticklabels(range(num_threads))

    # Set the x-axis to show time in seconds
    max_time = results["end"].max()
    ax.set_xlim(0, max_time)
    ax.set_xlabel("Time (seconds)")

    # Add a hostname color-coded legend on the y-axis
    for hostname in unique_hostnames:
        if hostname not in HOSTNAME_COLOR_DICT:
            HOSTNAME_COLOR_DICT[hostname] = plt.cm.Dark2(len(HOSTNAME_COLOR_DICT) % 8)  # Using Dark2 colormap

    for thread_id in range(num_threads):
        hostname = results[results["thread_id"] == thread_id]["hostname"].iloc[0]
        ax.get_yticklabels()[thread_id].set_color(HOSTNAME_COLOR_DICT[hostname])

    # Add a legend for the descriptions below the plot
    legend_elements = [plt.Rectangle((0, 0), 1, 1, facecolor=color, edgecolor='none', label=desc)
                       for desc, color in FACECOLOR_DICT.items()]
    ax.legend(handles=legend_elements, loc='upper center', bbox_to_anchor=(0.5, -0.05),
              ncol=len(FACECOLOR_DICT), frameon=False)

    # Adjust the figure size and layout
    fig.set_size_inches(20, 10)
    plt.tight_layout()
    
    print("Image generated, saving...")

    start_time = time.time()
    plt.savefig(logs_file.parent / f"{logs_file.stem}.png", dpi=300, bbox_inches='tight')
    print(f"Time to save image: {time.time() - start_time:.2f} seconds")
