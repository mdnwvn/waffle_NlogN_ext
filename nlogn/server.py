import pathlib
import math
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# Wrapper class for a temporary set
class Set:
    level: int = 0
    length: int = 0
    size: int = 0
    records: [{"key": str, "value": str}] = []

    def __init__(self, level: int):
        self.level = level

    def append(self, key: str, value:str)-> None:
        self.records.append({"key": key, "value": value})
        self.length += 1
        self.size += len(value)


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




def splitDB(path: str, size: int) -> None:
    setCount: int = findPow2(size)

    lookup = Set(-1) # The level of the lookup table doesn't matter

    # Initialize t+1 temporary sets for setting up the database.
    sets: [Set] = []
    for i in range(0, setCount + 1):
        sets.append(Set(i))

    
    # // Debug and plotting code //
    # print(setCount)
    # print(sets[-1].level)
    #sizes:[int] = []


    with open(path, mode="r") as db:
        records = db.readlines()
        for record in records:
            parts = record.split(" ")
            sizebin = findPow2(len(parts[2]))

            sets[sizebin].append(parts[1], parts[2].ljust(2**sizebin, "\255"))

            # // Debug and plotting code //
            #print(f"{findPow2(len(parts[2]))} | {parts[2]}")
            # sizes.append(findPow2(len(parts[2])))
            

    for s in sets:
        print(s.length)
    # // Plotting code //
    #print("Plotting DB distribution.")
    #ax = sns.histplot(data=sizes, bins=range(0, setCount + 1))
    #ax.set_yscale('log')
    #print("Writing plot to file.")
    #plt.savefig("./dbdistrib.png")

    return


def initNLN():
    return


def main() -> int:

    # Input tracefile path
    dbPath = pathlib.Path("./DBTraceFiles/serverInput.txt").resolve()

    dbSize: int = getSize(dbPath)  # Get the size of the database in bytes
    splitDB(dbPath, dbSize)  # Split the DB into logN levels
    initNLN()  # Initialize each Waffle instance

    # TODO: Create a Thrift interface for benchmarking

    return 0


main()
