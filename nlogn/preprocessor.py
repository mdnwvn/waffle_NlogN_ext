import pathlib
import math
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import os
import asyncio

wafflePath = pathlib.Path("../waffle/bin/proxy_server").resolve()
#waffleHost = "127.0.0.1"
waffleStartPort = 9090
nlnLevelMapPort = 9080
#
redisHost = "127.0.0.1"
redisPort = 6379


# Class to help with managing waffle instances.
class Handle:
    def __init__(
        self, used: bool = False, command: str = None, port: int = None
    ):
        self.used = used
        if used:
            self.command = command
        self.client: Client = None
        self.port = port


handles: [Handle] = []
#num_handles = 0

levelMap: Handle 

event_loop = asyncio.get_event_loop()


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

    levelMapPath = pathlib.Path("./NLNTraceFiles/level_map.txt").resolve()
    levelMap = Handle(
        used=True,
        command=" ".join([str(wafflePath),
                "-l",
                str(levelMapPath),
                "-r",
                "800",
                "-f",
                "100",
                "-d",
                "100",
                "-c",
                "3",
                "-n",
                "2",  # num cores
                "-h",
                redisHost,
                "-p",
                str(redisPort),
                "-0",
                "9080"]),
        port= 9080
    )
    for i in range(0, sets + 1):

        levelPath = pathlib.Path(f"./NLNTraceFiles/level_{i}.txt").resolve()
        with open(levelPath, "r") as fp:

            # We are intentionally throwing out empty levels in an effort to
            # save memory on our machines, rather than filling them with dummy elements.
            print(f"Starting level {i}")
            if len(fp.read(1)) == 0:
                print("FILE IS EMPTY")
                handles.append(Handle(False, None, None))
            else:
                print(
                    f"level {i} has {str(2**(sets+ 1 - i) - len(fp.readlines()))} dummies"
                )

                handles.append(
                    Handle(
                        True,
                        " ".join([
                                str(wafflePath),
                                "-l",
                                str(levelPath),
                                "-r",
                                "800",
                                "-f",
                                "100",
                                "-d",
                                str(2 ** (sets + 1 - i) - len(fp.readlines())),
                                "-c",
                                "2",
                                "-n",
                                "2",  # num cores
                                "-h",
                                redisHost,
                                "-p",
                                str(redisPort),
                                "-0",
                                str(waffleStartPort + i),
                            ]),
                        waffleStartPort + i,
                    )
                )

    # TODO: figure out why only the first spawned proxy server runs
    # and why it dies after a few seconds.


    return levelMap
if __name__ == "__main__":

    # Input tracefile path
    # dbPath = pathlib.Path("./DBTraceFiles/serverInput.txt").resolve()
    dbPath = pathlib.Path(
        "../waffle/tracefiles/0.99/workloadc/proxy_server_command_line_input.txt"
    ).resolve()

    dbSize: int = getSize(dbPath)  # Get the size of the database in bytes
    sets: int = splitDB(dbPath, dbSize)  # Split the DB into logN levels

    levelMap = initNLN(sets)  # Initialize each Waffle instance

    levelsHost = "10.10.153.115"

    with open("./nln_level_commands.txt","w+") as f:
        f.write(f"{levelMap.command}\n")
        for level in handles:
            if level.used:
                f.write(f"{level.command}\n")
            pass
        pass

    with open("./nln_proxy/src/nln_levels.h","w+") as f:
        f.write("""
#ifndef NLN_LEVELS_H
#define NLN_LEVELS_H

#include <string>


struct levels_entry {
    bool exists;
    int port;
};\n""")

        f.write(f'const std::string           levels_host = "{levelsHost}";\n')
        f.write(f"const struct levels_entry   levels_map = {{.exists = true, .port = {nlnLevelMapPort}}};")

        f.write(f"const int levels_len  = {sets + 1};\n")
        f.write(f"const struct levels_entry   levels[{sets + 1}] = {{\n")
    
        for level in handles:
            f.write(f"    {{.exists = {'true' if level.used else 'false' }, .port = {level.port if level.used else '-1'}  }},\n")
            pass
        f.write("};\n")
        f.write("#endif")

        pass
    input("Press Enter to continue...")
