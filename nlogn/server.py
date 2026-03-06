import pathlib
import math
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import subprocess


# Wrapper class for a temporary set
class Set:
    def __init__(self, level: int):
        self.level: int = 0
        self.length: int = 0
        self.size: int = 0
        self.records: [{"key": str, "value": str}] = []
        self.level = level

    def append(self, key: str, value: str) -> None:
        self.records.append({"key": key, "value": value})
        self.length += 1
        self.size += len(value) if isinstance(value, str) else 1


# Modified from https://www.geeksforgeeks.org/dsa/smallest-power-of-2-greater-than-or-equal-to-n/
# Helper function to find the nearest power of two greater than a given N
def findPow2(N: int) -> int:
    # Calculate log2 of N
    t: int = int(math.log2(N))
    # If 2^a is equal to N, return t
    if 2**t == N:
        return t
    # Return t+1
    return t + 1


def getSize(path: str) -> int:
    size: int = 0

    with open(path, mode="r") as db:
        records = db.readlines()
        for record in records:
            size += len(record.split(" ")[2])
    return size


def splitDB(path: str, size: int) -> int:
    setCount: int = findPow2(size)

    lookup = Set(-1)  # The level of the lookup table doesn't matter

    # Initialize t+1 temporary sets for setting up the database.
    sets: [Set] = []
    for i in range(0, setCount + 1):
        sets.append(Set(i))

    # // Debug and plotting code //
    # print(setCount)
    # print(sets[-1].level)
    # sizes:[int] = []

    # Open and parse the dataset tracefile
    with open(path, mode="r") as db:
        # records = db.readlines()
        for record in db:
            parts = record.split(" ")
            sizebin = findPow2(len(str(parts[2]).strip()))

            # Add the k-v pair to the appropriate level
            sets[sizebin].append(parts[1], str(parts[2]).strip().ljust(2**sizebin, "█"))

            # Add the key to the lookup set.
            # We need to pad this otherwise we leak whether we've
            # accessed a 1-digit or 2-digit level.
            lookup.append(parts[1], str(sizebin).strip().ljust(4, "█"))

            # // Debug and plotting code //
            # print(f"{findPow2(len(parts[2]))} | {parts[2]}")
            # sizes.append(findPow2(len(parts[2])))

    # Generate the tracefile for each level
    # TODO: Add a single dummy element for empty levels, either here or later, so waffle can generate the proper dummies.
    for i, s in enumerate(sets):
        with open(f"./NLNTraceFiles/level_{i}.txt", "w+", encoding="utf-8") as f:
            for t in sets[i].records:
                f.write(f"SET {t['key']} {t['value']}\n")
            print(sets[i].level)

    # Generate the index tracefile
    with open(f"./NLNTraceFiles/level_map.txt", "w+", encoding="utf-8") as f:
        for t in lookup.records:
            f.write(f"SET {t['key']} {t['value']}\n")

    # // Plotting code //
    # print("Plotting DB distribution.")
    # ax = sns.histplot(data=sizes, bins=range(0, setCount + 1))
    # ax.set_yscale('log')
    # print("Writing plot to file.")
    # plt.savefig("./dbdistrib.png")

    return setCount


def initNLN(sets: int):

    wafflePath = pathlib.Path("../waffle/bin/proxy_server").resolve()
    waffleStartPort = 9090

    redisHost = "127.0.0.1"
    redisPort = 6379

    levelMapPath = pathlib.Path("./NLNTraceFiles/level_map.txt").resolve()
    lmap_handle: subprocess.Popen = subprocess.Popen(
        [
            wafflePath,
            "-l",  levelMapPath,
            "-r", "800",
            "-f", "100",
            "-d", "100000",
            "-c", "3",
            "-n", "1",
            "-h", redisHost,
            "-p", str(redisPort),
        ]
    )
    level_handles: [subprocess.Popen] = []

    for i in range(0, sets + 1):

        levelPath = pathlib.Path(f"./NLNTraceFiles/level_{i}.txt").resolve()
        print(f"Starting level {i}")
        level_handles.append(
            subprocess.Popen(
                    [
                        wafflePath,
                        "-l",  levelPath,
                        "-r", "800",
                        "-f", "100",
                        "-d", "100000",
                        "-c", "3",
                        "-n", "1",
                        "-h", redisHost,
                        "-p", str(redisPort),
                        "-0", str(waffleStartPort + i)
                    ]
                )
            
            )

    #TODO: figure out why only the first spawned proxy server runs
    # and why it dies after a few seconds. Worst case, we have to
    # switch from a normal subprocess to something else.
    print(lmap_handle.pid)
    for p in level_handles:
        print(p.pid)
    input("Press Enter to continue...")

    return


def main() -> int:

    # Input tracefile path
    dbPath = pathlib.Path("./DBTraceFiles/serverInput.txt").resolve()

    dbSize: int = getSize(dbPath)  # Get the size of the database in bytes
    sets: int = splitDB(dbPath, dbSize)  # Split the DB into logN levels

    initNLN(sets)  # Initialize each Waffle instance

    # TODO: Create a Thrift interface



    return 0


main()
