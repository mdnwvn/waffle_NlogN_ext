import pathlib
import math
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import os
import asyncio

## Thift server imports
#from proxy import waffle_thrift
#from proxy import waffle_thrift_response
#from proxy import ttypes
#from thrift.transport import TSocket
#from thrift.transport import TTransport
#from thrift.protocol import TBinaryProtocol
#from thrift.server import TServer
#
## Thrift client imports
#from proxy.waffle_thrift import Client
#from proxy.waffle_thrift import sequence_id
#
#wafflePath = pathlib.Path("../waffle/bin/proxy_server").resolve()
#waffleHost = "127.0.0.1"
#waffleStartPort = 9090
#
#redisHost = "127.0.0.1"
#redisPort = 6379


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

    # TODON'T: figure out why only the first spawned proxy server runs
    # and why it dies after a few seconds. Worst case, we have to
    # switch from a normal subprocess to something else.
    # print(levelMap.handle.pid)
    # for p in handles:
    #     if p.used:
    #         print(p.handle.pid)

    return levelMap
#__HOST = "localhost"
#__PORT = 7080


#class ProxyHandler(object):
#    def get_client_id(self):
#        pass
#
#    def register_client_id(self, block_id, client_id):
#        """
#        Parameters:
#         - block_id
#         - client_id
#
#        """
#        pass
#
#    def async_get(self, seq_id, key):
#        """
#        Parameters:
#         - seq_id
#         - key
#
#        """
#        pass
#
#    def async_put(self, seq_id, key, value):
#        """
#        Parameters:
#         - seq_id
#         - key
#         - value
#
#        """
#        pass
#
#    def async_get_batch(self, seq_id, keys):
#        """
#        Parameters:
#         - seq_id
#         - keys
#
#        """
#        pass
#
#    def async_put_batch(self, seq_id, keys, values):
#        """
#        Parameters:
#         - seq_id
#         - keys
#         - values
#
#        """
#        pass
#
#    def get(self, key):
#        print(key)
#        """
#        Parameters:
#         - key
#
#        """
#
#        return
#
#    def put(self, key, value):
#        """
#        Parameters:
#         - key
#         - value
#
#        """
#        return
#
#    def get_batch(self, keys: [str]):
#        """
#        Parameters:
#         - keys
#
#        """
#        return [""]
#
#    def put_batch(self, keys: [str], values: [str]):
#        """
#        Parameters:
#         - keys
#         - values
#
#        """
#        pass


# def makeClient(port: int) -> Client:
#     tsocket = TSocket.TSocket(waffleHost, port)
#     transport = TTransport.TFramedTransport(tsocket)
#     protocol = TBinaryProtocol.TBinaryProtocol(transport)
#     transport.open()
#     return Client(protocol)

if __name__ == "__main__":

    # Input tracefile path
    # dbPath = pathlib.Path("./DBTraceFiles/serverInput.txt").resolve()
    dbPath = pathlib.Path(
        "../waffle/tracefiles/0.99/workloada/proxy_server_command_line_input.txt"
    ).resolve()

    dbSize: int = getSize(dbPath)  # Get the size of the database in bytes
    sets: int = splitDB(dbPath, dbSize)  # Split the DB into logN levels

    levelMap = initNLN(sets)  # Initialize each Waffle instance

    levelsHost = "10.10.153.115"

    with open("./nln_level_commands.txt","w+") as f:
        f.write(f"{levelMap.command}\n")
        for level in levelMap:
            if level.used:
                f.writable(f"{level.command}\n")
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
};""")

        f.write(f'const std::string           levels_host = "{levelsHost}";\n')
        f.write(f"const struct levels_entry   levels[{sets + 1}] = {{\n")
    
        for level in levelMap:
            f.write(f"{{ }},\n")
            pass
        f.write("};\n")
        f.write(f"const int                   levels_len  = {level};")




#endif""")
# )
        pass

    #print(f"Waiting for Level map")
    #while True:
    #    line = levelMap.handle.stdout.readline()
    #    #print (line)
    #    if line == b'Proxy server is reachable\n':
    #        print("Level map ready")
    #        #levelMap.client = makeClient(levelMap.port)

    #        #print(levelMap.client.get("user7997323107739036606"))
    #        break
    #    pass

    #for i, p in enumerate(handles):
    #    print(f"Waiting for Level {i}")
    #    if p.used == False:
    #        print(f"Level {i} is unused")
    #        continue
    #    while True:
    #        line = p.handle.stdout.readline()
    #        #print (line)
    #        if line == b'Proxy server is reachable\n':
    #            print(f"Level {i} ready")
    #            break
    #        pass

    # TODO: Create a Thrift interface
#    handler = ProxyHandler
#    processor = waffle_thrift.Processor(handler)
#    transport = TSocket.TServerSocket(__HOST, __PORT)
#    tfactory = TTransport.TFramedTransportFactory()
#    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
#    rpcServer = TServer.TSimpleServer(processor, transport, tfactory, pfactory)
#    print("Starting the rpc server at", __HOST, ":", __PORT)
#    rpcServer.serve()
#
    input("Press Enter to continue...")
