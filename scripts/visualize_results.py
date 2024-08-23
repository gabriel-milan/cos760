from pathlib import Path
from sys import argv

import matplotlib.pyplot as plt
import pandas as pd

FACECOLOR_DICT = {
    "SETUP": "blue",
    "COMMUNICATION": "red",
    "ORCHESTRATION": "orange",
    "COMPUTATION": "green",
}


def read_results(file_path: str | Path) -> pd.DataFrame:
    df = pd.read_csv(
        file_path,
        header=None,
        names=["description", "hostname", "thread_id", "start", "end"],
    )
    min_time = df["start"].min()
    df["start"] -= min_time
    df["end"] -= min_time
    return df


if __name__ == "__main__":
    if len(argv) != 2:
        print(f"Usage: python {argv[0]} <logs_file>")
        exit(1)

    logs_file = Path(argv[1])
    if not logs_file.exists():
        print(f"File {logs_file} not found")
        exit(1)

    results = read_results(logs_file)
    fig, ax = plt.subplots()
    height = 0.9
    for _, row in results.iterrows():
        ax.broken_barh(
            [(row["start"], row["end"] - row["start"])],
            (row["thread_id"] - height / 2, height),
            facecolors=FACECOLOR_DICT[row["description"]],
            # edgecolor="black",
        )
    # Revert y-axis
    ax.invert_yaxis()
    # Save big image with horizontal layout
    fig.set_size_inches(60, 10)
    plt.savefig(logs_file.parent / f"{logs_file.stem}.png", dpi=300)
