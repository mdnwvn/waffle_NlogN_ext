
#ifndef NLN_LEVELS_H
#define NLN_LEVELS_H

#include <string>


struct levels_entry {
    bool exists;
    int port;
};

const std::string           lookup_table_path = "/root/waffle_NlogN_ext/nlogn/NLNTraceFiles/level_map.txt";

const std::string           levels_host = "127.0.0.1";
const struct levels_entry   levels_map = {.exists = true, .port = 9080};const int levels_len  = 24;
const struct levels_entry   levels[24] = {
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = true, .port = 9092  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
    {.exists = false, .port = -1  },
};
#endif